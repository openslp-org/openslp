/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_network.c                                              */
/*                                                                         */
/* Abstract:    Implementation for functions that are related              */
/*              network (and ipc) communication.                           */
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

#include "slp_network.h"
#include "slp_net.h"
/*=========================================================================*/ 
int SLPNetworkConnectStream(struct sockaddr_storage *peeraddr,   
                            struct timeval* timeout)
/* Connect a TCP stream to the specified peer                              */
/*                                                                         */
/* peeraddr (IN) pointer to the peer to connect to                         */
/*                                                                         */
/* timeout  (IN) pointer to the maximum time to spend connecting           */
/*                                                                         */
/* returns: a connected socket or -1                                       */
/*=========================================================================*/ 
{
#ifdef _WIN32
    char lowat;
#else
    int lowat;
#endif
    int result;

    /* TODO: Make this connect non-blocking so that it will timeout */
    if (SLPNetIsIPV6()) {
        result = socket(AF_INET6,SOCK_STREAM,0);
    }
    else {
        result = socket(AF_INET,SOCK_STREAM,0);
    }
    if(result >= 0)
    {
        if(connect(result,
                   (struct sockaddr *)peeraddr,
                   sizeof(struct sockaddr_storage)) == 0)
        {
            /* set the receive and send buffer low water mark to 18 bytes 
            (the length of the smallest slpv2 message) */
            lowat = 18;
            setsockopt(result,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
            setsockopt(result,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat));
            return result;;
        }
        else
        {
            close(result);
            result = -1;
        }
    }

    return result;
}


/*=========================================================================*/ 
int SLPNetworkConnectToMulticast(struct sockaddr_storage *peeraddr, int ttl)
/* Creates a socket and provides a peeraddr to send to                     */
/*                                                                         */
/* peeraddr  (OUT) pointer to receive the connected DA's address           */
/*                                                                         */
/* ttl       (IN) ttl for the mcast socket                                 */
/*                                                                         */
/* Returns   Valid socket or -1 if no DA connection can be made            */
/*=========================================================================*/
{
    int                 sockfd;
#if defined(linux)
    int         optarg;
#else



    /* Solaris and Tru64 expect a unsigned char parameter */
    unsigned char   optarg;
#endif


#ifdef _WIN32
    BOOL Reuse = TRUE;
    int TTLArg;
    struct sockaddr_storage  mysockaddr_storage;

    memset(&mysockaddr_storage, 0, sizeof(mysockaddr_storage));
    if (SLPNetIsIPV6()) {
        SLPNetSetAddr(&mysockaddr_storage, PF_INET6, SLP_RESERVED_PORT, NULL, 0);
    }
    else {
        SLPNetSetAddr(&mysockaddr_storage, PF_INET, SLP_RESERVED_PORT, NULL, 0);
    }
#endif

    /* setup multicast socket */
    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd >= 0)
    {
        DWORD add = SLP_MCAST_ADDRESS;
        if (SLPNetIsIPV6()) {
            SLPNetSetAddr(peeraddr, PF_INET6, SLP_RESERVED_PORT, (unsigned char *) &add, sizeof(add));
        }
        else {
            SLPNetSetAddr(peeraddr, PF_INET, SLP_RESERVED_PORT, (unsigned char *) &add, sizeof(add));
        }
        optarg = ttl;
    }


#ifdef _WIN32
    TTLArg = ttl;
    if(setsockopt(sockfd,
        SOL_SOCKET,
        SO_REUSEADDR,
        (const char  *)&Reuse,
        sizeof(Reuse)) ||
        bind(sockfd, 
        (struct sockaddr *)&mysockaddr_storage, 
        sizeof(mysockaddr_storage)) ||
        setsockopt(sockfd,
        IPPROTO_IP,
        IP_MULTICAST_TTL,
        (char *)&TTLArg,
        sizeof(TTLArg)))
    {
        return -1;
    }
#else
    if(setsockopt(sockfd,IPPROTO_IP,IP_MULTICAST_TTL,&optarg,sizeof(optarg)))
    {
        return -1;
    }
#endif
    return sockfd;
}

/*=========================================================================*/ 
int SLPNetworkConnectToBroadcast(struct sockaddr_storage *peeraddr)
/* Creates a socket and provides a peeraddr to send to                     */
/*                                                                         */
/* peeraddr         (OUT) pointer to receive the connected DA's address    */                                                       
/*                                                                         */
/* Returns          Valid socket or -1 if no DA connection can be made     */
/*=========================================================================*/
{
    int     sockfd;
#ifdef _WIN32
    BOOL    on = 1;
#else
    int     on = 1;
#endif



    if (SLPNetIsIPV6()) {
        return(-1);
    }
    /* setup broadcast */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd >= 0)
    {
        DWORD ad = SLP_BCAST_ADDRESS;
        SLPNetSetAddr(peeraddr, PF_INET, SLP_RESERVED_PORT, (unsigned char *) &ad, sizeof(ad));
        if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char*)&on, sizeof(on)))
        {
            return -1;
        }
    }

    return sockfd;
}


