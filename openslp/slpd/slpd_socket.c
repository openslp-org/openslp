/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_socket.c                                              */
/*                                                                         */
/* Abstract:    Socket specific functions implementation                   */
/*                                                                         */
/* WARNING:     NOT thread safe!                                           */
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
#include "slpd_socket.h"
#include "slpd_property.h"


/*=========================================================================*/
/* common code includes                                                    */
/*=========================================================================*/
#include "slp_message.h"
#include "slp_xmalloc.h"
#include "slp_net.h"


/*-------------------------------------------------------------------------*/
int EnableBroadcast(sockfd_t sockfd)
/* Sets the socket options to receive broadcast traffic                    */
/*                                                                         */
/* sockfd   - the socket file descriptor to set option on                  */
/*                                                                         */
/* returns  - zero on success                                              */
/*-------------------------------------------------------------------------*/
{
#ifdef _WIN32
    const char on = 1;
#else
    const int on = 1;                                                
#endif
    return setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,&on,sizeof(on));
}

/*-------------------------------------------------------------------------*/
int SetMulticastTTL(sockfd_t sockfd, int ttl)
/* Set the socket options for ttl                                          */
/*                                                                         */
/* sockfd   - the socket file descriptor to set option on                  */
/*                                                                         */
/* returns  - zero on success                                              */
/*-------------------------------------------------------------------------*/
{

#if defined(linux)
    int         optarg = ttl;
#else
    /* Solaris and Tru64 expect a unsigned char parameter */
    unsigned char   optarg = (unsigned char)ttl;
#endif


#ifdef _WIN32
    BOOL Reuse = TRUE;
    int TTLArg;
    struct sockaddr_storage mysockaddr;

    memset(&mysockaddr, 0, sizeof(mysockaddr));
    mysockaddr.ss_family = AF_INET;
    ((struct sockaddr_in*) &mysockaddr)->sin_port = 0;

    TTLArg = ttl;
    if(setsockopt(sockfd,
                  SOL_SOCKET,
                  SO_REUSEADDR,
                  (const char  *)&Reuse,
                  sizeof(Reuse)) ||
       bind(sockfd, 
            (struct sockaddr *)&mysockaddr, 
            sizeof(mysockaddr)) ||
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

    return 0;
}


/*-------------------------------------------------------------------------*/
int JoinSLPMulticastGroup(int family,
                          sockfd_t sockfd,
                          struct sockaddr_storage* maddr,
                          struct sockaddr_storage* addr)
/* Sets the socket options to receive multicast traffic from the specified */
/* interface.                                                              */
/*                                                                         */
/* family   - the address family of the multicast group to join.           */
/*                                                                         */
/* sockfd   - the socket file descriptor to set the options on.            */
/*                                                                         */
/* maddr    - pointer to multicast group to join                           */
/*                                                                         */
/* addr     - pointer to address of the interface to join on               */
/*                                                                         */
/* returns  - zero on success                                              */
/*-------------------------------------------------------------------------*/
{
    struct ip_mreq      mreq4;
    struct ipv6_mreq    mreq6;
    
    if (SLPNetIsIPV4() && family == AF_INET)
    {
        /* join using the multicast address passed in */
        memcpy(&mreq4.imr_multiaddr, &(((struct sockaddr_in*) maddr)->sin_addr), sizeof(struct in_addr));

        /* join with specified interface */
        memcpy(&mreq4.imr_interface, &(((struct sockaddr_in*) addr)->sin_addr), sizeof(struct in_addr));
        return setsockopt(sockfd,
                          IPPROTO_IP,
                          IP_ADD_MEMBERSHIP,
                          (char*)&mreq4,
                          sizeof(mreq4));               
    }
    else if (SLPNetIsIPV6() && family == AF_INET6)
    {
        /* join using the multicast address passed in */
        memcpy(&mreq6.ipv6mr_multiaddr, &(((struct sockaddr_in6*) maddr)->sin6_addr), sizeof(struct in6_addr));

        /* join with specified interface */
        mreq6.ipv6mr_interface = ((struct sockaddr_in6*) addr)->sin6_scope_id;
        return setsockopt(sockfd,
                          IPPROTO_IPV6,
                          IPV6_JOIN_GROUP,
                          (char*)&mreq6,
                          sizeof(mreq6));               
    }
    else
    {
        /* error in address family specified */
        return -1;
    }
}


/*-------------------------------------------------------------------------*/
int DropSLPMulticastGroup(int family,
                          sockfd_t sockfd,
                          struct sockaddr_storage* maddr,
                          struct sockaddr_storage* addr)
/* Sets the socket options to not receive multicast traffic from the       */
/* specified interface.                                                    */
/*                                                                         */
/* family   - the address family (AF_INET or AF_INET6).                    */
/*                                                                         */
/* sockfd   - the socket file descriptor to set the options on.            */
/*                                                                         */
/* maddr    - pointer to the multicast address                             */
/*                                                                         */
/* addr     - pointer to the multicast address                             */
/*-------------------------------------------------------------------------*/
{
    struct ip_mreq      mreq4;
    struct ipv6_mreq    mreq6;

    if (SLPNetIsIPV4() && family == AF_INET)
    {
        /* drop from the multicast group passed in */
        memcpy(&mreq4.imr_multiaddr, &(((struct sockaddr_in*) maddr)->sin_addr), sizeof(struct in_addr));

        /* drop for the specified interface */
        memcpy(&mreq4.imr_interface, &(((struct sockaddr_in*) addr)->sin_addr), sizeof(struct in_addr));

        return setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&mreq4,sizeof(mreq4));               
    }
    else if (SLPNetIsIPV6() && family == AF_INET6)
    {
        /* drop from the multicast group passed in */
        memcpy(&mreq6.ipv6mr_multiaddr, &(((struct sockaddr_in6*) maddr)->sin6_addr), sizeof(struct in6_addr));

        /* drop for the specified interface */
        mreq6.ipv6mr_interface = 0;

        return setsockopt(sockfd, IPPROTO_IPV6, IPV6_LEAVE_GROUP, (char*)&mreq6,sizeof(mreq6));               
    }
    else
    {
        /* error in address family specified */
        return -1;
    }
}


