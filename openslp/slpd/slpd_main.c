/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_main.c                                                */
/*                                                                         */
/* Abstract:    Main daemon loop                                           */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/* Copyright (c) 1995, 1999  Caldera Systems, Inc.                         */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify it */
/* under the terms of the GNU Lesser General Public License as published   */
/* by the Free Software Foundation; either version 2.1 of the License, or  */
/* (at your option) any later version.                                     */
/*                                                                         */
/*     This program is distributed in the hope that it will be useful,     */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       */
/*     GNU Lesser General Public License for more details.                 */
/*                                                                         */
/*     You should have received a copy of the GNU Lesser General Public    */
/*     License along with this program; see the file COPYING.  If not,     */
/*     please obtain a copy from http://www.gnu.org/copyleft/lesser.html   */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*     Please submit patches to http://www.openslp.org                     */
/*                                                                         */
/***************************************************************************/

#include "slpd.h"

/*==========================================================================*/
int G_SIGALRM   = 0;
int G_SIGTERM   = 0;
int G_SIGHUP    = 0;
/*==========================================================================*/                                                       
                                                       

/*--------------------------------------------------------------------------*/
void SignalHandler(int signum)
/*--------------------------------------------------------------------------*/
{
    switch(signum)
    {
    case SIGALRM:
        G_SIGALRM = 1;
        break;

    case SIGTERM:
        G_SIGTERM = 1;

    case SIGHUP:
        G_SIGHUP = 1;
        
    case SIGPIPE:
    default:
        return;
    }
}


                  
/*-------------------------------------------------------------------------*/
void HandleTCPListen(SLPDSocketList* list, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    SLPDSocket* conn;

    if(list->count >= SLPD_MAX_SOCKETS)
    {
        return;
    }

    conn = (SLPDSocket*)malloc(sizeof(SLPDSocket));
    if(conn)
    {
        memset(conn,0,sizeof(SLPDSocket));
        conn->peeraddrlen = sizeof(conn->peeraddr);
        conn->fd = accept(sock->fd,
                          &(conn->peeraddr),
                          &(conn->peeraddrlen));
        if(conn->fd >= 0)
        {
            conn->type = TCP_FIRST_READ;
            conn->recvbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
            conn->sendbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
            
            /* TODO: set timestamp of API connections to zero so they */
            /* will be permanent */

            time(&(conn->timestamp));
            SLPDebug("Accepted connection to %x\n",conn->peeraddr.sin_addr.s_addr);
            SLPDSocketListLink(list,conn);
        }
        else
        {
            /* an error occured during accept()  */
            /* LOG: should we log here   */
            free(conn);
        }
    }
}


/*-------------------------------------------------------------------------*/
void HandleUDPRead(SLPDSocketList* list, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    int                 bytesread;
    int                 bytestowrite;
    struct sockaddr_in  peeraddr;
    int                 peeraddrlen    = sizeof(peeraddr);

    memset(&peeraddr,0,sizeof(peeraddr));
    if(sock->recvbuf && sock->sendbuf)
    {
        bytesread = recvfrom(sock->fd,
                              sock->recvbuf->start,
                              SLP_MAX_DATAGRAM_SIZE,
                              0,
                              &peeraddr,
                              &peeraddrlen);
        if(bytesread > 0)
        {
            SLPDebug("Received UDP message from %s\n",
                     inet_ntoa(peeraddr.sin_addr));
            
            sock->recvbuf->end = sock->recvbuf->start + bytesread;

            if(SLPDProcessMessage(sock->recvbuf,sock->sendbuf) == 0)
            {
                /* check to see if we should send anything */
                bytestowrite = sock->sendbuf->end - sock->sendbuf->start;
                if(bytestowrite > 0)
                {
                    if(sendto(sock->fd,
                          sock->sendbuf->start,
                          sock->sendbuf->end - sock->sendbuf->start,
                          0,
                          &peeraddr,
                          peeraddrlen) > 0)
                    {
                        SLPDebug("Sent UDP message to %s\n",
                                 inet_ntoa(peeraddr.sin_addr));
                    }   
                }
            }
            else
            {
                SLPError("An error occured while processing message from %s\n",
                         inet_ntoa(peeraddr.sin_addr));
            } 
        }
    }
    else
    {
        /* we're out of memory, drop the UDP datagram */
        recv(sock->fd,&bytesread,4,0);  
        /* &bytesread serves as a (small) temporary bucket to drop data into */
        SLPError("Slpd is out of memory!!\n");
    }
}


