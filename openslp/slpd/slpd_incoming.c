/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_incoming.c                                            */
/*                                                                         */
/* Abstract:    Handles "incoming" network conversations requests made by  */
/*              other agents to slpd. (slpd_outgoing.c handles reqests     */
/*              made by slpd to other agents)                              */
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


/*=========================================================================*/
SLPList G_IncomingSocketList = {0,0,0};
/*=========================================================================*/


/*-------------------------------------------------------------------------*/
void IncomingDatagramRead(SLPList* socklist, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    int                 bytesread;
    int                 bytestowrite;
    int                 byteswritten;
	int                 err;

    bytesread = recvfrom(sock->fd,
                         sock->recvbuf->start,
                         SLP_MAX_DATAGRAM_SIZE,
                         0,
                         (struct sockaddr *) &(sock->peerinfo.peeraddr),
                         &(sock->peerinfo.peeraddrlen));
    if (bytesread > 0)
    {
        sock->recvbuf->end = sock->recvbuf->start + bytesread;

        if ((err = SLPDProcessMessage(&(sock->peerinfo),
									 sock->recvbuf,
									 &(sock->sendbuf))) == 0)
        {
            /* check to see if we should send anything */
            bytestowrite = sock->sendbuf->end - sock->sendbuf->start;
            if (bytestowrite > 0)
            {
                byteswritten = sendto(sock->fd,
				      sock->sendbuf->start,
				      bytestowrite,
				      0,
				      (struct sockaddr *)
				      &(sock->peerinfo.peeraddr),
				      sock->peerinfo.peeraddrlen);
		if (byteswritten != bytestowrite)
		{
		    SLPLog("An error occured while replying to a message "
			   "from %s\n",
			   inet_ntoa(sock->peerinfo.peeraddr.sin_addr));
		}
            }
        } else
        {
            SLPLog("An error (%d) occured while processing message from %s\n",
                   err, inet_ntoa(sock->peerinfo.peeraddr.sin_addr));
        } 
    }
}


/*-------------------------------------------------------------------------*/
void IncomingStreamWrite(SLPList* socklist, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    int byteswritten, flags = 0;

#if defined(MSG_DONTWAIT)
    flags = MSG_DONTWAIT;
#endif

    if (sock->state == STREAM_WRITE_FIRST)
    {
        /* make sure that the start and curpos pointers are the same */
        sock->sendbuf->curpos = sock->sendbuf->start;
        sock->state = STREAM_WRITE;
    }

    if (sock->sendbuf->end - sock->sendbuf->start != 0)
    {
        byteswritten = send(sock->fd,
                            sock->sendbuf->curpos,
                            sock->sendbuf->end - sock->sendbuf->start,
                            flags);
        if (byteswritten > 0)
        {
            /* reset lifetime to max because of activity */
            sock->age = 0;
            sock->sendbuf->curpos += byteswritten;
            if (sock->sendbuf->curpos == sock->sendbuf->end)
            {
                /* message is completely sent */
                sock->state = STREAM_READ_FIRST;
            }
        } else
        {
#ifdef WIN32
            if (WSAEWOULDBLOCK == WSAGetLastError())
#else
            if (errno == EWOULDBLOCK)
#endif
            {
                /* Error occured or connection was closed */
                sock->state = SOCKET_CLOSE;
            }
        }    
    }
}


/*-------------------------------------------------------------------------*/
void IncomingStreamRead(SLPList* socklist, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    int     bytesread;
    char    peek[16];

    if (sock->state == STREAM_READ_FIRST)
    {
        /*---------------------------------------------------------------*/
        /* take a peek at the packet to get version and size information */
        /*---------------------------------------------------------------*/
        bytesread = recvfrom(sock->fd,
                             peek,
                             16,
                             MSG_PEEK,
                             (struct sockaddr *)&(sock->peerinfo.peeraddr),
                             &(sock->peerinfo.peeraddrlen));
        if (bytesread > 0)
        {

            /* check the version */
#if defined(ENABLE_SLPv1)
            if (*peek == 2 || (G_SlpdProperty.isDA && *peek == 1))
#else
            if (*peek == 2)
#endif
            {
                /* allocate the recvbuf big enough for the whole message */
                sock->recvbuf = SLPBufferRealloc(sock->recvbuf,AsUINT24(peek+2));
                if (sock->recvbuf)
                {
                    sock->state = STREAM_READ; 
                } else
                {
                    SLPLog("Slpd is out of memory!\n");
                    sock->state = SOCKET_CLOSE;
                }
            } else
            {
                SLPLog("Unsupported version %i received from %s\n",
                       *peek,
                       inet_ntoa(sock->peerinfo.peeraddr.sin_addr));

                sock->state = SOCKET_CLOSE;
            }
        } else
        {
            sock->state = SOCKET_CLOSE;
            return;
        }        
    }

    if (sock->state == STREAM_READ)
    {
        /*------------------------------*/
        /* recv the rest of the message */
        /*------------------------------*/
        bytesread = recv(sock->fd,
                         sock->recvbuf->curpos,
                         sock->recvbuf->end - sock->recvbuf->curpos,
                         0);              

        if (bytesread > 0)
        {
            /* reset age to max because of activity */
            sock->age = 0;
            sock->recvbuf->curpos += bytesread;
            if (sock->recvbuf->curpos == sock->recvbuf->end)
            {
                if (SLPDProcessMessage(&sock->peerinfo,
                                       sock->recvbuf,
                                       &(sock->sendbuf)) == 0)
                {
                    sock->state = STREAM_WRITE_FIRST;
                    IncomingStreamWrite(socklist, sock);
                } else
                {
                    /* An error has occured in SLPDProcessMessage() */
                    SLPLog("An error while processing message from %s\n",
                           inet_ntoa(sock->peerinfo.peeraddr.sin_addr));
                    sock->state = SOCKET_CLOSE;
                }                                                          
            }
        } else
        {
            /* error in recv() */
            sock->state = SOCKET_CLOSE;
        }
    }
}