/*-------------------------------------------------------------------------*/
int BindSocketToInetAddr(int family, int sock, struct sockaddr_storage* addr)
/* Binds the specified socket to the SLP port and interface.               */
/*                                                                         */
/* family   - the address family (AF_INET or AF_INET6).                    */
/*                                                                         */
/* sock     - the socket to be bound                                       */
/*                                                                         */
/* addr     - the address to bind to.                                      */
/*                                                                         */
/* Returns  - zero on success, -1 on error.                                */
/*-------------------------------------------------------------------------*/
{
    struct sockaddr_storage temp_addr;
    int                     result;
#ifdef _WIN32
    char                    lowat;
    BOOL                    reuse = TRUE;
#else
    int                     lowat;
    int                     reuse = 1;
#endif

    if (SLPNetIsIPV4() && family == AF_INET)
    {
        if (addr == NULL)
        {
            addr = &temp_addr;
            addr->ss_family = AF_INET;
            ((struct sockaddr_in*) addr)->sin_addr.s_addr = INADDR_ANY;
        }
        ((struct sockaddr_in*) addr)->sin_port = htons(SLP_RESERVED_PORT);
    }
    else if (SLPNetIsIPV6() && family == AF_INET6)
    {
        if (addr == NULL)
        {
            addr = &temp_addr;
            addr->ss_family = AF_INET6;
            ((struct sockaddr_in6*) addr)->sin6_scope_id = 0;
            memcpy(&(((struct sockaddr_in6*) addr)->sin6_addr), &in6addr_any, sizeof(struct in6_addr));
        }
        ((struct sockaddr_in6*) addr)->sin6_port = htons(SLP_RESERVED_PORT);
    }

    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(const char *)&reuse,sizeof(reuse));

    result = bind(sock, (struct sockaddr*) addr, sizeof(struct sockaddr_storage));
    if(result == 0)
    {
        /* set the receive and send buffer low water mark to 18 bytes 
        (the length of the smallest slpv2 message) */
        lowat = 18;
        setsockopt(sock,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
        setsockopt(sock,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat));
    }
    else {
        printf("Error was %d\n", WSAGetLastError());
    }

    return result;
}