/*-------------------------------------------------------------------------*/
void HandleTCPRead(SLPDSocketList* list, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    int     fdflags;
    int     bytesread;
    char    peek[16];
    
    if(sock->type == TCP_FIRST_READ)
    {
        fdflags = fcntl(sock->fd, F_GETFL, 0);
        fcntl(sock->fd,F_SETFL, fdflags | O_NONBLOCK);
        
        /*---------------------------------------------------------------*/
        /* take a peek at the packet to get version and size information */
        /*---------------------------------------------------------------*/
        bytesread = recvfrom(sock->fd,
                             peek,
                             16,
                             MSG_PEEK,
                             &(sock->peeraddr),
                             &(sock->peeraddrlen));
        if(bytesread > 0)
        {
            /* check the version */
            if(*peek == 2)
            {
                /* allocate the recvbuf big enough for the whole message */
                sock->recvbuf = SLPBufferRealloc(sock->recvbuf,AsUINT24(peek+2));
                if(sock->recvbuf)
                {
                    sock->type = TCP_READ;
                }
                else
                {
                    SLPError("Slpd is out of memory!!\n");
                    sock->type = SOCKET_CLOSE;
                }
            }
            else
            {
                SLPLog("Unsupported version %i received from %s\n",
                       *peek,
                       inet_ntoa(sock->peeraddr.sin_addr));

                sock->type = SOCKET_CLOSE;
            }
        }
        else
        {
            if(errno != EWOULDBLOCK)
            {
                SLPDebug("Closed connection to %s.  Error on recvfrom() %i\n", 
                         inet_ntoa(sock->peeraddr.sin_addr),errno);
                sock->type = SOCKET_CLOSE;
                return;
            }
        }        

        
        /*------------------------------*/
        /* recv the rest of the message */
        /*------------------------------*/
        bytesread = recv(sock->fd,
                         sock->recvbuf->curpos,
                         sock->recvbuf->end - sock->recvbuf->curpos,
                         0);              

        if(bytesread > 0)
        {
            /*------------------------------*/
            /* Reset the timestamp          */
            /*------------------------------*/
            time(&(sock->timestamp));
            
            sock->recvbuf->curpos += bytesread;
            if(sock->recvbuf->curpos == sock->recvbuf->end)
            {
                if(SLPDProcessMessage(sock->recvbuf,sock->sendbuf) == 0)
                {
                    sock->type = TCP_FIRST_WRITE;
                }
                else
                {
                    /* An error has occured in SLPDProcessMessage() */
                    SLPError("An error while processing message from %s\n",
                             inet_ntoa(sock->peeraddr.sin_addr));
                    sock->type = SOCKET_CLOSE;
                }                                                          
            }
        }
        else
        {
            if(errno != EWOULDBLOCK)
            {
                /* error in recv() */
                SLPDebug("Closed connection to %s.  Error on recv() %i\n", 
                         inet_ntoa(sock->peeraddr.sin_addr),errno);
                sock->type = SOCKET_CLOSE;
            }
        }
    }
}


