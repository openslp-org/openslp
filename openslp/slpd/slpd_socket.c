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


/*-------------------------------------------------------------------------*/
int EnableBroadcast(sockfd_t sockfd)
/* Sets the socket options to receive broadcast traffic                    */
/*                                                                         */
/* sockfd   - the socket file descriptor to set option on                  */
/*                                                                         */
/* returns  - zero on success                                              */
/*-------------------------------------------------------------------------*/
{
#ifdef WIN32
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


#ifdef WIN32
    BOOL Reuse = TRUE;
    int TTLArg;
    struct sockaddr_in  mysockaddr;

    memset(&mysockaddr, 0, sizeof(mysockaddr));
    mysockaddr.sin_family = AF_INET;
    mysockaddr.sin_port = 0;

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
int JoinSLPMulticastGroup(sockfd_t sockfd, struct in_addr* maddr,
                          struct in_addr* addr)
/* Sets the socket options to receive multicast traffic from the specified */
/* interface.                                                              */
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
    struct ip_mreq  mreq;

    /* join using the multicast address passed in */
    memcpy(&mreq.imr_multiaddr, maddr, sizeof(struct in_addr));

    /* join with specified interface */
    memcpy(&mreq.imr_interface, addr, sizeof(struct in_addr));

    return setsockopt(sockfd,
                      IPPROTO_IP,
                      IP_ADD_MEMBERSHIP,
                      (char*)&mreq,
                      sizeof(mreq));               
}


/*-------------------------------------------------------------------------*/
int DropSLPMulticastGroup(sockfd_t sockfd, struct in_addr* maddr,
                          struct in_addr* addr)
/* Sets the socket options to not receive multicast traffic from the       */
/* specified interface.                                                    */
/*                                                                         */
/* sockfd   - the socket file descriptor to set the options on.            */
/*                                                                         */
/* maddr     - pointer to the multicast address                             */
/*                                                                         */
/* addr     - pointer to the multicast address                             */
/*-------------------------------------------------------------------------*/
{
    struct ip_mreq  mreq;

    /* drop from the multicast group passed in */
    memcpy(&mreq.imr_multiaddr, maddr, sizeof(struct in_addr));

    /* drop for the specified interface */
    memcpy(&mreq.imr_interface,addr,sizeof(addr));

    return setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&mreq,sizeof(mreq));               
}


/*-------------------------------------------------------------------------*/
int BindSocketToInetAddr(int sock, struct in_addr* addr)
/* Binds the specified socket to the SLP port and interface.               */
/*                                                                         */
/* sock     - the socket to be bound                                       */
/*                                                                         */
/* addr     - the in_addr to bind to.                                      */
/*                                                                         */
/* Returns  - zero on success, -1 on error.                                */
/*-------------------------------------------------------------------------*/
{
    int                 result;
#ifdef WIN32
    char                lowat;
    BOOL                reuse = TRUE;
#else
    int                 lowat;
    int                 reuse = 1;
#endif
    struct sockaddr_in  mysockaddr;

    memset(&mysockaddr, 0, sizeof(mysockaddr));
    mysockaddr.sin_family = AF_INET;
    mysockaddr.sin_port = htons(SLP_RESERVED_PORT);

    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(const char *)&reuse,sizeof(reuse));

    if(addr != NULL)
    {
        mysockaddr.sin_addr = *addr;
    }
    else
    {
        mysockaddr.sin_addr.s_addr = INADDR_ANY;
    }

    result = bind(sock, (struct sockaddr *) &mysockaddr,sizeof(mysockaddr));
    if(result == 0)
    {
        /* set the receive and send buffer low water mark to 18 bytes 
        (the length of the smallest slpv2 message) */
        lowat = 18;
        setsockopt(sock,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
        setsockopt(sock,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat));
    }

    return result;
}