/*-------------------------------------------------------------------------*/
int BindSocketToLoopback(int family, int sock)
/* Binds the specified socket to the specified port of the loopback        */
/* interface.                                                              */
/*                                                                         */
/* family   - the address family (AF_INET or AF_INET6).                    */
/*                                                                         */
/* sock     - the socket to be bound                                       */
/*                                                                         */
/* Returns  - zero on success, -1 on error.                                */
/*-------------------------------------------------------------------------*/
{
    struct sockaddr_storage loaddr;

    if (SLPNetIsIPV4() && family == AF_INET)
    {
        ((struct sockaddr_in*) &loaddr)->sin_family = AF_INET;
        ((struct sockaddr_in*) &loaddr)->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        return BindSocketToInetAddr(AF_INET, sock,&loaddr);
    }
    else if (SLPNetIsIPV6() && family == AF_INET6)
    {
        ((struct sockaddr_in6*) &loaddr)->sin6_family = AF_INET6;
        memcpy(&(((struct sockaddr_in6*) &loaddr)->sin6_addr), &in6addr_loopback, sizeof(struct in6_addr));
        ((struct sockaddr_in6*) &loaddr)->sin6_scope_id = 0;
        return BindSocketToInetAddr(AF_INET6, sock,&loaddr);
    }
    else
    {
        /* error in address family specified */
        return -1;
    }
}

/*=========================================================================*/
SLPDSocket* SLPDSocketAlloc()
/* Allocate memory for a new SLPDSocket.                                   */
/*                                                                         */
/* Returns: pointer to a newly allocated SLPDSocket, or NULL if out of     */
/*          memory.                                                        */
/*=========================================================================*/
{
    SLPDSocket* sock;

    sock = (SLPDSocket*)xmalloc(sizeof(SLPDSocket));
    if(sock)
    {
        memset(sock,0,sizeof(SLPDSocket));
        sock->fd = -1;
    }

    return sock;
}

/*=========================================================================*/
void SLPDSocketFree(SLPDSocket* sock)
/* Frees memory associated with the specified SLPDSocket                   */
/*                                                                         */
/* sock (IN) pointer to the socket to free                                 */
/*=========================================================================*/
{
    /* close the socket descriptor */
    CloseSocket(sock->fd);

    /* free receive buffer */
    if(sock->recvbuf)
    {
        SLPBufferFree(sock->recvbuf);
    }

    /* free send buffer(s) */
    if(sock->sendlist.count)
    {
        while(sock->sendlist.count)
        {
            SLPBufferFree((SLPBuffer)SLPListUnlink(&(sock->sendlist), sock->sendlist.head));
        }
    }
    
    if(sock->sendbuf)
    {
        SLPBufferFree(sock->sendbuf);                        
    }

    /* free the actual socket structure */
    xfree(sock);
}


/*==========================================================================*/
SLPDSocket* SLPDSocketCreateDatagram(struct sockaddr_storage* peeraddr,
                                     int type)
