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

/*-------------------------------------------------------------------------*/
int EnableBroadcast(int sockfd)
/* Sets the socket options to receive broadcast traffic                    */
/*                                                                         */
/* sockfd   - the socket file descriptor to set option on                  */
/*                                                                         */
/* returns  - zero on success                                              */
/*-------------------------------------------------------------------------*/
{
    const int on = 1;                                                
    return setsockopt(sockfd,SOL_SOCKET,SO_BROADCAST,&on,sizeof(on));
}

/*-------------------------------------------------------------------------*/
int JoinSLPMulticastGroup(int sockfd, struct in_addr* addr)
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
    struct ip_mreq	mreq;
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
int DropSLPMulticastGroup(int sockfd, struct in_addr* addr)
/* Sets the socket options to not receive multicast traffic from the       */
/* specified interface.                                                    */
/*                                                                         */
/* sockfd   - the socket file descriptor to set the options on.            */
/*                                                                         */
/* addr     - pointer to the multicast address                             */
/*-------------------------------------------------------------------------*/
{
    struct ip_mreq	mreq;
	struct in_addr	mcast_addr;

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
    int                 lowat;
    struct sockaddr_in	mysockaddr;

    memset(&mysockaddr, 0, sizeof(mysockaddr));
    mysockaddr.sin_family = AF_INET;
    mysockaddr.sin_port = htons(SLP_RESERVED_PORT);
    mysockaddr.sin_addr = *addr;
    result = bind(sock,&mysockaddr,sizeof(mysockaddr));
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

/*-------------------------------------------------------------------------*/
SLPDSocket* SLPDSocketNew()
/*-------------------------------------------------------------------------*/
{
    SLPDSocket* sock;
     
    sock = (SLPDSocket*)malloc(sizeof(SLPDSocket));
    if(sock)
    {
        memset(sock,0,sizeof(SLPDSocket));
        sock->fd = -1;
    }

    return sock;
}

/*-------------------------------------------------------------------------*/
void SLPDSocketFree(SLPDSocket* sock)
/*-------------------------------------------------------------------------*/
{
    close(sock->fd);
    if(sock->recvbuf) SLPBufferFree(sock->recvbuf);
    if(sock->sendbuf) SLPBufferFree(sock->sendbuf);                        
    free(sock);
}


/*==========================================================================*/
SLPDSocket* SLPDSocketCreateDatagram(struct in_addr* myaddr,
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

    sock = SLPDSocketNew();
    if(sock)
    {
        sock->fd = socket(PF_INET, SOCK_DGRAM, 0);
        if(sock->fd >=0)
        {
            if(BindSocketToInetAddr(sock->fd, peeraddr) == 0)
            {
                sock->recvbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);  
                sock->sendbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
                sock->peerinfo.peeraddr.sin_addr = *peeraddr;
                sock->peerinfo.peeraddrlen = sizeof(sock->peerinfo.peeraddr);
                sock->peerinfo.peertype = SLPD_PEER_ACCEPTED;     
                
                switch(type)
                {
                case DATAGRAM_MULTICAST:
                    if(JoinSLPMulticastGroup(sock->fd, myaddr) == 0)
                    {
                        sock->state = DATAGRAM_MULTICAST;
                    }
                    else
                    {
                        goto FAILURE;
                    }
                    break;
    
                case DATAGRAM_BROADCAST:
                    if(EnableBroadcast(sock->fd) == 0)
                    {
                        sock->state = DATAGRAM_BROADCAST;
                    }
                    else
                    {
                        goto FAILURE;
                    }
                    break;
    
                case DATAGRAM_UNICAST:
                default:
                    sock->state = DATAGRAM_UNICAST;
                    break;
    
                }
            }
        }
    }

    return sock;

    FAILURE:

    if(sock)
    {
        SLPDSocketFree(sock);
    }
    return 0;
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
    SLPDSocket* sock;

    sock = SLPDSocketNew();
    if(sock)
    {
        sock->fd = socket(PF_INET, SOCK_STREAM, 0);
        if(sock->fd >= 0)
        {          
            if(BindSocketToInetAddr(sock->fd, peeraddr) >= 0)
            {
                if(listen(sock->fd,5) == 0)
                {
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
    struct sockaddr_in  peeraddr;
    int                 fdflags;
    int                 lowat;
    SLPDSocket*         sock = 0;
    
    sock = SLPDSocketNew();
    if(sock == 0)
    {
        goto FAILURE;
    }
    
    /* create the stream socket */
    sock->fd = socket(PF_INET,SOCK_DGRAM,0);
    if(sock->fd < 0)
    {
        goto FAILURE;                        
    }

    /* set the socket to non-blocking */
    fdflags = fcntl(sock->fd, F_GETFL, 0);
    fcntl(sock->fd,F_SETFL, fdflags | O_NONBLOCK);

    /* zero then set peeraddr to connect to */
    memset(&peeraddr, 0, sizeof(peeraddr));
    peeraddr.sin_family = AF_INET;
    peeraddr.sin_port = htons(SLP_RESERVED_PORT);
    peeraddr.sin_addr = *addr;

    /* set the receive and send buffer low water mark to 18 bytes 
    (the length of the smallest slpv2 message) */
    lowat = 18;
    setsockopt(sock->fd,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
    setsockopt(sock->fd,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat));
        
    /* non-blocking connect */
    if(connect(sock->fd, &peeraddr, sizeof(peeraddr)) == 0)   
    {
        /* Connection occured immediately */
        sock->state = STREAM_FIRST_WRITE;
    }
    else
    {
        if(errno == EINPROGRESS)
        {
            /* Connect would have blocked */
            sock->state = STREAM_CONNECT;
        }
        else
        {
            goto FAILURE;
        }                                
    }                   

    return sock;

    /* cleanup on failure */
    FAILURE:
    if (sock)
    {
        close(sock->fd);
        free(sock);
        sock = 0;
    }                   
    
    return sock;
}


/*=========================================================================*/
SLPDSocket* SLPDSocketListAdd(SLPDSocketList* list, SLPDSocket* addition) 
/* Adds a free()s the specified socket from the specified list             */
/*                                                                         */
/* list     - pointer to the SLPSocketList to add the socket to.           */
/*                                                                         */
/* addition - a pointer to the socket to add                               */
/*                                                                         */
/* Returns  - pointer to the added socket or NULL on error                 */
/*=========================================================================*/
{
    ListLink((PListItem*)&list->head,(PListItem)addition);
    list->count = list->count + 1;
    return addition;
}

/*=========================================================================*/
SLPDSocket* SLPDSocketListRemove(SLPDSocketList* list, SLPDSocket* sock)
/* Unlinks and free()s the specified socket from the specified list        */
/*                                                                         */
/* list     - pointer to the SLPSocketList to unlink the socket from.      */
/*                                                                         */
/* sock     - pointer to the SLPSocket to unlink to the list               */
/*                                                                         */
/* Returns  - pointer to the previous socket (may be NULL)                 */
/*=========================================================================*/
{
    SLPDSocket* del = sock;
    sock = (SLPDSocket*)sock->listitem.previous;
    ListUnlink((PListItem*)&list->head,(PListItem)del);
    if(sock == 0)
    {
        sock = list->head;
    }                   
    SLPDSocketFree(del);
    list->count = list->count - 1;
    return sock;
}         


/*=========================================================================*/
void SLPDSocketAge(SLPDSocketList* list, time_t seconds)
/* Age the sockets in the list by the specified number of seconds.  If     */
/* SLPDSocket->lifetime <= 0 it is removed from the list                   */
/*                                                                         */
/* list (IN) pointer to the list to age                                    */
/*                                                                         */
/* seconds (IN) seconds to age each entry of the list                      */
/*=========================================================================*/
{
    SLPDSocket* sock = list->head;
    while(sock)
    {
        /* Do not age certain types of sockets */
        if(sock->state != SOCKET_LISTEN &&
           sock->state != DATAGRAM_UNICAST &&
           sock->state != DATAGRAM_MULTICAST &&
           sock->state != DATAGRAM_BROADCAST)
        {
            sock->age = sock->age + seconds;

            if(list->count > SLPD_COMFORT_SOCKETS)
            {
                if (sock->age > G_SlpdProperty.unicastMaximumWait)
                {
                    
                    /* Log if the removed socket was supposed to be talking */
                    /* a DA                                                 */
                    if( sock->state == STREAM_CONNECT &&
                        sock->peerinfo.peertype == SLPD_PEER_CONNECTED )
                    {
                        SLPLog("WARNING: DA at %s is does not appear to be accepting connections!\n",
                               inet_ntoa(sock->peerinfo.peeraddr.sin_addr));
                    }
                    
                    /* remove the socket from the list */
                    SLPDSocketListRemove(list,sock);
                }                                   
            }
            else
            {
                if (sock->age > SLPD_MAX_SOCKET_LIFETIME)
                {
                    /* close the socket */
                    SLPDSocketListRemove(list,sock);
                }
            }
        }

        sock = (SLPDSocket*)sock->listitem.next;
    }                                                 
}
