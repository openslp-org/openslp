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
SLPDSocketList G_OutgoingSocketList = {0,0};
/*=========================================================================*/


/*-------------------------------------------------------------------------*/
void OutgoingStreamReconnect(SLPDSocketList* list, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    if(connect(sock->fd, 
               &(sock->peerinfo.peeraddr), 
               sizeof(sock->peerinfo.peeraddr)) == 0)   
    {
        /* Connection occured immediately */
        sock->state = STREAM_WRITE_FIRST;
    }
    else
    {
        if(errno == EINPROGRESS)
        {
            /* Connect would have blocked */
            sock->state = STREAM_CONNECT_BLOCK;
        }
        else
        {
            sock->state = SOCKET_CLOSE;
        }                                
    } 
}


/*-------------------------------------------------------------------------*/
void OutgoingDatagramWrite(SLPDSocketList* list, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    size_t bytestowrite = sock->sendbuf->end - sock->sendbuf->start;
    if(bytestowrite > 0)
    {
        sendto(sock->fd,
               sock->sendbuf->start,
               bytestowrite,
               0,
               &(sock->peerinfo.peeraddr),
               sock->peerinfo.peeraddrlen);
    }

    sock->state = SOCKET_CLOSE;
}


/*-------------------------------------------------------------------------*/
void OutgoingStreamWrite(SLPDSocketList* list, SLPDSocket* sock)
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
        sock->sendbuf = sock->sendbufhead;
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
            }
        }
        else
        {
            if (errno != EWOULDBLOCK)
            {
                /* Error occured or connection was closed. Try to reconnect */
                /* Socket will be closed if connect times out               */
                OutgoingStreamReconnect(list,sock);
            }
        }    
    }
}


/*-------------------------------------------------------------------------*/
void OutgoingStreamRead(SLPDSocketList* list, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    int     fdflags;
    int     bytesread;
    char    peek[16];

    if (sock->state == STREAM_READ_FIRST)
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
        if (bytesread > 0)
        {
            /* check the version */
            if (*peek == 2)
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
                       inet_ntoa(sock->peerinfo.peeraddr.sin_addr));
                sock->state = SOCKET_CLOSE;
            }
        }
        else
        {
            if (errno != EWOULDBLOCK)
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
                switch(SLPDProcessMessage(&(sock->peerinfo),
                                          sock->recvbuf,
                                          sock->sendbuf))
                {
                case SLP_ERROR_DA_BUSY_NOW:
                    sock->state = STREAM_WRITE_WAIT;
                    break;
                
                case SLP_ERROR_PARSE_ERROR:
                case SLP_ERROR_VER_NOT_SUPPORTED:
                    SLPLog("PARSE_ERROR / VER_NOT_SUPPORTED from %s\n",
                           inet_ntoa(sock->peerinfo.peeraddr.sin_addr));
                    sock->state = SOCKET_CLOSE;
                    break;
                
                default:
                    /* End of outgoing message exchange. Unlink   */
                    /* send buf from to do list and free it       */
                    sock->sendbuf = SLPBufferListRemove(&(sock->sendbufhead),sock->sendbuf);
                    sock->state = STREAM_WRITE_FIRST;
                    break;
                }
            }
        }
        else
        {
            if (errno != EWOULDBLOCK)
            {
                /* Error occured or connection was closed. Try to reconnect */
                /* Socket will be closed if connect times out               */
                OutgoingStreamReconnect(list,sock);
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
    sock = G_OutgoingSocketList.head;
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
    SLPDSocket* sock = G_OutgoingSocketList.head;

    while (sock)
    {
        switch (sock->state)
        {
        case DATAGRAM_UNICAST:
        case DATAGRAM_MULTICAST:
        case DATAGRAM_BROADCAST:
            OutgoingDatagramWrite(&G_OutgoingSocketList,sock);
            break;


        case STREAM_CONNECT_BLOCK:
            sock->age = sock->age + 1;
            if (sock->age > G_SlpdProperty.unicastMaximumWait)
            {
                /* TODO: Log that the DA is not accepting connections */

                if (sock->daentry)
                {
                    /* Remove the DA we were talking to from the list because */
                    /* it is not accepting connections                        */
                    SLPDKnownDARemove(sock->daentry);
                }
                sock = SLPDSocketListRemove(&G_OutgoingSocketList,sock);
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
                    sock = SLPDSocketListRemove(&G_OutgoingSocketList,sock);
                }
            }
            else
            {
                if (sock->age > SLPD_MAX_SOCKET_LIFETIME)
                {
                    sock = SLPDSocketListRemove(&G_OutgoingSocketList,sock);
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

        if (sock)
        {
            sock = (SLPDSocket*)sock->listitem.next;
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
    SLPDSocket* sock;

    /*------------------------------------------------------------*/
    /* First, remove all of the sockets that might be in the list */
    /*------------------------------------------------------------*/
    sock = G_OutgoingSocketList.head;
    while (sock)
    {
        sock = SLPDSocketListRemove(&G_OutgoingSocketList,sock);       
    }

    return 0;
}

