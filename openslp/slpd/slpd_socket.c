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

#ifdef WIN32
    #define CloseSocket(Arg) closesocket(Arg)
#else
    #define CloseSocket(Arg) close(Arg)
#endif

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
int JoinSLPMulticastGroup(sockfd_t sockfd, struct in_addr* addr)
/* Sets the socket options to receive multicast traffic from the specified */
/* interface.                                                              */
/*                                                                         */
/* sockfd   - the socket file descriptor to set the options on.            */
/*                                                                         */
/* addr     - pointer to address of the interface to join on               */
/*                                                                         */
/* returns  - zero on success                                              */
/*-------------------------------------------------------------------------*/
{
    struct ip_mreq  mreq;
    struct in_addr  mcast_addr;

    /* join using the reserved SLP_MCAST_ADDRESS */
    mcast_addr.s_addr = htonl(SLP_MCAST_ADDRESS);
    mreq.imr_multiaddr = mcast_addr;

    /* join with specified interface */
    memcpy(&mreq.imr_interface,addr,sizeof(struct in_addr));

    return setsockopt(sockfd,
                      IPPROTO_IP,
                      IP_ADD_MEMBERSHIP,
                      (char*)&mreq,
                      sizeof(mreq));               
}


/*-------------------------------------------------------------------------*/
int DropSLPMulticastGroup(sockfd_t sockfd, struct in_addr* addr)
/* Sets the socket options to not receive multicast traffic from the       */
/* specified interface.                                                    */
/*                                                                         */
/* sockfd   - the socket file descriptor to set the options on.            */
/*                                                                         */
/* addr     - pointer to the multicast address                             */
/*-------------------------------------------------------------------------*/
{
    struct ip_mreq  mreq;
    struct in_addr  mcast_addr;

    /* join using the reserved SLP_MCAST_ADDRESS */
    mcast_addr.s_addr = htonl(SLP_MCAST_ADDRESS);
    mreq.imr_multiaddr = mcast_addr;

    /* join with specified interface */
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
    BOOL Reuse = TRUE;
#else
    int                 lowat;
#endif
    struct sockaddr_in  mysockaddr;

    memset(&mysockaddr, 0, sizeof(mysockaddr));
    mysockaddr.sin_family = AF_INET;
    mysockaddr.sin_port = htons(SLP_RESERVED_PORT);

#ifdef WIN32
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(const char *)&Reuse,sizeof(Reuse));
#endif

    if (addr != NULL)
    {

        mysockaddr.sin_addr = *addr;
    } else
    {
        mysockaddr.sin_addr.s_addr = INADDR_ANY;
    }

    result = bind(sock, (struct sockaddr *) &mysockaddr,sizeof(mysockaddr));
    if (result == 0)
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

    sock = (SLPDSocket*)malloc(sizeof(SLPDSocket));
    if (sock)
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
    close(sock->fd);

    /* free receive buffer */
    if (sock->recvbuf)
    {
        SLPBufferFree(sock->recvbuf);
    }

    /* free send buffer(s) */
    if (sock->sendlist.count)
    {
        while (sock->sendlist.count)
        {
            SLPBufferFree((SLPBuffer)SLPListUnlink(&(sock->sendlist), sock->sendlist.head));
        }
    } else if (sock->sendbuf)
    {
        SLPBufferFree(sock->sendbuf);                        
    }

    /* free the actual socket structure */
    free(sock);
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
    if (sock)
    {
        sock->fd = socket(PF_INET, SOCK_DGRAM, 0);
        if (sock->fd >=0)
        {
            sock->recvbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);  
            sock->sendbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
            sock->peerinfo.peeraddr.sin_family = AF_INET;
            sock->peerinfo.peeraddr.sin_addr = *peeraddr;
            sock->peerinfo.peeraddr.sin_port = htons(SLP_RESERVED_PORT);
            sock->peerinfo.peeraddrlen = sizeof(sock->peerinfo.peeraddr);
            sock->state = type;
        } else
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

    sock = SLPDSocketAlloc();
    if (sock)
    {
        sock->fd = socket(PF_INET, SOCK_DGRAM, 0);
        if (sock->fd >=0)
        {
            if (BindSocketToInetAddr(sock->fd, peeraddr) == 0)
            {
                sock->recvbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);  
                sock->sendbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
                if (peeraddr != NULL)
                {
                    sock->peerinfo.peeraddr.sin_addr = *peeraddr;
                    sock->peerinfo.peeraddrlen = sizeof(sock->peerinfo.peeraddr);
                } else
                {
                    sock->peerinfo.peeraddrlen = sizeof(struct sockaddr);
                }

                sock->peerinfo.peertype = SLPD_PEER_ACCEPTED;     

                switch (type)
                {
                case DATAGRAM_MULTICAST:
                    if (JoinSLPMulticastGroup(sock->fd, myaddr) == 0)
                    {
                        sock->state = DATAGRAM_MULTICAST;
                        goto SUCCESS;
                    }
                    break;

                case DATAGRAM_BROADCAST:
                    if (EnableBroadcast(sock->fd) == 0)
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

    if (sock)
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
    if (sock)
    {
        sock->fd = socket(PF_INET, SOCK_STREAM, 0);
        if (sock->fd >= 0)
        {
            if (BindSocketToInetAddr(sock->fd, peeraddr) >= 0)
            {
                if (listen(sock->fd,5) == 0)
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

    if (sock)
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
    if (sock == 0)
    {
        goto FAILURE;
    }

    /* create the stream socket */
    sock->fd = socket(PF_INET,SOCK_DGRAM,0);
    if (sock->fd < 0)
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
    sock->peerinfo.peeraddr.sin_family = AF_INET;
    sock->peerinfo.peeraddr.sin_port = htons(SLP_RESERVED_PORT);
    sock->peerinfo.peeraddr.sin_addr = *addr;

    /* set the receive and send buffer low water mark to 18 bytes 
    (the length of the smallest slpv2 message) */
    lowat = 18;
    setsockopt(sock->fd,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
    setsockopt(sock->fd,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat));

    /* non-blocking connect */
    if (connect(sock->fd, 
                (struct sockaddr *) &(sock->peerinfo.peeraddr), 
                sizeof(sock->peerinfo.peeraddr)) == 0)
    {
        /* Connection occured immediately */
        sock->state = STREAM_CONNECT_IDLE;
    } else
    {
#ifdef WIN32
        if (WSAEWOULDBLOCK == WSAGetLastError())
#else
        if (errno == EINPROGRESS)
#endif
        {
            /* Connect would have blocked */
            sock->state = STREAM_CONNECT_BLOCK;
        } else
        {
            goto FAILURE;
        }                                
    }                   

    return sock;

    /* cleanup on failure */
    FAILURE:
    if (sock)
    {
        CloseSocket(sock->fd);
        free(sock);
        sock = 0;
    }

    return sock;
}

