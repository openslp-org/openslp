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
#include "slpd_incoming.h"
#include "slpd_socket.h"
#include "slpd_process.h"
#include "slpd_property.h"
#include "slpd_log.h"


/*=========================================================================*/
/* common code includes                                                    */
/*=========================================================================*/
#include "slp_xmalloc.h"
#include "slp_message.h"
#include "slp_net.h"


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
    int                 peeraddrlen = sizeof(struct sockaddr_storage);
    char                addr_str[INET6_ADDRSTRLEN];

    bytesread = recvfrom(sock->fd,
                         sock->recvbuf->start,
                         SLP_MAX_DATAGRAM_SIZE,
                         0,
                         (struct sockaddr*) &(sock->peeraddr),
                         &peeraddrlen);
    if (bytesread > 0)
    {
        sock->recvbuf->end = sock->recvbuf->start + bytesread;

        switch (SLPDProcessMessage(&sock->peeraddr,
                                   sock->recvbuf,
                                   &(sock->sendbuf)))
        {
        case SLP_ERROR_PARSE_ERROR:
        case SLP_ERROR_VER_NOT_SUPPORTED:
        case SLP_ERROR_MESSAGE_NOT_SUPPORTED:
            break;                    
        default:
            /* check to see if we should send anything */
            bytestowrite = sock->sendbuf->end - sock->sendbuf->start;
            if (bytestowrite > 0)
            {
                byteswritten = sendto(sock->fd, 
                                      sock->sendbuf->start,
                                      bytestowrite,
                                      0,
                                      (struct sockaddr*) &(sock->peeraddr),
                                      sizeof(struct sockaddr_storage));
                if (byteswritten != bytestowrite)
                {
                    SLPDLog("NETWORK_ERROR - %d replying %s\n",
                            errno,
                            SLPNetSockAddrStorageToString(&(sock->peeraddr), addr_str, sizeof(addr_str)));
                }
            }
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
        }
        else
        {
#ifdef _WIN32
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
    int     bytesread, recvlen = 0;
    char    peek[16];
    int     peeraddrlen = sizeof(struct sockaddr_storage);

    if (sock->state == STREAM_READ_FIRST)
    {
        /*---------------------------------------------------*/
        /* take a peek at the packet to get size information */
        /*---------------------------------------------------*/
        bytesread = recvfrom(sock->fd,
                             peek,
                             16,
                             MSG_PEEK,
                             (struct sockaddr*) &(sock->peeraddr),
                             &peeraddrlen);
        if (bytesread > 0)
        {

            if (*peek == 2)
                recvlen = AsUINT24(peek + 2);
            else if (*peek == 1) /* SLPv1 packet */
                recvlen = AsUINT16(peek + 2);
            /* allocate the recvbuf big enough for the whole message */
            sock->recvbuf = SLPBufferRealloc(sock->recvbuf,recvlen);
            if (sock->recvbuf)
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
                switch (SLPDProcessMessage(&sock->peeraddr,
                                           sock->recvbuf,
                                           &(sock->sendbuf)))
                {
                case SLP_ERROR_PARSE_ERROR:
                case SLP_ERROR_VER_NOT_SUPPORTED:
                case SLP_ERROR_MESSAGE_NOT_SUPPORTED:
                    sock->state = SOCKET_CLOSE;
                    break;                    
                default:
                    sock->state = STREAM_WRITE_FIRST;
                    IncomingStreamWrite(socklist, sock);
                }
            }
        }
        else
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
    int                     fdflags;
    sockfd_t                fd;
    SLPDSocket*             connsock;
    struct sockaddr_storage peeraddr;
    socklen_t               peeraddrlen;
#ifdef _WIN32
    const char              lowat = SLPD_SMALLEST_MESSAGE;
#else    
    const int               lowat = SLPD_SMALLEST_MESSAGE;
#endif


    /* Only accept if we can. If we still maximum number of sockets, just*/
    /* ignore the connection */
    if (socklist->count < SLPD_MAX_SOCKETS)
    {
        peeraddrlen = sizeof(peeraddr);
        fd = accept(sock->fd,
                    (struct sockaddr*) &peeraddr, 
                    &peeraddrlen);
        if (fd >= 0)
        {
            connsock = SLPDSocketAlloc();
            if (connsock)
            {
                /* setup the accepted socket */
                connsock->fd        = fd;
                connsock->peeraddr  = peeraddr;
                connsock->state     = STREAM_READ_FIRST;

                /* Set the low water mark on the accepted socket */
                setsockopt(connsock->fd,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
                setsockopt(connsock->fd,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat)); 
                /* set accepted socket to non blocking */
#ifdef _WIN32
                fdflags = 1;
                ioctlsocket(connsock->fd, FIONBIO, &fdflags);
#else
                fdflags = fcntl(connsock->fd, F_GETFL, 0);
                fcntl(connsock->fd,F_SETFL, fdflags | O_NONBLOCK);
#endif        
                SLPListLinkHead(socklist,(SLPListItem*)connsock);
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
        }
        else if (FD_ISSET(sock->fd,writefds))
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
            if (G_IncomingSocketList.count > SLPD_COMFORT_SOCKETS)
            {
                /* Accellerate ageing cause we are low on sockets */
                if (sock->age > SLPD_CONFIG_BUSY_CLOSE_CONN)
                {
                    del = sock;
                }
            }
            else
            {
                if (sock->age > SLPD_CONFIG_CLOSE_CONN)
                {
                    del = sock;
                }
            }
            sock->age = sock->age + seconds;
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
    char*                   begin = NULL;
    char*                   beginSave = NULL;
    char*                   end = NULL;
    int                     finished;
    struct sockaddr_storage myaddr;
    struct sockaddr_storage mcastaddr;
    struct sockaddr_storage bcastaddr;
    struct sockaddr_storage loaddr;
    //// need these addresses for ipv6 too
    SLPDSocket*             sock;
    char                    addr_str[INET6_ADDRSTRLEN];

    /*------------------------------------------------------------*/
    /* First, remove all of the sockets that might be in the list */
    /*------------------------------------------------------------*/
    while (G_IncomingSocketList.count)
    {
        SLPDSocketFree((SLPDSocket*)SLPListUnlink(&G_IncomingSocketList,G_IncomingSocketList.head));
    }


    /*--------------------------------------------------*/
    /* set up address to use for loopback and broadcast */
    /*--------------------------------------------------*/
    //// need to do this for ipv6 too
    loaddr.ss_family = AF_INET;
    ((struct sockaddr_in*) &loaddr)->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    bcastaddr.ss_family = AF_INET;
    ((struct sockaddr_in*) &bcastaddr)->sin_addr.s_addr = htonl(INADDR_BROADCAST);
    mcastaddr.ss_family = AF_INET;
    ((struct sockaddr_in*) &mcastaddr)->sin_addr.s_addr = htonl(SLP_MCAST_ADDRESS);

    /*--------------------------------------------------------------------*/
    /* Create SOCKET_LISTEN socket for LOOPBACK for the library to talk to*/
    /*--------------------------------------------------------------------*/
    //// needs to be done for ipv6 too
    sock = SLPDSocketCreateListen(&loaddr);
    if (sock)
    {
        SLPListLinkTail(&G_IncomingSocketList,(SLPListItem*)sock);
        SLPDLog("Listening on loopback...\n");
    }
    else
    {
        SLPDLog("NETWORK_ERROR - Could not listen on loopback\n");
        SLPDLog("INTERNAL_ERROR - No SLPLIB support will be available\n");
    }

    /*---------------------------------------------------------------------*/
    /* Create sockets for all of the interfaces in the interfaces property */
    /*---------------------------------------------------------------------*/

    /*---------------------------------------------------------------------*/
    /* Copy G_SlpdProperty.interfaces to a temporary buffer to parse the   */
    /*   string in a safety way                                            */
    /*---------------------------------------------------------------------*/

    if (G_SlpdProperty.interfaces != NULL)
    {
        begin = xstrdup((char *) G_SlpdProperty.interfaces);
        beginSave = begin;  /* save pointer for free() operation later */
        end = begin;
        finished = 0;
    }
    else
    {
        finished = 1; /* if no interface is defined,       */
                      /* don't even enter the parsing loop */
    }

    for (; (finished == 0); begin = ++end)
    {
        while (*end && *end != ',') end ++;
        if (*end == 0) finished = 1;
        *end = 0;                      /* Terminate string. */
        if (end <= begin) continue;    /* Skip empty entries */

        /* begin now points to a null terminated ip address string */
        //// need to address the ipv6 case here
        myaddr.ss_family = AF_INET;
        ((struct sockaddr_in*) &myaddr)->sin_addr.s_addr = inet_addr(begin);

        /*------------------------------------------------*/
        /* Create TCP_LISTEN that will handle unicast TCP */
        /*------------------------------------------------*/
        sock =  SLPDSocketCreateListen(&myaddr);
        if (sock)
        {
            SLPListLinkTail(&G_IncomingSocketList,(SLPListItem*)sock);
            SLPDLog("Listening on %s ...\n",SLPNetSockAddrStorageToString(&myaddr, addr_str, sizeof(addr_str)));
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
            SLPDLog("Multicast socket on %s ready\n",SLPNetSockAddrStorageToString(&myaddr, addr_str, sizeof(addr_str)));
        }
        else
        {
            SLPDLog("Couldn't bind to multicast for interface %s (%s)\n",
                    SLPNetSockAddrStorageToString(&myaddr, addr_str, sizeof(addr_str)), strerror(errno));
        }

#if defined(ENABLE_SLPv1)
        if (G_SlpdProperty.isDA)
        {
            /*------------------------------------------------------------*/
            /* Create socket that will handle multicast UDP for SLPv1 DA  */
            /* Discovery.                                                 */
            /*------------------------------------------------------------*/
            //// need to do this for ipv6 as well
            mcastaddr.ss_family = AF_INET;
            ((struct sockaddr_in*) &mcastaddr)->sin_addr.s_addr = htonl(SLPv1_DA_MCAST_ADDRESS);
            sock =  SLPDSocketCreateBoundDatagram(&myaddr,
                                                  &mcastaddr,
                                                  DATAGRAM_MULTICAST);
            if (sock)
            {
                SLPListLinkTail(&G_IncomingSocketList,(SLPListItem*)sock);
                SLPDLog("SLPv1 DA Discovery Multicast socket on %s ready\n",
                        SLPNetSockAddrStorageToString(&myaddr, addr_str, sizeof(addr_str)));
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
            SLPDLog("Unicast socket on %s ready\n",SLPNetSockAddrStorageToString(&myaddr, addr_str, sizeof(addr_str)));
        }
    }     

    if (beginSave) xfree(beginSave);


    /*--------------------------------------------------------*/
    /* Create socket that will handle broadcast UDP           */
    /*--------------------------------------------------------*/
    sock =  SLPDSocketCreateBoundDatagram(&myaddr,
                                          &bcastaddr,
                                          DATAGRAM_BROADCAST);
    if (sock)
    {
        SLPListLinkTail(&G_IncomingSocketList,(SLPListItem*)sock);
        SLPDLog("Broadcast socket for %s ready\n", SLPNetSockAddrStorageToString(&bcastaddr, addr_str, sizeof(addr_str)));
    }

    if (G_IncomingSocketList.count == 0)
    {
        SLPDLog("No usable interfaces\n");
        return 1;
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


#ifdef DEBUG
/*=========================================================================*/
void SLPDIncomingSocketDump()
/*=========================================================================*/
{

}
#endif