/*-------------------------------------------------------------------------*/
void IncomingSocketListen(SLPList* socklist, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    int                 fdflags;
    sockfd_t            fd;
    SLPDSocket*         connsock;
    struct sockaddr_in  peeraddr;
    socklen_t           peeraddrlen;
#ifdef WIN32
    const char   lowat = SLPD_SMALLEST_MESSAGE;
#else    
    const int   lowat = SLPD_SMALLEST_MESSAGE;
#endif


    /* Only accept if we can. If we still maximum number of sockets, just*/
    /* ignore the connection */
    if (socklist->count < SLPD_MAX_SOCKETS)
    {
        peeraddrlen = sizeof(peeraddr);
        fd = accept(sock->fd,
                    (struct sockaddr *) &peeraddr, 
                    &peeraddrlen);
        if (fd >= 0)
        {
            connsock = SLPDSocketAlloc();
            if (connsock)
            {
                /* setup the accepted socket */
                connsock->fd                    = fd;
	        connsock->peerinfo.peeraddrlen  = peeraddrlen;
                connsock->peerinfo.peeraddr     = peeraddr;
                connsock->peerinfo.peertype     = SLPD_PEER_ACCEPTED;
                connsock->state                 = STREAM_READ_FIRST;

                /* Set the low water mark on the accepted socket */
                setsockopt(connsock->fd,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
                setsockopt(connsock->fd,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat)); 
                /* set accepted socket to non blocking */
#ifdef WIN32
                fdflags = 1;
                ioctlsocket(connsock->fd, FIONBIO, &fdflags);
#else
                fdflags = fcntl(connsock->fd, F_GETFL, 0);
                fcntl(connsock->fd,F_SETFL, fdflags | O_NONBLOCK);
#endif        
                SLPListLinkHead(socklist,(SLPListItem*)connsock);
                IncomingStreamRead(socklist, sock);
            }
        }
    }
}


/*=========================================================================*/
void SLPDIncomingHandler(int* fdcount,
                         fd_set* readfds,
                         fd_set* writefds)