/*=========================================================================*/ 
int SLPNetworkSendMessage(int sockfd,
                          int socktype,
                          SLPBuffer buf,
                          struct sockaddr_storage* peeraddr,
                          struct timeval* timeout)
/* Sends a message                                                         */
/*                                                                         */
/* Returns  -  zero on success non-zero on failure                         */
/*                                                                         */
/* errno         EPIPE error during write                                  */
/*               ETIME read timed out                                      */
/*=========================================================================*/ 
{
    fd_set      writefds;
    int         xferbytes;
    int         flags = 0;

#if defined(MSG_NOSIGNAL)
    flags = MSG_NOSIGNAL;
#endif

    buf->curpos = buf->start;

    while(buf->curpos < buf->end)
    {
        FD_ZERO(&writefds);
        FD_SET(sockfd, &writefds);

        xferbytes = select(sockfd + 1, 0, &writefds, 0, timeout);
        if(xferbytes > 0)
        {
            if(socktype == SOCK_DGRAM)
            {
                xferbytes = sendto(sockfd,
                                   buf->curpos, 
                                   buf->end - buf->curpos, 
                                   flags,
                                   (struct sockaddr *)peeraddr,
                                   sizeof(struct sockaddr_storage));
            }
            else
            {
                xferbytes = send(sockfd,
                                 buf->curpos, 
                                 buf->end - buf->curpos, 
                                 flags);
            }

            if(xferbytes > 0)
            {
                buf->curpos = buf->curpos + xferbytes;
            }
            else
            {
                errno = EPIPE;
                return -1;
            }
        }
        else if(xferbytes == 0)
        {
            /* timed out */
            errno = ETIMEDOUT;
            return -1;
        }
        else
        {
            errno = EPIPE;
            return -1;
        }
    }

    return 0;
}



/*=========================================================================*/ 
int SLPNetworkRecvMessage(int sockfd,
                          int socktype,
                          SLPBuffer* buf,
                          struct sockaddr_storage* peeraddr,
                          struct timeval* timeout)
/* Receives a message                                                      */
/*                                                                         */
/* Returns  -    zero on success, non-zero on failure                      */
/*                                                                         */
/* errno         ENOTCONN error during read                                */
/*               ETIME read timed out                                      */
/*               ENOMEM out of memory                                      */
/*               EINVAL parse error                                        */
/*=========================================================================*/ 
{
    int         xferbytes;
    fd_set      readfds;
    char        peek[16];
    int         peeraddrlen = sizeof(struct sockaddr_storage);

    /*---------------------------------------------------------------*/
    /* take a peek at the packet to get version and size information */
    /*---------------------------------------------------------------*/
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    xferbytes = select(sockfd + 1, &readfds, 0 , 0, timeout);
    if(xferbytes > 0)
    {
        if(socktype == SOCK_DGRAM)
        {
            xferbytes = recvfrom(sockfd,
                                 peek,
                                 16,
                                 MSG_PEEK,
                                 (struct sockaddr *)peeraddr,
                                 &peeraddrlen);
        }
        else
        {
            xferbytes = recv(sockfd,
                             peek,
                             16,
                             MSG_PEEK);
        }

        if(xferbytes <= 0)
        {
#ifdef _WIN32
            if(WSAGetLastError() != WSAEMSGSIZE)
            {
                errno = ENOTCONN;
                return -1;
            }
#else
            errno = ENOTCONN;
            return -1;
#endif
        }
    }
    else if(xferbytes == 0)
    {
        errno = ETIMEDOUT;
        return -1;
    }
    else
    {
        errno = ENOTCONN;
        return -1;
    }

    /*------------------------------*/
    /* Read the rest of the message */
    /*------------------------------*/
    /* check the version */
    if(*peek == 2)
    {
        /* allocate the recvmsg big enough for the whole message */
        *buf = SLPBufferRealloc(*buf, AsUINT24(peek + 2));
        if(*buf)
        {
            while((*buf)->curpos < (*buf)->end)
            {
                FD_ZERO(&readfds);
                FD_SET(sockfd, &readfds);
                xferbytes = select(sockfd + 1, &readfds, 0 , 0, timeout);
                if(xferbytes > 0)
                {
                    xferbytes = recv(sockfd,
                                     (*buf)->curpos, 
                                     (*buf)->end - (*buf)->curpos, 
                                     0);
                    if(xferbytes > 0)
                    {
                        (*buf)->curpos = (*buf)->curpos + xferbytes;
                    }
                    else
                    {
                        errno = ENOTCONN;
                        return -1;
                    }
                }
                else if(xferbytes == 0)
                {
                    errno = ETIMEDOUT;
                    return -1;
                }
                else
                {
                    errno =  ENOTCONN;
                    return -1;
                }
            } /* end of main read while. */  
        }
        else
        {
            errno = ENOMEM;
            return -1;
        }
    }
    else
    {
        errno = EINVAL;
        return -1;
    }

    return 0;
}
