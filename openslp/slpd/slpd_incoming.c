/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_incoming.c                                            */
/*                                                                         */
/* Abstract:    Handles "incoming" network conversations requests made by  */
/*              other agents to slpd. (slpd_outgoing.c) handles reqests    */
/*              made by slpd to other agents                               */
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

/*-------------------------------------------------------------------------*/
void HandleSocketListen(SLPDSocketList* list, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    SLPDSocket* connsock;
    const int   lowat = SLPD_SMALLEST_MESSAGE;
    
    /* Only accept if we can. If we still maximum number of sockets, just*/
    /* ignore the connection */
    if(list->count < SLPD_MAX_SOCKETS)
    {
        connsock = (SLPDSocket*) malloc(sizeof(SLPDSocket));
        memset(connsock,0,sizeof(SLPDSocket));
        
        connsock->peerinfo.peeraddrlen = sizeof(sock->peerinfo.peeraddr);
        connsock->fd = accept(sock->fd,
                              &(connsock->peerinfo.peeraddr), 
                              &(connsock->peerinfo  .peeraddrlen));
        if(connsock->fd >= 0)
        {
            setsockopt(connsock->fd,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
            setsockopt(connsock->fd,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat)); 
            connsock->peerinfo.peertype = SLPD_PEER_ACCEPTED;
            connsock->recvbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
            connsock->sendbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
            connsock->state = STREAM_FIRST_READ;
            sock->age = 0;
            SLPDSocketListAdd(list,connsock);
        }
        else
        {
            free(connsock);
        }
    }
}


/*-------------------------------------------------------------------------*/
void HandleDatagramRead(SLPDSocketList* list, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    int                 bytesread;
    int                 bytestowrite;
    
    bytesread = recvfrom(sock->fd,
                         sock->recvbuf->start,
                         SLP_MAX_DATAGRAM_SIZE,
                         0,
                         &(sock->peerinfo.peeraddr),
                         &(sock->peerinfo.peeraddrlen));
    if(bytesread > 0)
    {
        sock->recvbuf->end = sock->recvbuf->start + bytesread;

        if(SLPDProcessMessage(&(sock->peerinfo),
                              sock->recvbuf,
                              sock->sendbuf) == 0)
        {
            /* check to see if we should send anything */
            bytestowrite = sock->sendbuf->end - sock->sendbuf->start;
            if(bytestowrite > 0)
            {
                sendto(sock->fd,
                       sock->sendbuf->start,
                       sock->sendbuf->end - sock->sendbuf->start,
                       0,
                       &(sock->peerinfo.peeraddr),
                       sock->peerinfo.peeraddrlen);
            }
        }
        else
        {
            SLPLog("An error occured while processing message from %s\n",
                   inet_ntoa(sock->peerinfo.peeraddr.sin_addr));
        } 
    }

}


/*-------------------------------------------------------------------------*/
void HandleStreamRead(SLPDSocketList* list, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    int     fdflags;
    int     bytesread;
    char    peek[16];
    
    if(sock->state == STREAM_FIRST_READ)
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
                             &(sock->peerinfo.peeraddr),
                             &(sock->peerinfo.peeraddrlen));
        if(bytesread > 0)
        {
            /* check the version */
            if(*peek == 2)
            {
                /* allocate the recvbuf big enough for the whole message */
                sock->recvbuf = SLPBufferRealloc(sock->recvbuf,AsUINT24(peek+2));
                if(sock->recvbuf)
                {
                    sock->state = STREAM_READ; 
                }
                else
                {
                    SLPLog("Slpd is out of memory!\n");
                    sock->state = SOCKET_CLOSE;
                }
            }
            else
            {
                SLPLog("Unsupported version %i received from %s\n",
                       *peek,
                       inet_ntoa(sock->peerinfo.peeraddr.sin_addr));

                sock->state = SOCKET_CLOSE;
            }
        }
        else
        {
            if(errno != EWOULDBLOCK)
            {
                sock->state = SOCKET_CLOSE;
                return;
            }
        }        
    }
        
    if(sock->state == STREAM_READ)
    {
        /*------------------------------*/
        /* recv the rest of the message */
        /*------------------------------*/
        bytesread = recv(sock->fd,
                         sock->recvbuf->curpos,
                         sock->recvbuf->end - sock->recvbuf->curpos,
                         0);              
    
        if(bytesread > 0)
        {
            /* reset age to max because of activity */
            sock->age = 0;
            sock->recvbuf->curpos += bytesread;
            if(sock->recvbuf->curpos == sock->recvbuf->end)
            {
                if(SLPDProcessMessage(&sock->peerinfo,
                                      sock->recvbuf,
                                      sock->sendbuf) == 0)
                {
                    sock->state = STREAM_FIRST_WRITE;
                }
                else
                {
                    /* An error has occured in SLPDProcessMessage() */
                    SLPLog("An error while processing message from %s\n",
                           inet_ntoa(sock->peerinfo.peeraddr.sin_addr));
                    sock->state = SOCKET_CLOSE;
                }                                                          
            }
        }
        else
        {
            if(errno != EWOULDBLOCK)
            {
                /* error in recv() */
                sock->state = SOCKET_CLOSE;
            }
        }
    }
}