/* Handles all outgoing requests that are pending on the specified file    */
/* discriptors                                                             */
/*                                                                         */
/* fdcount  (IN/OUT) number of file descriptors marked in fd_sets          */
/*                                                                         */
/* readfds  (IN) file descriptors with pending read IO                     */
/*                                                                         */
/* writefds  (IN) file descriptors with pending read IO                    */
/*=========================================================================*/
{
    SLPDSocket* sock;
    sock = (SLPDSocket*) G_IncomingSocketList.head;
    while (sock && *fdcount)
    {
        if (FD_ISSET(sock->fd,readfds))
        {
            switch (sock->state)
            {
            case SOCKET_LISTEN:
                IncomingSocketListen(&G_IncomingSocketList,sock);
                break;

            case DATAGRAM_UNICAST:
            case DATAGRAM_MULTICAST:
            case DATAGRAM_BROADCAST:
                IncomingDatagramRead(&G_IncomingSocketList,sock);
                break;                      

            case STREAM_READ:
            case STREAM_READ_FIRST:
                IncomingStreamRead(&G_IncomingSocketList,sock);
                break;

            default:
                break;
            }

            *fdcount = *fdcount - 1;
        } else if (FD_ISSET(sock->fd,writefds))
        {
            switch (sock->state)
            {
            case STREAM_WRITE:
            case STREAM_WRITE_FIRST:
                IncomingStreamWrite(&G_IncomingSocketList,sock);
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
void SLPDIncomingAge(time_t seconds)
/*=========================================================================*/
{
    SLPDSocket* del  = 0;
    SLPDSocket* sock = (SLPDSocket*)G_IncomingSocketList.head;
    while (sock)
    {
        switch (sock->state)
        {
        case STREAM_READ_FIRST:
        case STREAM_READ:
        case STREAM_WRITE_FIRST:
        case STREAM_WRITE:
            sock->age = sock->age + seconds;
            if (G_IncomingSocketList.count > SLPD_COMFORT_SOCKETS)
            {
                if (sock->age > G_SlpdProperty.unicastMaximumWait)
                {
                    del = sock;
                }
            } else
            {
                if (sock->age > SLPD_MAX_SOCKET_LIFETIME)
                {
                    del = sock;
                }
            }
            break;

        default:
            /* don't age the other sockets at all */
            break;
        }

        sock = (SLPDSocket*)sock->listitem.next;

        if (del)
        {
            SLPDSocketFree((SLPDSocket*)SLPListUnlink(&G_IncomingSocketList,(SLPListItem*)del));
            del = 0;
        }
    }                                                 
}


/*=========================================================================*/
int SLPDIncomingInit()
/* Initialize incoming socket list to have appropriate sockets for all     */
/* network interfaces                                                      */
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

    /*------------------------------------------------------------*/
    /* First, remove all of the sockets that might be in the list */
    /*------------------------------------------------------------*/
    while (G_IncomingSocketList.count)
    {
        SLPDSocketFree((SLPDSocket*)SLPListUnlink(&G_IncomingSocketList,G_IncomingSocketList.head));
    }


    /*-----------------------------------------------*/
    /* set up address to use for loopback/broadcast  */
    /*-----------------------------------------------*/
    loaddr.s_addr = htonl(LOOPBACK_ADDRESS);
    bcastaddr.s_addr = htonl(SLP_BCAST_ADDRESS);
    mcastaddr.s_addr = htonl(SLP_MCAST_ADDRESS);

    /*--------------------------------------------------------------------*/
    /* Create SOCKET_LISTEN socket for LOOPBACK for the library to talk to*/
    /*--------------------------------------------------------------------*/
    sock = SLPDSocketCreateListen(&loaddr);
    if (sock)
    {
        SLPListLinkTail(&G_IncomingSocketList,(SLPListItem*)sock);
        SLPLog("Listening on loopback...\n");
    } else
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
    while ( finished == 0)
    {
        while (*end && *end != ',') end ++;
        if (*end == 0) finished = 1;
        while (*end <=0x2f)
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
        if (sock)
        {
            SLPListLinkTail(&G_IncomingSocketList,(SLPListItem*)sock);
            SLPLog("Listening on %s ...\n",inet_ntoa(myaddr));
        }


        /*----------------------------------------------------------------*/
        /* Create socket that will handle multicast UDP.                  */
        /*----------------------------------------------------------------*/

        sock =  SLPDSocketCreateBoundDatagram(&myaddr,
                                              &mcastaddr,
                                              DATAGRAM_MULTICAST);
        if (sock)
        {
            SLPListLinkTail(&G_IncomingSocketList,(SLPListItem*)sock);
            SLPLog("Multicast socket on %s ready\n",inet_ntoa(myaddr));
        }


#if defined(ENABLE_SLPv1)
	if (G_SlpdProperty.isDA)
	{
	    /*------------------------------------------------------------*/
	    /* Create socket that will handle multicast UDP for SLPv1 DA  */
	    /* Discovery.                                                 */
	    /*------------------------------------------------------------*/
	    mcastaddr.s_addr = htonl(SLPv1_DA_MCAST_ADDRESS);
	    sock =  SLPDSocketCreateBoundDatagram(&myaddr,
						  &mcastaddr,
						  DATAGRAM_MULTICAST);
	    if (sock)
	    {
		SLPListLinkTail(&G_IncomingSocketList,(SLPListItem*)sock);
		SLPLog("SLPv1 DA Discovery Multicast socket on %s ready\n",
		       inet_ntoa(myaddr));
	    }
	}
#endif

        /*--------------------------------------------*/
        /* Create socket that will handle unicast UDP */
        /*--------------------------------------------*/
        sock =  SLPDSocketCreateBoundDatagram(&myaddr,
                                              &myaddr,
                                              DATAGRAM_UNICAST);
        if (sock)
        {
            SLPListLinkTail(&G_IncomingSocketList,(SLPListItem*)sock);
            SLPLog("Unicast socket on %s ready\n",inet_ntoa(myaddr));
        }

        begin = end + 1;
    }     

    /*--------------------------------------------------------*/
    /* Create socket that will handle broadcast UDP           */
    /*--------------------------------------------------------*/
    sock =  SLPDSocketCreateBoundDatagram(&myaddr,
                                          &bcastaddr,
                                          DATAGRAM_BROADCAST);
    if (sock)
    {
        SLPListLinkTail(&G_IncomingSocketList,(SLPListItem*)sock);
        SLPLog("Broadcast socket for %s ready\n", inet_ntoa(bcastaddr));
    }

    return 0;
}

/*=========================================================================*/
int SLPDIncomingDeinit()
/* Deinitialize incoming socket list to have appropriate sockets for all   */
/* network interfaces                                                      */
/*                                                                         */
/* Returns  Zero on success non-zero on error                              */
/*=========================================================================*/
{
    SLPDSocket* del  = 0;
    SLPDSocket* sock = (SLPDSocket*)G_IncomingSocketList.head;
    while (sock)
    {
        del = sock;
        sock = (SLPDSocket*)sock->listitem.next;
        if (del)
        {
            SLPDSocketFree((SLPDSocket*)SLPListUnlink(&G_IncomingSocketList,(SLPListItem*)del));
            del = 0;
        }
    } 

    return 0;
}