/*-------------------------------------------------------------------------*/
void HandleTCPWrite(SLPDSocketList* list, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    int byteswritten;
    
    if(sock->type == TCP_FIRST_WRITE)
    {
        /* make sure that the start and curpos pointers are the same */
        sock->sendbuf->curpos = sock->sendbuf->start;
        sock->type = TCP_WRITE;
    }

    if(sock->sendbuf->end - sock->sendbuf->start != 0)
    {
        byteswritten = send(sock->fd,
                            sock->sendbuf->curpos,
                            sock->sendbuf->end - sock->sendbuf->start,
                            MSG_DONTWAIT);
        if(byteswritten > 0)
        {
            /*------------------------------*/
            /* Reset the timestamp          */
            /*------------------------------*/
            time(&(sock->timestamp));
    
            sock->sendbuf->curpos += byteswritten;
            if(sock->sendbuf->curpos == sock->sendbuf->end)
            {
                /* message is completely sent */
                sock->type = TCP_FIRST_READ;
             }
        }
        else
        {
            if(errno != EWOULDBLOCK)
            {
                /* Error occured or connection was closed */
                SLPDebug("Closed connection to %s.  Error on send()\n", 
                         inet_ntoa(sock->peeraddr.sin_addr));
    
                sock->type = SOCKET_CLOSE;
            }   
        }    
    }
}

/*-------------------------------------------------------------------------*/
int Daemonize(const char* pidfile)
/* turn the calling process into a daemon (detach from tty setuid(), etc   */
/*-------------------------------------------------------------------------*/
{
    #if(defined PARANOID)
    struct  passwd* p;
    #endif

    switch(fork())
    {
    case -1:
        return -1;
    case 0:
        close(0);
        close(1);
        close(2);

        #if(defined PARANOID)
        /* suid to daemon */
        p = getpwnam("daemon"); 
        if(p)
        {
            chown(pidfile, p->pw_uid, p->pw_gid);
            setgid(p->pw_gid);
            setuid(p->pw_uid);
        }                    
        #endif
        
        break;
    default:
        exit(0);
    }
    
    return 0;
}

/*-------------------------------------------------------------------------*/
int CheckForRunningSlpd(const char* pidfile)
/*-------------------------------------------------------------------------*/
{
    char    pidstr[13] = {0};
    pid_t   pid = 0;
    FILE*   fd = fopen(pidfile,"r");
    if(fd)
    {
        /* read the pid from the file */
        fread(pidstr,13,1,fd);
        fclose(fd);
        pid = atoi(pidstr);
        if(pid)
        {
            if(kill(pid,SIGWINCH) == 0)
            {
                return 1;
            }
        }
    }

    /* write my pid to the pidfile */
    fd = fopen(pidfile,"w");
    if(fd)
    {
        sprintf(pidstr,"%i",getpid());
        fwrite(pidstr,strlen(pidstr),1,fd);
        fclose(fd);
    }

    return 0;
}

