/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_outgoing.c                                            */
/*                                                                         */
/* Abstract:    Handles "outgoing" network conversations requests made by  */
/*              other agents to slpd. (slpd_incoming.c handles reqests     */
/*              made by other agents to slpd)                              */
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
SLPList G_OutgoingSocketList = {0,0,0};
/*=========================================================================*/


/*-------------------------------------------------------------------------*/
void OutgoingStreamReconnect(SLPList* socklist, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    if(connect(sock->fd, 
               (struct sockaddr *)&(sock->peeraddr), 
               sizeof(struct sockaddr_in)) == 0)   
    {
        /* Connection occured immediately*/
        sock->state = STREAM_WRITE_FIRST;
    }
    else
    {
#ifdef WIN32
      if (WSAEWOULDBLOCK == WSAGetLastError())
#else
        if(errno == EINPROGRESS)
#endif
        {
            /* Connect would have blocked */
            sock->state = STREAM_CONNECT_BLOCK;
        }
        else
        {
            /* TODO: if reconnect fails on known DA then we should remove  */
            /*       the know DA                                           */
            if(sock->daentry)
            {
                SLPDKnownDARemove(sock->daentry);
                sock->daentry = 0;
            }
            sock->state = SOCKET_CLOSE;
        }                                
    } 
}

/*-------------------------------------------------------------------------*/
void OutgoingStreamRead(SLPList* socklist, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    int     bytesread;
    char    peek[16];
    int     peeraddrlen = sizeof(struct sockaddr_in);

    if (sock->state == STREAM_READ_FIRST)
    {
        /*---------------------------------------------------------------*/
        /* take a peek at the packet to get version and size information */
        /*---------------------------------------------------------------*/
        bytesread = recvfrom(sock->fd,
                             peek,
                             16,
                             MSG_PEEK,
                             (struct sockaddr *)&(sock->peeraddr),
                             &peeraddrlen);
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
                       inet_ntoa(sock->peeraddr.sin_addr));
                sock->state = SOCKET_CLOSE;
            }
        }
        else
        {
#ifdef WIN32
      if (WSAEWOULDBLOCK == WSAGetLastError())
#else
        if(errno == EWOULDBLOCK)
#endif
            {
                /* TODO: */
                /* Stream was probably closed.  Try to re connect */
                sock->state = SOCKET_CLOSE;
                return;
            }
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
            /* reset age because of activity */
            sock->age = 0;

            /* move buffer pointers */
            sock->recvbuf->curpos += bytesread;

            /* check to see if everything was read */
            if (sock->recvbuf->curpos == sock->recvbuf->end)
            {
                switch(SLPDProcessMessage(&(sock->peeraddr),
                                          sock->recvbuf,
                                          &(sock->sendbuf)))
                {
                case SLP_ERROR_DA_BUSY_NOW:
                    sock->state = STREAM_WRITE_WAIT;
                    break;
                
                case SLP_ERROR_PARSE_ERROR:
                case SLP_ERROR_VER_NOT_SUPPORTED:
                    SLPLog("PARSE_ERROR / VER_NOT_SUPPORTED from %s\n",
                           inet_ntoa(sock->peeraddr.sin_addr));
                    sock->state = SOCKET_CLOSE;
                    break;
                
                default:
                    /* End of outgoing message exchange. Unlink   */
                    /* send buf from to do list and free it       */
                    SLPBufferFree((SLPBuffer)SLPListUnlink(&(sock->sendlist),(SLPListItem*)(sock->sendbuf)));
                    sock->state = STREAM_WRITE_FIRST;
                    break;
                }
            }
        }
        else
        {
#ifdef WIN32
      if (WSAEWOULDBLOCK == WSAGetLastError())
#else
        if(errno == EWOULDBLOCK)
#endif
            {
                /* Error occured or connection was closed. Try to reconnect */
                /* Socket will be closed if connect times out               */
                OutgoingStreamReconnect(socklist,sock);
            }
        }
    }
}


/*-------------------------------------------------------------------------*/
void OutgoingStreamWrite(SLPList* socklist, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    int        byteswritten;
    int        flags = 0;

#if defined(MSG_DONTWAIT)
    flags = MSG_DONTWAIT;
#endif

    if (sock->state == STREAM_WRITE_FIRST)
    {
        /* make sure that there is something to do list */
        sock->sendbuf = (SLPBuffer)sock->sendlist.head;
        if (sock->sendbuf == 0)
        {
            /* there is nothing in the to do list */
            sock->state = STREAM_CONNECT_IDLE;
            return;
        }
        
        /* make sure that the start and curpos pointers are the same */
        sock->sendbuf->curpos = sock->sendbuf->start;
        sock->state = STREAM_WRITE;
    }

    if (sock->sendbuf->end - sock->sendbuf->start > 0)
    {
        byteswritten = send(sock->fd,
                            sock->sendbuf->curpos,
                            sock->sendbuf->end - sock->sendbuf->start,
                            flags);
        if (byteswritten > 0)
        {
            /* reset age because of activity */
            sock->age = 0; 
            
            /* move buffer pointers */
            sock->sendbuf->curpos += byteswritten;

            /* check to see if everything was written */
            if (sock->sendbuf->curpos == sock->sendbuf->end)
            {
                /* Message is completely sent. Set state to read the reply */
                sock->state = STREAM_READ_FIRST;
                OutgoingStreamRead(socklist, sock);
            }
        }
        else
        {
#ifdef WIN32
      if (WSAEWOULDBLOCK == WSAGetLastError())
#else
        if(errno == EWOULDBLOCK)
#endif
            {
                /* Error occured or connection was closed. Try to reconnect */
                /* Socket will be closed if connect times out               */
                OutgoingStreamReconnect(socklist,sock);
            }
        }    
    }
}




