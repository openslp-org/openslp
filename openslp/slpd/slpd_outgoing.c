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
/*     Please submit patches to http://www.openslp.org                     */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/* Copyright (C) 2000 Caldera Systems, Inc                                 */
/* All rights reserved.                                                    */
/*                                                                         */
/* Redistribution and use in source and binary forms, with or without      */
/* modification, are permitted provided that the following conditions are  */
/* met:                                                                    */ 
/*                                                                         */
/*      Redistributions of source code must retain the above copyright     */
/*      notice, this list of conditions and the following disclaimer.      */
/*                                                                         */
/*      Redistributions in binary form must reproduce the above copyright  */
/*      notice, this list of conditions and the following disclaimer in    */
/*      the documentation and/or other materials provided with the         */
/*      distribution.                                                      */
/*                                                                         */
/*      Neither the name of Caldera Systems nor the names of its           */
/*      contributors may be used to endorse or promote products derived    */
/*      from this software without specific prior written permission.      */
/*                                                                         */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS     */
/* `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT      */
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   */
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CALDERA      */
/* SYSTEMS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, */
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON       */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    */
/*                                                                         */
/***************************************************************************/


/*=========================================================================*/
/* slpd includes                                                           */
/*=========================================================================*/
#include "slpd_outgoing.h"
#include "slpd_property.h"
#include "slpd_process.h"
#include "slpd_log.h"
#include "slpd_knownda.h"


/*=========================================================================*/
/* common code includes                                                    */
/*=========================================================================*/
#include "../common/slp_message.h"


/*=========================================================================*/
SLPList G_OutgoingSocketList = {0,0,0};
/*=========================================================================*/


/*-------------------------------------------------------------------------*/
void OutgoingDatagramRead(SLPList* socklist, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    int                 bytesread;
    int                 peeraddrlen = sizeof(struct sockaddr_in);

    bytesread = recvfrom(sock->fd,
                         sock->recvbuf->start,
                         SLP_MAX_DATAGRAM_SIZE,
                         0,
                         (struct sockaddr *) &(sock->peeraddr),
                         &peeraddrlen);
    if(bytesread > 0)
    {
        sock->recvbuf->end = sock->recvbuf->start + bytesread;

        SLPDProcessMessage(&(sock->peeraddr),
                           sock->recvbuf,
                           &(sock->sendbuf));
/*       
        SLPDSocketFree((SLPDSocket*)SLPListUnlink(socklist,(SLPListItem*)sock));
*/
    }
}


/*-------------------------------------------------------------------------*/
void OutgoingStreamReconnect(SLPList* socklist, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
#ifdef WIN32
    u_long              fdflags;
#else    
    int                 fdflags;
#endif
    /*----------------------------------------------------------------*/
    /* Make sure we have not reconnected too many times               */
    /*----------------------------------------------------------------*/
    sock->reconns += 1;
    if(sock->reconns > SLPD_CONFIG_MAX_RECONN)
    {
        sock->state = SOCKET_CLOSE;
        return;
    }
    
    /*----------------------------------------------------------------*/
    /* Close the existing socket to clean the stream  and open an new */
    /* socket                                                         */
    /*----------------------------------------------------------------*/
    CloseSocket(sock->fd);
    sock->fd = socket(PF_INET,SOCK_STREAM,0);
    if(sock->fd < 0)
    {
        sock->state = SOCKET_CLOSE;
        return;
    }
    
    /*---------------------------------------------*/
    /* Set the new socket to enable nonblocking IO */
    /*---------------------------------------------*/
#ifdef WIN32
    fdflags = 1;
    ioctlsocket(sock->fd, FIONBIO, &fdflags);
#else
    fdflags = fcntl(sock->fd, F_GETFL, 0);
    fcntl(sock->fd,F_SETFL, fdflags | O_NONBLOCK);
#endif
    
    /*--------------------------*/
    /* Connect a the new socket */
    /*--------------------------*/
    if(connect(sock->fd, 
               (struct sockaddr *)&(sock->peeraddr), 
               sizeof(struct sockaddr_in)))
    {
#ifdef WIN32
        if(WSAEWOULDBLOCK == WSAGetLastError())
#else
        if(errno == EINPROGRESS)
#endif
        {
            /* Connect blocked */
            sock->state = STREAM_CONNECT_BLOCK;
            return;
        }
    }

    /* Connection occured immediately. Set to WRITE_FIRST so whole */
    /* packet will be written                                      */
    sock->state = STREAM_WRITE_FIRST;
}