/* peeraddr - (IN) the address of the peer to connect to                    */
/*                                                                          */
/* type (IN) DATAGRAM_UNICAST, DATAGRAM_MULTICAST, DATAGRAM_BROADCAST       */
/*                                                                          */
/* Returns: A datagram socket SLPDSocket->state will be set to              */
/*          DATAGRAM_UNICAST, DATAGRAM_MULTICAST, or DATAGRAM_BROADCAST     */
/*==========================================================================*/
{
    SLPDSocket*     sock;  
    sock = SLPDSocketAlloc();
    if(sock)
    {
        /* SLP_MAX_DATAGRAM_SIZE is as big as a datagram SLP     */
        /* can be.                                               */
        sock->recvbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
        sock->sendbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
        if(sock->recvbuf && sock->sendbuf)
        {
            if (peeraddr->ss_family == AF_INET)
                sock->fd = socket(PF_INET, SOCK_DGRAM, 0);
            else if (peeraddr->ss_family == AF_INET6)
                sock->fd = socket(PF_INET6, SOCK_DGRAM, 0);
            else
                sock->fd = INVALID_SOCKET;  /* only IPv4 (and soon IPv6) are supported */

            if(sock->fd >= 0)
            {
                switch(type)
                {
                case DATAGRAM_BROADCAST:
                    EnableBroadcast(sock->fd);
                    break;

                case DATAGRAM_MULTICAST:
                    SetMulticastTTL(sock->fd,G_SlpdProperty.multicastTTL);
                    break;

                default:
                    break;
                }
                
                memcpy(&sock->peeraddr, peeraddr, sizeof(struct sockaddr_storage));
                if (peeraddr->ss_family == AF_INET) {
                    ((struct sockaddr_in*) &(sock->peeraddr))->sin_port = htons(SLP_RESERVED_PORT);
                    sock->state = type;
                }
                else if (peeraddr->ss_family == AF_INET6) {
                    ((struct sockaddr_in6*) &(sock->peeraddr))->sin6_port = htons(SLP_RESERVED_PORT);
                    ((struct sockaddr_in6*) &(sock->peeraddr))->sin6_scope_id = 0;
                    sock->state = type;
                }
                else {
                    SLPDSocketFree(sock);
                    sock = 0;
                }

            }
            else
            {
                SLPDSocketFree(sock);
                sock = 0;
            }
        }
        else
        {
            SLPDSocketFree(sock);
            sock = 0;
        }
    }

    return sock;
} 

/*==========================================================================*/
SLPDSocket* SLPDSocketCreateBoundDatagram(struct sockaddr_storage* myaddr,
                                          struct sockaddr_storage* peeraddr,
                                          int type)
/* myaddr - (IN) the address of the interface to join mcast on              */ 
/*                                                                          */
/* peeraddr - (IN) the address of the peer to connect to                    */
/*                                                                          */
/* type (IN) DATAGRAM_UNICAST, DATAGRAM_MULTICAST, DATAGRAM_BROADCAST       */
/*                                                                          */
/* Returns: A datagram socket SLPDSocket->state will be set to              */
/*          DATAGRAM_UNICAST, DATAGRAM_MULTICAST, or DATAGRAM_BROADCAST     */
/*==========================================================================*/
{
    SLPDSocket*                 sock;
    struct sockaddr_storage*    bindaddr;

    /*------------------------------------------*/
    /* Adjust for multicast binding differences */
    /*------------------------------------------*/
#ifdef LINUX
    bindaddr = peeraddr;  
#else
    if(type == DATAGRAM_MULTICAST)
        bindaddr = NULL;    /* must bind to INADDR_ANY for multicast */
    else
        bindaddr = peeraddr;  
#endif

    /*------------------------*/
    /* Create and bind socket */
    /*------------------------*/
    sock = SLPDSocketAlloc();
    if(sock)
    {
        sock->recvbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
        sock->sendbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
        if (SLPNetIsIPV4() && peeraddr->ss_family == AF_INET)
            sock->fd = socket(PF_INET, SOCK_DGRAM, 0);
        else if (SLPNetIsIPV6() && peeraddr->ss_family == AF_INET6)
            sock->fd = socket(PF_INET6, SOCK_DGRAM, 0);
        else
            sock->fd = INVALID_SOCKET;  /* only ipv4 and ipv6 are supported */

        if(sock->fd >=0)
        {
            if(BindSocketToInetAddr(peeraddr->ss_family, sock->fd, bindaddr) == 0)
            {
                if(peeraddr != NULL)
                {
                    if (peeraddr->ss_family == AF_INET)
                        memcpy(&((struct sockaddr_in*) &sock->peeraddr)->sin_addr, &((struct sockaddr_in*) peeraddr)->sin_addr, sizeof(struct sockaddr_in));
                    else if (peeraddr->ss_family == AF_INET6)
                        memcpy(&((struct sockaddr_in6*) &sock->peeraddr)->sin6_addr, &((struct sockaddr_in6*) peeraddr)->sin6_addr, sizeof(struct sockaddr_in6));
                }

                switch(type)
                {
                case DATAGRAM_MULTICAST:
                    if(JoinSLPMulticastGroup(peeraddr->ss_family, sock->fd, peeraddr, myaddr) == 0)
                    {
                        /* store mcast address */
                        memcpy(&(sock->mcastaddr), peeraddr, sizeof(struct sockaddr_storage));

                        sock->state = DATAGRAM_MULTICAST;
                        goto SUCCESS;
                    }
                    break;

                case DATAGRAM_BROADCAST:
                    if(EnableBroadcast(sock->fd) == 0)
                    {
                        sock->state = DATAGRAM_BROADCAST;
                        goto SUCCESS;
                    }
                    break;

                case DATAGRAM_UNICAST:
                default:
                    sock->state = DATAGRAM_UNICAST;
                    goto SUCCESS;
                    break;  
                }
            }
        }
    }

    if(sock)
    {
        SLPDSocketFree(sock);
    }
    sock = 0;


    SUCCESS:    
    return sock;    
}


