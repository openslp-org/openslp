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
/* addr     - pointer to the multicast address                             */
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
    
    mysockaddr.sin_family = AF_INET;
    mysockaddr.sin_port = htons(SLP_RESERVED_PORT);
    mysockaddr.sin_addr = *addr;
    result = bind(sock,&mysockaddr,sizeof(mysockaddr));

    /* set the receive and send buffer low water mark to 18 bytes 
    (the length of the smallest slpv2 message) */
    lowat = 18;
    setsockopt(sock,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
    setsockopt(sock,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat));
    
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
/* Returns  - pointer to the removed socket                                */
/*=========================================================================*/
{
    ListUnlink((PListItem*)&list->head,(PListItem)sock);
    list->count = list->count - 1;
    return sock;
}         


/*=========================================================================*/
void SLPDSocketInit(SLPDSocketList* list)
/* Adds SLPSockets (UDP and TCP) for all the interfaces and the loopback   */
/*                                                                         */
/* list     - pointer to SLPSocketList to initialize                       */
/*                                                                         */
/* Returns  - zero on success, -1 on failure.                              */
/*=========================================================================*/
{
    char*           begin;
    char*           end;
    int             finished;
    struct in_addr  myaddr;
    struct in_addr  mcastaddr;
    struct in_addr  bcastaddr;
    SLPDSocket*     sock;

    /*----------------------------------------------------*/
    /* Decide what address to use for multicast/broadcast */
    /*----------------------------------------------------*/
    mcastaddr.s_addr = htonl(SLP_MCAST_ADDRESS);
    bcastaddr.s_addr = htonl(0xffffffff);     

    

    /*-----------------------------------------------------------------*/
    /* Create SOCKET_LISTEN socket for LOOPBACK for the library to talk to*/
    /*-----------------------------------------------------------------*/
    sock = (SLPDSocket*)malloc(sizeof(SLPDSocket));
    if(sock == 0)
    {
        return;
    }
    memset(sock,0,sizeof(SLPDSocket));

    sock->fd = socket(PF_INET, SOCK_STREAM, 0);
    if(sock->fd >= 0)
    {          
        if(BindSocketToLoopback(sock->fd) >= 0)
        {
            if(listen(sock->fd,5) == 0)
            {
                sock->state = SOCKET_LISTEN;
                if(SLPDSocketListAdd(list,sock) == 0)
                {
                    SLPFatal("Out of memory");
                }

                SLPLog("SLPLIB API socket listening\n");
            }
            else
            {
                /* could not listen(), close the socket*/
                close(sock->fd);
                free(sock);
                SLPLog("ERROR: Could not listen on loopback\n");
                SLPLog("ERROR: No SLPLIB support will be available\n");
            }
        }
        else
        {
            /* could not bind, close the socket*/
            close(sock->fd);
            free(sock);
            SLPLog("ERROR: Could not bind loopback port 427.\n");
            SLPLog("ERROR: slpd may already be running\n");
        }
    }
    else
    {
        /* could not create the socket */
        free(sock);
        SLPLog("ERROR: Could not create socket for loopback.\n");
        SLPLog("ERROR: No SLPLIB support will be available\n");
    }

                                                                                   
    /*---------------------------------------------------------------------*/
    /* Create sockets for all of the interfaces in the interfaces property */
    /*---------------------------------------------------------------------*/
    begin = (char*)G_SlpdProperty.interfaces;
    end = begin;
    finished = 0;
    while( finished == 0)
    {
        while(*end && *end != ',') end ++;
        if(*end == 0) finished = 1;
        while(*end <=0x2f) 
        {
            *end = 0;
            end--;
        }

        /* begin now points to a null terminated ip address string */
        myaddr.s_addr = inet_addr(begin);

        /*--------------------------------------------------------*/
        /* Create socket that will handle multicast UDP           */
        /*--------------------------------------------------------*/
    
        sock = (SLPDSocket*)malloc(sizeof(SLPDSocket));
        if(sock == 0)
        {
            break;
        }
        sock->fd = socket(PF_INET, SOCK_DGRAM, 0);
        if(sock->fd >=0)
        {
            if(BindSocketToInetAddr(sock->fd, &mcastaddr) >= 0)
            {
                if(JoinSLPMulticastGroup(sock->fd, &myaddr) == 0)
                {
                    sock->state = DATAGRAM_MULTICAST;
                    sock->recvbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);  
                    sock->sendbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
                    sock->peerinfo.peeraddrlen = sizeof(sock->peerinfo.peeraddr);
                    sock->peerinfo.peertype = SLPD_PEER_REMOTE; 
                    if(sock->recvbuf == 0 || sock->sendbuf == 0)
                    {
                        SLPFatal("SLPD out of memory !!\n");
                    }
                    SLPDSocketListAdd(list,sock);
                    SLPLog("Multicast socket on %s ready\n",inet_ntoa(myaddr));
                }
                else
                {
                    /* could not add multicast membership */
                    close(sock->fd);
                    free(sock);
                }
            }
            else
            {
                /* could not bind(), close the socket*/
                close(sock->fd);
                free(sock);
            }
        }

        /*--------------------------------------------*/
        /* Create socket that will handle unicast UDP */
        /*--------------------------------------------*/
        sock = (SLPDSocket*)malloc(sizeof(SLPDSocket));
        if(sock == 0)
        {
            break;
        }
        sock->fd = socket(PF_INET, SOCK_DGRAM, 0);
        if(sock->fd >= 0)
        {          
            if(BindSocketToInetAddr(sock->fd, &myaddr) >= 0)
            {
                sock->state = DATAGRAM_UNICAST;
                sock->recvbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);  
                sock->sendbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);  
                sock->peerinfo.peertype = SLPD_PEER_REMOTE; 
                if(sock->recvbuf == 0 || sock->sendbuf == 0)
                {
                    SLPFatal("SLPD out of memory !!\n");
                }
                SLPDSocketListAdd(list,sock);
                SLPLog("UDP socket on %s ready\n",inet_ntoa(myaddr));
            }
            else
            {
                /* could not bind(), close the socket*/
                close(sock->fd);
                free(sock);
            }
        }

        /*------------------------------------------------*/
        /* Create TCP_LISTEN that will handle unicast TCP */
        /*------------------------------------------------*/
        sock = (SLPDSocket*)malloc(sizeof(SLPDSocket));
        if(sock == 0)
        {
            break;
        }
        sock->fd = socket(PF_INET, SOCK_STREAM, 0);
        if(sock->fd >= 0)
        {          
            if(BindSocketToInetAddr(sock->fd, &myaddr) >= 0)
            {
                if(listen(sock->fd,2) == 0)
                {
                    sock->state = SOCKET_LISTEN;
                    if(SLPDSocketListAdd(list,sock) == 0)
                    {
                        SLPFatal("Out of memory");
                    } 
                    
                    SLPLog("TCP socket on %s listening\n",inet_ntoa(myaddr));
                }
                else
                {
                    /* could not listen(), close the socket*/
                    close(sock->fd);
                    free(sock);
                }
            }
            else
            {
                /* could not bind, close the socket*/
                close(sock->fd);
                free(sock);   
            }
        }
        
        begin = end + 1;
    }     

    /*--------------------------------------------------------*/
    /* Create socket that will handle broadcast UDP           */
    /*--------------------------------------------------------*/
    
    sock = (SLPDSocket*)malloc(sizeof(SLPDSocket));
    if(sock == 0)
    {
        return;
    }
    sock->fd = socket(PF_INET, SOCK_DGRAM, 0);
    if(sock->fd >=0)
    {
        if(BindSocketToInetAddr(sock->fd, &bcastaddr) >= 0)
        {
            if(EnableBroadcast(sock->fd) == 0)
            {
                sock->state = DATAGRAM_BROADCAST;
                sock->recvbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);  
                sock->sendbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
                sock->peerinfo.peeraddrlen = sizeof(sock->peerinfo.peeraddr);
                sock->peerinfo.peertype = SLPD_PEER_REMOTE; 
                if(sock->recvbuf == 0 || sock->sendbuf == 0)
                {
                    SLPFatal("SLPD out of memory !!\n");
                }
                SLPDSocketListAdd(list,sock);
                SLPLog("Broadcast socket for %s ready\n", inet_ntoa(bcastaddr));
            }
            else
            {
                /* could not add multicast membership */
                close(sock->fd);
                free(sock);
            }
        }
        else
        {
            /* could not bind(), close the socket*/
            close(sock->fd);
            free(sock);
        }
    }

}


/*=========================================================================*/
void SLPDSocketDeInit(SLPDSocketList* list)
/* Adds SLPSockets (UDP and TCP) for all the interfaces and the loopback   */
/*                                                                         */
/* list     - pointer to SLPSocketList to deinitialize                     */
/*                                                                         */
/* Returns  - zero on success, -1 on failure.                              */
/*=========================================================================*/
{
    /* TODO remove and free all socket list resources */
};