/*-------------------------------------------------------------------------*/
void HandleStreamWrite(SLPDSocketList* list, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    int byteswritten, flags = 0;
    
#if defined(MSG_DONTWAIT)
    flags = MSG_DONTWAIT;
#endif

    if(sock->state == STREAM_FIRST_WRITE)
    {
        /* make sure that the start and curpos pointers are the same */
        sock->sendbuf->curpos = sock->sendbuf->start;
        sock->state = STREAM_WRITE;
    }

    if(sock->sendbuf->end - sock->sendbuf->start != 0)
    {
        byteswritten = send(sock->fd,
                            sock->sendbuf->curpos,
                            sock->sendbuf->end - sock->sendbuf->start,
                            flags);
        if(byteswritten > 0)
        {
            /* reset lifetime to max because of activity */
            sock->age = 0;
            sock->sendbuf->curpos += byteswritten;
            if(sock->sendbuf->curpos == sock->sendbuf->end)
            {
                /* message is completely sent */
                sock->state = STREAM_FIRST_READ;
             }
        }
        else
        {
            if(errno != EWOULDBLOCK)
            {
                /* Error occured or connection was closed */
                sock->state = SOCKET_CLOSE;
            }   
        }    
    }
}


/*=========================================================================*/
void SLPDIncomingHandler(SLPDSocketList* list, 
                         int* fdcount,
                         fd_set* readfds,
                         fd_set* writefds)
/* Handles all outgoing requests that are pending on the specified file    */
/* discriptors                                                             */
/*                                                                         */
/* list     (IN) list of incoming sockets                                  */
/*                                                                         */
/* fdcount  (IN/OUT) number of file descriptors marked in fd_sets          */
/*                                                                         */
/* readfds  (IN) file descriptors with pending read IO                     */
/*                                                                         */
/* writefds  (IN) file descriptors with pending read IO                    */
/*=========================================================================*/
{
    SLPDSocket* sock;
    sock = list->head;
    while(sock && *fdcount)
    {
        if(FD_ISSET(sock->fd,readfds))
        {
            switch(sock->state)
            {
            
            case SOCKET_LISTEN:
                HandleSocketListen(list,sock);
                break;

            case DATAGRAM_UNICAST:
            case DATAGRAM_MULTICAST:
            case DATAGRAM_BROADCAST:
                HandleDatagramRead(list,sock);
                break;                      
        
            case STREAM_READ:
            case STREAM_FIRST_READ:
                HandleStreamRead(list,sock);
                break;

            default:
                break;
            }

            *fdcount = *fdcount - 1;
        }
        else if(FD_ISSET(sock->fd,writefds))
        {
            switch(sock->state)
            {
            
            case STREAM_CONNECT:
                sock->age = 0;
                sock->state = STREAM_FIRST_WRITE;
               
            case STREAM_WRITE:
            case STREAM_FIRST_WRITE:
                HandleStreamWrite(list,sock);
                break;
                
            default:
                break;
            }
            
            *fdcount = *fdcount - 1;
        } 

        sock = (SLPDSocket*)sock->listitem.next; 
    }                               
}