/*=========================================================================*/
void SLPDOutgoingHandler(int* fdcount,
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
    sock = (SLPDSocket*)G_OutgoingSocketList.head;
    while (sock && *fdcount)
    {
        if (FD_ISSET(sock->fd,readfds))
        {
            switch (sock->state)
            {
            case STREAM_READ:
            case STREAM_READ_FIRST:
                OutgoingStreamRead(&G_OutgoingSocketList,sock);
                break;

            default:
                /* No SOCKET_LISTEN sockets should exist */
                /* No DATAGRAM_ sockets should exist */
                break;
            }

            *fdcount = *fdcount - 1;
        }
        else if (FD_ISSET(sock->fd,writefds))
        {
            switch (sock->state)
            {
            
            case STREAM_CONNECT_BLOCK:
                sock->age = 0;
                sock->state = STREAM_WRITE_FIRST;

            case STREAM_WRITE:
            case STREAM_WRITE_FIRST:
                OutgoingStreamWrite(&G_OutgoingSocketList,sock);
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
void SLPDOutgoingAge(time_t seconds)
/*=========================================================================*/
{
    SLPDSocket* del  = 0;
    SLPDSocket* sock = (SLPDSocket*)G_OutgoingSocketList.head;

    while (sock)
    {
        switch (sock->state)
        {
        case STREAM_CONNECT_BLOCK:
            sock->age = sock->age + 1;
            if (sock->age > G_SlpdProperty.unicastMaximumWait)
            {
                /* TODO: Log that the DA is not accepting connections */
                if (sock->daentry)
                {
                    /* Remove the DA we were talking to from the list because */
                    /* it is not accepting connections                        */
                    SLPDAEntryFree((SLPDAEntry*)SLPListUnlink(&G_KnownDAList,(SLPListItem*)(sock->daentry)));
                }
                del = sock;
            }
            break;

        case STREAM_READ_FIRST:
        case STREAM_READ:
        case STREAM_WRITE_FIRST:
        case STREAM_WRITE:
            sock->age = sock->age + seconds;
            if (G_OutgoingSocketList.count > SLPD_COMFORT_SOCKETS)
            {
                if (sock->age > G_SlpdProperty.unicastMaximumWait)
                {
                    sock = del;
                }
            }
            else
            {
                if (sock->age > SLPD_MAX_SOCKET_LIFETIME)
                {
                    sock = del;
                }
            }
            break;
        
        case STREAM_WRITE_WAIT:
            sock->age = 0;
            sock->state = STREAM_WRITE_FIRST;
            break;

        default:
            /* don't age the other sockets at all */
            break;
        }

        sock = (SLPDSocket*)sock->listitem.next;

        if (del)
        {
            SLPDSocketFree((SLPDSocket*)SLPListUnlink(&G_OutgoingSocketList,(SLPListItem*)del));
            del = 0;
        }
    }                                                 
}


/*=========================================================================*/
int SLPDOutgoingInit()
/* Initialize outgoing socket list to have appropriate sockets for all     */
/* network interfaces                                                      */
/*                                                                         */
/* list     (IN/OUT) pointer to a socket list to be filled with sockets    */
/*                                                                         */
/* Returns  Zero on success non-zero on error                              */
/*=========================================================================*/
{
    /*------------------------------------------------------------*/
    /* First, remove all of the sockets that might be in the list */
    /*------------------------------------------------------------*/
    while (G_OutgoingSocketList.count)
    {
        SLPDSocketFree((SLPDSocket*)SLPListUnlink(&G_OutgoingSocketList,(SLPListItem*)G_OutgoingSocketList.head));
    }

    return 0;
}


/*=========================================================================*/
int SLPDOutgoingDeinit()
/* Deinitialize incoming socket list to have appropriate sockets for all   */
/* network interfaces                                                      */
/*                                                                         */
/* Returns  Zero on success non-zero on error                              */
/*=========================================================================*/
{
    SLPDSocket* del  = 0;
    SLPDSocket* sock = (SLPDSocket*)G_OutgoingSocketList.head;
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