/*==========================================================================*/
SLPDSocket* SLPDSocketCreateListen(struct sockaddr_storage* peeraddr)
/*                                                                          */
/* peeraddr - (IN) the address of the peer to connect to                    */
/*                                                                          */
/* type (IN) DATAGRAM_UNICAST, DATAGRAM_MULTICAST, DATAGRAM_BROADCAST       */
/*                                                                          */
/* Returns: A listening socket. SLPDSocket->state will be set to            */
/*          SOCKET_LISTEN.   Returns NULL on error                          */
/*==========================================================================*/
{
    int fdflags;
    SLPDSocket* sock;

    sock = SLPDSocketAlloc();
    if(sock)
    {
        if (SLPNetIsIPV4() && peeraddr->ss_family == AF_INET)
            sock->fd = socket(PF_INET, SOCK_STREAM, 0);
        else if (SLPNetIsIPV6() && peeraddr->ss_family == AF_INET6)
            sock->fd = socket(PF_INET6, SOCK_STREAM, 0);
        else
            sock->fd = INVALID_SOCKET;  /* only ipv4 (and soon ipv6) are supported */

        if(sock->fd >= 0)
        {
            if(BindSocketToInetAddr(peeraddr->ss_family, sock->fd, peeraddr) >= 0)
            {
                if(listen(sock->fd,5) == 0)
                {
                    /* Set socket to non-blocking so subsequent calls to */
                    /* accept will *never* block                         */
#ifdef _WIN32
                    fdflags = 1;
                    ioctlsocket(sock->fd, FIONBIO, &fdflags);
#else
                    fdflags = fcntl(sock->fd, F_GETFL, 0);
                    fcntl(sock->fd,F_SETFL, fdflags | O_NONBLOCK);
#endif        
                    sock->state = SOCKET_LISTEN;
                    
                    return sock;
                }
            }
        }
    }

    if(sock)
    {
        SLPDSocketFree(sock);
    }

    return 0;
}