/*-------------------------------------------------------------------------*/
int BindSocketToLoopback(int sock)
/* Binds the specified socket to the specified port of the loopback        */
/* interface.                                                              */
/*                                                                         */
/* sock     - the socket to be bound                                       */
/*                                                                         */
/* Returns  - zero on success, -1 on error.                                */
/*-------------------------------------------------------------------------*/
{
    struct in_addr  loaddr;
    loaddr.s_addr = htonl(LOOPBACK_ADDRESS);
    return BindSocketToInetAddr(sock,&loaddr);
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
    else if(sock->sendbuf)
    {
        SLPBufferFree(sock->sendbuf);                        
    }

    /* free the actual socket structure */
    xfree(sock);
}


/*==========================================================================*/
SLPDSocket* SLPDSocketCreateDatagram(struct in_addr* peeraddr,
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
    SLPDSocket*     sock;  
    sock = SLPDSocketAlloc();
    if(sock)
    {
        sock->recvbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
        sock->sendbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
        if(sock->recvbuf && sock->sendbuf)
        {

            sock->fd = socket(PF_INET, SOCK_DGRAM, 0);
            if(sock->fd >=0)
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

                sock->peeraddr.sin_family = AF_INET;
                sock->peeraddr.sin_addr = *peeraddr;
                sock->peeraddr.sin_port = htons(SLP_RESERVED_PORT);
                sock->state = type;

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
SLPDSocket* SLPDSocketCreateBoundDatagram(struct in_addr* myaddr,
                                          struct in_addr* peeraddr,
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
    SLPDSocket*     sock;
    struct in_addr*  bindaddr;

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
        sock->fd = socket(PF_INET, SOCK_DGRAM, 0);
        if(sock->fd >=0)
        {
            if(BindSocketToInetAddr(sock->fd, bindaddr) == 0)
            {
                if(peeraddr != NULL)
                {
                    sock->peeraddr.sin_addr = *peeraddr;
                }

                switch(type)
                {
                case DATAGRAM_MULTICAST:
                    if(JoinSLPMulticastGroup(sock->fd, peeraddr, myaddr) == 0)
                    {
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
SLPDSocket* SLPDSocketCreateListen(struct in_addr* peeraddr)
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
        sock->fd = socket(PF_INET, SOCK_STREAM, 0);
        if(sock->fd >= 0)
        {
            if(BindSocketToInetAddr(sock->fd, peeraddr) >= 0)
            {
                if(listen(sock->fd,5) == 0)
                {
                    /* Set socket to non-blocking so subsequent calls to */
                    /* accept will *never* block                         */
#ifdef WIN32
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
SLPDSocket* SLPDSocketCreateConnected(struct in_addr* addr)
/*                                                                          */
/* addr - (IN) the address of the peer to connect to                        */
/*                                                                          */
/* Returns: A connected socket or a socket in the process of being connected*/
/*          if the socket was connected the SLPDSocket->state will be set   */
/*          to writable.  If the connect would block, SLPDSocket->state will*/
/*          be set to connect.  Return NULL on error                        */
/*==========================================================================*/
{
#ifdef WIN32
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
    sock->fd = socket(PF_INET,SOCK_STREAM,0);
    if(sock->fd < 0)
    {
        goto FAILURE;                        
    }

    /* set the socket to non-blocking */
#ifdef WIN32
    fdflags = 1;
    ioctlsocket(sock->fd, FIONBIO, &fdflags);
#else
    fdflags = fcntl(sock->fd, F_GETFL, 0);
    fcntl(sock->fd,F_SETFL, fdflags | O_NONBLOCK);
#endif  

    /* zero then set peeraddr to connect to */
    sock->peeraddr.sin_family = AF_INET;
    sock->peeraddr.sin_port = htons(SLP_RESERVED_PORT);
    sock->peeraddr.sin_addr = *addr;

    /* set the receive and send buffer low water mark to 18 bytes 
    (the length of the smallest slpv2 message) */
    lowat = 18;
    setsockopt(sock->fd,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
    setsockopt(sock->fd,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat));

    /* non-blocking connect */
    if(connect(sock->fd, 
               (struct sockaddr *) &(sock->peeraddr), 
               sizeof(sock->peeraddr)) == 0)
    {
        /* Connection occured immediately */
        sock->state = STREAM_CONNECT_IDLE;
    }
    else
    {
#ifdef WIN32
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
        CloseSocket(sock->fd);
        xfree(sock);
        sock = 0;
    }

    return sock;
}

