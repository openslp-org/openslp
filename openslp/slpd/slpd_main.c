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
int G_SIGALRM;
int G_SIGTERM;
int G_SIGHUP;
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
int SetUpSignalHandlers()
/*-------------------------------------------------------------------------*/
{
    int result;
    struct sigaction sa;

    sa.sa_handler    = SignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags      = 0;//SA_ONESHOT;
#if defined(HAVE_SA_RESTORER)
    sa.sa_restorer   = 0;
#endif
    
    result = sigaction(SIGALRM,&sa,0);
    result |= sigaction(SIGTERM,&sa,0);
    result |= sigaction(SIGPIPE,&sa,0);
    
    signal(SIGHUP,SignalHandler);
    //result |= sigaction(SIGHUP,&sa,0);

    return result;
}


/*-------------------------------------------------------------------------*/
int Daemonize(const char* pidfile)
/* Turn the calling process into a daemon (detach from tty setuid(), etc   */
/*                                                                         */      
/* Returns: zero on success non-zero if slpd could not daemonize (or if    */
/*          slpd is already running                             .          */
/*-------------------------------------------------------------------------*/
{
    pid_t   pid;
    FILE*   fd;
    struct  passwd* pwent;
    char    pidstr[13];
    
    if(G_SlpdCommandLine.detach)
    {
    
        /*-------------------------------------------*/
        /* Release the controlling tty and std files */
        /*-------------------------------------------*/
        switch(fork())
        {
        case -1:
            return -1;
        case 0:
            /* child lives */
            break;
    
        default:
            /* parent dies */
            exit(0);
        }
    
        close(0); 
        close(1); 
        close(2); 
        setsid(); /* will only fail if we are already the process group leader */
    }
     
    /*------------------------------------------*/
    /* make sure that we're not running already */
    /*------------------------------------------*/
    /* read the pid from the file */
    fd = fopen(pidfile,"r");
    if(fd)
    {
        fread(pidstr,13,1,fd);
        fclose(fd);
        pid = atoi(pidstr);
        if(pid)
        {
            if(kill(pid,0) == 0)
            {
                /* we are already running */
                SLPFatal("slpd daemon is already running\n");
                return -1;
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
    
    /*----------------*/
    /* suid to daemon */
    /*----------------*/
    /* TODO: why do the following lines mess up my signal handlers? */
    pwent = getpwnam("daemon"); 
    if(pwent)
    {
        if( setgroups(1, &pwent->pw_gid) < 0 ||
            setgid(pwent->pw_gid) < 0 ||
            setuid(pwent->pw_uid) < 0 )
        {
            /* TODO: should we log here and return fail */
        }
    }
    
    return 0;
}


/*-------------------------------------------------------------------------*/
void LoadFdSets(SLPDSocketList* list, 
                int* highfd, 
                fd_set* readfds, 
                fd_set* writefds)
/*-------------------------------------------------------------------------*/
{
    SLPDSocket* sock;
    
    sock = list->head;
    while(sock)
    {
        if(sock->fd > *highfd)
        {
            *highfd = sock->fd;
        }

        switch(sock->state)
        {
        case DATAGRAM_UNICAST:
        case DATAGRAM_MULTICAST:
        case DATAGRAM_BROADCAST:
            FD_SET(sock->fd,readfds);
            break;
            
        case SOCKET_LISTEN:
            if(list->count < SLPD_MAX_SOCKETS)
            {
                FD_SET(sock->fd,readfds);
            }
            break;

        case STREAM_READ:
        case STREAM_FIRST_READ:
            FD_SET(sock->fd,readfds);
            break;

        case STREAM_WRITE:
        case STREAM_FIRST_WRITE:
        case STREAM_CONNECT:
            FD_SET(sock->fd,writefds);
            break;

        case SOCKET_CLOSE:
        default:
            sock = SLPDSocketListRemove(list, sock); 
            break;
        }

        sock = (SLPDSocket*)sock->listitem.next;
    }
}


/*=========================================================================*/
int main(int argc, char* argv[])
/*=========================================================================*/
{
    fd_set          readfds;
    fd_set          writefds;
    int             highfd;
    int             fdcount         = 0;
    SLPDSocketList  incoming        = {0,0};
    SLPDSocketList  outgoing        = {0,0};
        
    
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

    
    /*------------------------------*/
    /* Initialize the log file      */
    /*------------------------------*/
    SLPLogFileOpen(G_SlpdCommandLine.logfile, 0);
    SLPLog("****************************************\n");
    SLPLog("*** SLPD daemon started              ***\n");
    SLPLog("****************************************\n");
    SLPLog("command line = %s\n",argv[0]);
    
    
    /*--------------------------------------------------*/
    /* Initialize for the first time                    */
    /*--------------------------------------------------*/
    SLPDPropertyInit(G_SlpdCommandLine.cfgfile);
    SLPDDatabaseInit(G_SlpdCommandLine.regfile);
    SLPDIncomingInit(&incoming);
    /* TODO SLPDOutgoingInit(&outgoing); */
    /* TODO: Check error codes on all init functions */
    SLPDKnownDAInit();
    
    
    /*---------------------------*/
    /* make slpd run as a daemon */
    /*---------------------------*/
    if(Daemonize(G_SlpdCommandLine.pidfile))
    {
        SLPFatal("Could not run as daemon\n");
    }
    

    /*-----------------------*/
    /* Setup signal handlers */ 
    /*-----------------------*/
    if(SetUpSignalHandlers())
    {
        SLPFatal("Could not set up signal handlers.\n");
    }

    
    /*------------------------------*/
    /* Set up alarm to age database */
    /*------------------------------*/
    alarm(SLPD_AGE_INTERVAL);

    
    /*-----------*/
    /* Main loop */
    /*-----------*/
    G_SIGALRM   = 0;
    G_SIGTERM   = 0;
    G_SIGHUP    = 0;    
    while(G_SIGTERM == 0)
    {
        
        /*--------------------------------------------------------*/
        /* Load the fdsets up with all valid sockets in the list  */
        /*--------------------------------------------------------*/
        highfd = 0;
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        LoadFdSets(&incoming, &highfd, &readfds,&writefds);
        LoadFdSets(&outgoing, &highfd, &readfds,&writefds);
        
        /*----------------------------------------------------------*/
        /* Before select(), check to see if we should reinitialize  */
        /*----------------------------------------------------------*/
        if(G_SIGHUP)
        {
            /* Reinitialize */
            SLPLog("****************************************\n");
            SLPLog("*** SLPD daemon restarted            ***\n");
            SLPLog("****************************************\n");
            SLPLog("Got SIGHUP reinitializing... \n");
        
            SLPDPropertyInit(G_SlpdCommandLine.cfgfile);
            SLPDDatabaseInit(G_SlpdCommandLine.regfile);
            SLPDIncomingInit(&incoming);
            /* TODO SLPDOutgoingInit(&outgoing); */
                
            SLPDKnownDAInit();
            G_SIGHUP = 0;
            
            /* continue to top of loop so that fd_sets are loaded again */
            continue; 
        }

        /*--------------------------------------------------------------*/
        /* Before select(), check to see if we should age the database  */
        /*--------------------------------------------------------------*/
        if(G_SIGALRM)
        {
            /* TODO: add call to do passive DAAdvert */
            SLPDSocketAge(&incoming, SLPD_AGE_INTERVAL);
            SLPDSocketAge(&outgoing, SLPD_AGE_INTERVAL);
            SLPDDatabaseAge(SLPD_AGE_INTERVAL);
            G_SIGALRM = 0;
            alarm(SLPD_AGE_INTERVAL);
            
            /* continue to top of loop so that fd_sets are loaded again */
            continue;
        }
        
        /*-------------*/
        /* Main select */
        /*-------------*/
        fdcount = select(highfd+1,&readfds,&writefds,0,0);
        if(fdcount > 0) /* fdcount will be < 0 when interrupted by a signal */
        {
            SLPDIncomingHandler(&incoming, &fdcount,&readfds,&writefds);
            SLPDOutgoingHandler(&outgoing, &fdcount,&readfds,&writefds);
        }

        /*--------------------------------*/
        /* Echo registrations to KnownDAs */
        /*--------------------------------*/ 
        /* TODO: Put call to SLPDKnownDARegister() here */


    } /* End of main loop */

    SLPLog("Got SIGTERM.  Going down\n");

    return 0;
}