/*==========================================================================*/
int SLPDSocketIsMcastOn(SLPDSocket* sock, struct sockaddr_storage* addr)
/*                                                                          */
/* sock - (IN) the socket to check                                          */
/*                                                                          */
/* addr - (IN) the address to check                                         */
/*                                                                          */
/* Returns: non-zero if the socket is listening on that address.            */
/*          Zero, otherwise.                                                */
/*==========================================================================*/
{
    switch (sock->mcastaddr.ss_family) {
        case AF_INET:
            if (memcmp(&(((struct sockaddr_in*) &sock->mcastaddr)->sin_addr), &(((struct sockaddr_in*) addr)->sin_addr), sizeof(struct in_addr)) == 0)
                return 1;
            else
                return 0;
        case AF_INET6:
            if (memcmp(&(((struct sockaddr_in6*) &sock->mcastaddr)->sin6_addr), &(((struct sockaddr_in6*) addr)->sin6_addr), sizeof(struct in6_addr)) == 0)
                return 1;
            else
                return 0;
        default:
            /* only IPv4 and IPv6 are supported */
            return 0;
    }
}

/*==========================================================================*/
SLPDSocket* SLPDSocketCreateConnected(struct sockaddr_storage* addr)
/*                                                                          */
/* addr - (IN) the address of the peer to connect to                        */
/*                                                                          */
/* Returns: A connected socket or a socket in the process of being connected*/
/*          if the socket was connected the SLPDSocket->state will be set   */
/*          to writable.  If the connect would block, SLPDSocket->state will*/
/*          be set to connect.  Return NULL on error                        */
/*==========================================================================*/
{
#ifdef _WIN32
    char                lowat;
    u_long              fdflags;
#else
    int                 lowat;
    int                 fdflags;
#endif
    SLPDSocket*         sock = 0;

    sock = SLPDSocketAlloc();
    if(sock == 0)
    {
        goto FAILURE;
    }

    /* create the stream socket */
    if (addr->ss_family == AF_INET)
        sock->fd = socket(PF_INET,SOCK_STREAM,0);
    else if (addr->ss_family == AF_INET6)
        sock->fd = socket(PF_INET6,SOCK_STREAM,0);
    else
        sock->fd = INVALID_SOCKET;

    if(sock->fd < 0)
    {
        goto FAILURE;                        
    }

    /* set the socket to non-blocking */
#ifdef _WIN32
    fdflags = 1;
    ioctlsocket(sock->fd, FIONBIO, &fdflags);
#else
    fdflags = fcntl(sock->fd, F_GETFL, 0);
    fcntl(sock->fd,F_SETFL, fdflags | O_NONBLOCK);
#endif  

    /* zero then set peeraddr to connect to */
    if (addr->ss_family == AF_INET) {
        ((struct sockaddr_in*) &(sock->peeraddr))->sin_addr = ((struct sockaddr_in*) addr)->sin_addr;
        ((struct sockaddr_in*) &(sock->peeraddr))->sin_port = htons(SLP_RESERVED_PORT);
    }
    else if (addr->ss_family == AF_INET6) {
        ((struct sockaddr_in6*) &(sock->peeraddr))->sin6_addr = ((struct sockaddr_in6*) addr)->sin6_addr;
        ((struct sockaddr_in6*) &(sock->peeraddr))->sin6_port = htons(SLP_RESERVED_PORT);
    }
    else {
        goto FAILURE;  /* only ipv4 and ipv6 addresses are supported */
    }

    /* set the receive and send buffer low water mark to 18 bytes 
    (the length of the smallest slpv2 message) */
    lowat = 18;
    setsockopt(sock->fd,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
    setsockopt(sock->fd,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat));

    /* non-blocking connect */
    if(connect(sock->fd, 
               (struct sockaddr*) &(sock->peeraddr), 
               sizeof(sock->peeraddr)) == 0)
    {
        /* Connection occured immediately */
        sock->state = STREAM_CONNECT_IDLE;
    }
    else
    {
#ifdef _WIN32
        if(WSAEWOULDBLOCK == WSAGetLastError())
#else
        if(errno == EINPROGRESS)
#endif
        {
            /* Connect would have blocked */
            sock->state = STREAM_CONNECT_BLOCK;
        }
        else
        {
            goto FAILURE;
        }                                
    }                   

    return sock;

    /* cleanup on failure */
    FAILURE:
    if(sock)
    {
        SLPDSocketFree(sock);
        sock = 0;
    }

    return sock;
}