/*-------------------------------------------------------------------------*/
void OutgoingStreamRead(SLPList* socklist, SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    int     bytesread;
    char    peek[16];
    int     peeraddrlen = sizeof(struct sockaddr_in);

    if(sock->state == STREAM_READ_FIRST)
    {
        /*---------------------------------------------------*/
        /* take a peek at the packet to get size information */
        /*---------------------------------------------------*/
        bytesread = recvfrom(sock->fd,
                             peek,
                             16,
                             MSG_PEEK,
                             (struct sockaddr *)&(sock->peeraddr),
                             &peeraddrlen);
        if(bytesread > 0)
        {
            /* allocate the recvbuf big enough for the whole message */
            sock->recvbuf = SLPBufferRealloc(sock->recvbuf,AsUINT24(peek+2));
            if(sock->recvbuf)
            {
                sock->state = STREAM_READ;
            }
            else
            {
                SLPDLog("INTERNAL_ERROR - out of memory!\n");
                sock->state = SOCKET_CLOSE;
            }
        }
        else
        {
#ifdef WIN32
            if(WSAEWOULDBLOCK != WSAGetLastError())
#else
            if(errno != EWOULDBLOCK)
#endif
            {
                /* Error occured or connection was closed. Try to reconnect */
                /* Socket will be closed if connect times out               */
                OutgoingStreamReconnect(socklist,sock);
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
            /* reset age because of activity */
            sock->age = 0;

            /* move buffer pointers */
            sock->recvbuf->curpos += bytesread;

            /* check to see if everything was read */
            if(sock->recvbuf->curpos == sock->recvbuf->end)
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
            if(WSAEWOULDBLOCK != WSAGetLastError())
#else
            if(errno != EWOULDBLOCK)
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

    if(sock->state == STREAM_WRITE_FIRST)
    {
        /* make sure that there is something to do list */
        sock->sendbuf = (SLPBuffer)sock->sendlist.head;
        if(sock->sendbuf == 0)
        {
            /* there is nothing in the to do list */
            sock->state = STREAM_CONNECT_IDLE;
            /* reset the reconnect count because the socket */
            /* appears to be healthy again                  */
            sock->reconns = 0;
            return;
        }

        /* make sure that the start and curpos pointers are the same */
        sock->sendbuf->curpos = sock->sendbuf->start;
        sock->state = STREAM_WRITE;
    }

    if(sock->sendbuf->end - sock->sendbuf->start > 0)
    {
        byteswritten = send(sock->fd,
                            sock->sendbuf->curpos,
                            sock->sendbuf->end - sock->sendbuf->start,
                            flags);
        if(byteswritten > 0)
        {
            /* reset age because of activity */
            sock->age = 0; 

            /* move buffer pointers */
            sock->sendbuf->curpos += byteswritten;

            /* check to see if everything was written */
            if(sock->sendbuf->curpos == sock->sendbuf->end)
            {
                /* Message is completely sent. Set state to read the reply */
                sock->state = STREAM_READ_FIRST;
            }
        }
        else
        {
#ifdef WIN32
            if(WSAEWOULDBLOCK != WSAGetLastError())
#else
            if(errno != EWOULDBLOCK)
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
SLPDSocket* SLPDOutgoingConnect(struct in_addr* addr)
/* Get a pointer to a connected socket that is associated with the         */
/* outgoing socket list.  If a connected socket already exists on the      */
/* outgoing list, a pointer to it is returned, otherwise a new connection  */
/* is made and added to the outgoing list                                  */
/*                                                                         */
/* addr (IN) the address of the peer a connection is desired for           */
/*                                                                         */
/* returns: pointer to socket or null on error                             */
/*=========================================================================*/
{
    SLPDSocket* sock = (SLPDSocket*)G_OutgoingSocketList.head;
    while(sock)
    {
        if(sock->state == STREAM_CONNECT_IDLE ||
           sock->state > SOCKET_PENDING_IO)
        {
            if(sock->peeraddr.sin_addr.s_addr == addr->s_addr)
            {
                break;
            }
        }
        sock = (SLPDSocket*)sock->listitem.next;    
    }

    if(sock == 0)
    {
        sock = SLPDSocketCreateConnected(addr);
        SLPListLinkTail(&(G_OutgoingSocketList),(SLPListItem*)sock);
    }

    return sock;
}

/*=========================================================================*/
void SLPDOutgoingDatagramWrite(SLPDSocket* sock)
/* Add a ready to write outgoing datagram socket to the outgoing list.     */
/* The datagram will be written then sit in the list until it ages out     */
/* (after  net.slp.unicastMaximumWait)                                     */
/*                                                                         */
/* sock (IN) the socket that will belong on the outgoing list              */
/*=========================================================================*/
{
    if(sendto(sock->fd,
              sock->sendbuf->start,
              sock->sendbuf->end - sock->sendbuf->start,
              0,
              (struct sockaddr *) &(sock->peeraddr),
              sizeof(struct sockaddr_in)) >= 0)
    {
        /* Link the socket into the outgoing list so replies will be */
        /* proccessed                                                */
        SLPListLinkHead(&G_OutgoingSocketList,(SLPListItem*)sock);
    }
    else
    {
        /* Data could not even be sent to peer, do not add to list */
        /* free socket instead                                     */
        SLPDSocketFree(sock);
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
    while(sock && *fdcount)
    {
        if(FD_ISSET(sock->fd,readfds))
        {
            switch(sock->state)
            {
            case DATAGRAM_MULTICAST:
            case DATAGRAM_BROADCAST:
            case DATAGRAM_UNICAST:
                OutgoingDatagramRead(&G_OutgoingSocketList,sock);
                break;

            case STREAM_READ:
            case STREAM_READ_FIRST:
                OutgoingStreamRead(&G_OutgoingSocketList,sock);
                break;

            default:
                /* No SOCKET_LISTEN sockets should exist */
                break;
            }

            *fdcount = *fdcount - 1;
        }
        else if(FD_ISSET(sock->fd,writefds))
        {
            switch(sock->state)
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

    while(sock)
    {
        switch(sock->state)
        {
        case DATAGRAM_MULTICAST:
        case DATAGRAM_BROADCAST:
        case DATAGRAM_UNICAST:
            if(sock->age > G_SlpdProperty.unicastMaximumWait / 1000)
            {
                del = sock;
            }
            sock->age = sock->age + seconds;
            break;

        case STREAM_READ_FIRST:
        case STREAM_WRITE_FIRST:
            sock->age = 0;
            break;
        case STREAM_CONNECT_BLOCK:
        case STREAM_READ:
        case STREAM_WRITE:
            if(G_OutgoingSocketList.count > SLPD_COMFORT_SOCKETS)
            {
                /* Accellerate ageing cause we are low on sockets */
                if(sock->age > SLPD_CONFIG_BUSY_CLOSE_CONN )
                {
                    SLPDKnownDARemove(&(sock->peeraddr.sin_addr));
                    del = sock;
                }
            }
            else
            {
                if(sock->age > SLPD_CONFIG_CLOSE_CONN)
                {
                    SLPDKnownDARemove(&(sock->peeraddr.sin_addr));
                    del = sock;
                }
            }
            sock->age = sock->age + seconds;
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

        if(del)
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
    while(G_OutgoingSocketList.count)
    {
        SLPDSocketFree((SLPDSocket*)SLPListUnlink(&G_OutgoingSocketList,(SLPListItem*)G_OutgoingSocketList.head));
    }

    return 0;
}


/*=========================================================================*/
int SLPDOutgoingDeinit(int graceful)
/* Deinitialize incoming socket list to have appropriate sockets for all   */
/* network interfaces                                                      */
/*                                                                         */
/* graceful (IN) Do not close sockets with pending writes                  */
/*                                                                         */
/* Returns  Zero on success non-zero when pending writes remain            */
/*=========================================================================*/
{
    SLPDSocket* del  = 0;
    SLPDSocket* sock = (SLPDSocket*)G_OutgoingSocketList.head;

    while(sock)
    {
        /* graceful only closes sockets without pending I/O */
        if(graceful == 0)
        {
            del = sock;
        }
        else if(sock->state < SOCKET_PENDING_IO)
        {
            del = sock;
        }

        sock = (SLPDSocket*)sock->listitem.next;

        if(del)
        {
            SLPDSocketFree((SLPDSocket*)SLPListUnlink(&G_OutgoingSocketList,(SLPListItem*)del));
            del = 0;
        }
    }

    return G_OutgoingSocketList.count;
}



#ifdef DEBUG
/*=========================================================================*/
void SLPDOutgoingSocketDump()
/*=========================================================================*/
{

}
#endif