/*=========================================================================*/
int SLPDIncomingInit(SLPDSocketList* list)
/* Initialize incoming socket list to have appropriate sockets for all     */
/* network interfaces                                                      */
/*                                                                         */
/* list     (IN/OUT) pointer to a socket list to be filled with sockets    */
/*                                                                         */
/* Returns  Zero on success non-zero on error                              */
/*=========================================================================*/
{
    char*           begin;
    char*           end;
    int             finished;
    struct in_addr  myaddr;
    struct in_addr  mcastaddr;
    struct in_addr  bcastaddr;
    struct in_addr  loaddr;
    SLPDSocket*     sock;

    /*-----------------------------------------------*/
    /* set up address to use for multicast/broadcast */
    /*-----------------------------------------------*/
    mcastaddr.s_addr = htonl(SLP_MCAST_ADDRESS);
    loaddr.s_addr = htonl(LOOPBACK_ADDRESS);
    bcastaddr.s_addr = htonl(SLP_BCAST_ADDRESS);
 

    /*-----------------------------------------------------------------*/
    /* Create SOCKET_LISTEN socket for LOOPBACK for the library to talk to*/
    /*-----------------------------------------------------------------*/
    sock = SLPDSocketCreateListen(&loaddr);
    if(sock)
    {
        SLPDSocketListAdd(list,sock);
        SLPLog("Listening on loopback...\n");
    }
    else
    {
        SLPLog("ERROR: Could not listen on loopback\n");
        SLPLog("ERROR: No SLPLIB support will be available\n");
    }
                                                                                       
    /*---------------------------------------------------------------------*/
    /* Create sockets for all of the interfaces in the interfaces property */
    /*---------------------------------------------------------------------*/
    begin = (char*)G_SlpdProperty.interfaces;
    end = begin;
    finished = 0;
    while( finished == 0)
    {
        while(*end && *end != ',') end ++;
        if(*end == 0) finished = 1;
        while(*end <=0x2f) 
        {
            *end = 0;
            end--;
        }

        /* begin now points to a null terminated ip address string */
        myaddr.s_addr = inet_addr(begin);

        /*------------------------------------------------*/
        /* Create TCP_LISTEN that will handle unicast TCP */
        /*------------------------------------------------*/
        sock =  SLPDSocketCreateListen(&myaddr);
        if(sock)
        {
            SLPDSocketListAdd(list,sock);
            SLPLog("Listening on %s ...\n",inet_ntoa(myaddr));
        }
        
        
        /*--------------------------------------------------------*/
        /* Create socket that will handle multicast UDP           */
        /*--------------------------------------------------------*/
        sock =  SLPDSocketCreateDatagram(&myaddr,&mcastaddr,DATAGRAM_MULTICAST);
        if(sock)
        {
            SLPDSocketListAdd(list,sock);
            SLPLog("Multicast socket on %s ready\n",inet_ntoa(myaddr));
        }
        

        /*--------------------------------------------*/
        /* Create socket that will handle unicast UDP */
        /*--------------------------------------------*/
        sock =  SLPDSocketCreateDatagram(&myaddr, &myaddr, DATAGRAM_UNICAST);
        if(sock)
        {
            SLPDSocketListAdd(list,sock);
            SLPLog("Unicast socket on %s ready\n",inet_ntoa(myaddr));
        }
        

        begin = end + 1;
    }     

    /*--------------------------------------------------------*/
    /* Create socket that will handle broadcast UDP           */
    /*--------------------------------------------------------*/
    
    /* TODO: Is this how we should be creating broadcast? */
    sock =  SLPDSocketCreateDatagram(&myaddr,&bcastaddr,DATAGRAM_BROADCAST);
    if(sock)
    {
        SLPDSocketListAdd(list,sock);
        SLPLog("Broadcast socket for %s ready\n", inet_ntoa(bcastaddr));
    }

    return 0;
}