/*=========================================================================*/
int main(int argc, char* argv[])
/*=========================================================================*/
{
    
    fd_set          readfds;
    fd_set          writefds;
    int             highfd      = 0;
    int             fdcount     = 0;
    SLPDSocket*      slpsocket   = 0;
    SLPDSocketList   socketlist  = {0,0};
    
    /*------------------------------*/
    /* Make sure we are root        */
    /*------------------------------*/
    if(getuid() != 0)
    {
        SLPFatal("slpd must be started by root\n");
    }
    
    /*------------------------*/
    /* Parse the command line */
    /*------------------------*/
    if(SLPDParseCommandLine(argc,argv))
    {
        SLPFatal("Invalid command line\n");
    }

    /*-----------------------------------------*/
    /* Check to see if slpd is already running */
    /*-----------------------------------------*/
    if(CheckForRunningSlpd(G_SlpdCommandLine.pidfile))
    {
        SLPFatal("slpd appears to be running already\n");
    }

    /*------------------------------*/
    /* Initialize the log file      */
    /*------------------------------*/
    #if(defined DEBUG)
    SLPLogSetLevel(4);
    if(SLPLogFileOpen(G_SlpdCommandLine.logfile, 0) != 0)
    #else
    SLPLogSetLevel(3);
    if(SLPLogFileOpen(G_SlpdCommandLine.logfile, 1) != 0)
    #endif                                     
    {
        SLPError("SLPD: Could not open logfile: %s. Logging disabled.\n",
                 G_SlpdCommandLine.logfile);
        SLPLogSetLevel(1);
    }
    SLPLog("****************************************\n");
    SLPLog("*** SLPD daemon started              ***\n");
    SLPLog("****************************************\n");
    SLPLog("command line = %s\n",argv[0]);
    
       
    /*--------------------------------------------------*/
    /* Initialize for the first time                    */
    /*--------------------------------------------------*/
    SLPDPropertyInit(G_SlpdCommandLine.cfgfile);
    SLPDDatabaseInit(G_SlpdCommandLine.regfile);
    SLPDSocketInit(&socketlist);
    
    
    /*-----------------------*/
    /* Setup signal handlers */ 
    /*-----------------------*/
    signal(SIGALRM,SignalHandler);
    signal(SIGTERM,SignalHandler);
    signal(SIGPIPE,SignalHandler);
    signal(SIGHUP,SignalHandler);


    /*------------------------------*/
    /* Set up initial timeout stuff */
    /*------------------------------*/
    alarm(SLPD_AGE_TIMEOUT);
    
//    if(Daemonize(G_SlpdCommandLine.pidfile) < 0)
//    {
//        SLPFatal("Could not run as a daemon\n");
//    }

    while(G_SIGTERM == 0)
    {
        if(G_SIGHUP)
        {
            /* Reinitialize */
            SLPLog("****************************************\n");
            SLPLog("*** SLPD daemon restarted            ***\n");
            SLPLog("****************************************\n");
            SLPLog("Got SIGHUP reinitializing... \n");
            SLPDPropertyInit(G_SlpdCommandLine.cfgfile);
            SLPDDatabaseInit(G_SlpdCommandLine.regfile);
            SLPDSocketInit(&socketlist);                
            G_SIGHUP = 0;
        }

        highfd = 0;
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        slpsocket = socketlist.head;
        while(slpsocket)
        {
            if(slpsocket->fd > highfd)
            {
                highfd = slpsocket->fd;
            }

            switch(slpsocket->type)
            {
            case UDP:
            case UDP_MCAST:
                FD_SET(slpsocket->fd,&readfds);
                break;
                
            case TCP_LISTEN:
                if(socketlist.count < SLPD_MAX_SOCKETS)
                {
                    FD_SET(slpsocket->fd,&readfds);
                }
                break;
    
            case TCP_READ:
            case TCP_FIRST_READ:
                FD_SET(slpsocket->fd,&readfds);
                break;
  
            case TCP_WRITE:
            case TCP_FIRST_WRITE:
                FD_SET(slpsocket->fd,&writefds);
                break;

            default:
                break;
            }
    
            slpsocket = slpsocket->next;
        }
        
        
        if(G_SIGALRM)
        {
            SLPDDatabaseAge(SLPD_AGE_TIMEOUT);
            G_SIGALRM = 0;
            alarm(SLPD_AGE_TIMEOUT);
        }
        
        fdcount = select(highfd+1,&readfds,&writefds,0,0);
        if(fdcount > 0)
        {
            slpsocket = socketlist.head;
            while(slpsocket && fdcount)
            {
                if(FD_ISSET(slpsocket->fd,&readfds))
                {
                    switch(slpsocket->type)
                    {
                    
                    case TCP_LISTEN:
                        HandleTCPListen(&socketlist,slpsocket);
                        break;

                    case UDP:
                    case UDP_MCAST:
                        HandleUDPRead(&socketlist,slpsocket);
                        break;                      
                
                    case TCP_READ:
                    case TCP_FIRST_READ:
                        HandleTCPRead(&socketlist,slpsocket);
                        break;

                    default:
                        break;
                    }

                    fdcount --;
                } 

                if(FD_ISSET(slpsocket->fd,&writefds))
                {
                    HandleTCPWrite(&socketlist,slpsocket);
                    fdcount --;
                }   

                /* TODO: Close aged sockets */

                if(slpsocket->type == SOCKET_CLOSE)
                {
                    slpsocket = SLPDSocketListDestroy(&socketlist,slpsocket);
                }
                else
                {
                    slpsocket = slpsocket->next;
                }
            }
        }
    }

    SLPLog("Got SIGTERM.  Going down\n");

    /* remove the pid file */
    unlink(G_SlpdCommandLine.pidfile);

    return 0;
}




