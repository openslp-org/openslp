/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slplib_network.c                                           */
/*                                                                         */
/* Abstract:    Implementation for INTERNAL functions that are related     */
/*              network (and ipc) communication.                           */
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

#include "slp.h"
#include "libslp.h"

/*=========================================================================*/ 
struct sockaddr_in  G_SlpdAddr;
int                 G_SlpdSocket  = -1;
/*=========================================================================*/    
    
/*=========================================================================*/ 
int NetworkGetSlpdSocket(struct sockaddr* peeraddr, int peeraddrlen)       
/* Connects to slpd.  Will not block.                                      */
/*                                                                         */
/* peeraddr     (OUT) receives the address of the socket                   */
/*                                                                         */
/* peeraddrlen  (OUT) receives the length of the socket address            */
/*                                                                         */
/* Returns  -   -1 if connection can not be made                           */
/*=========================================================================*/ 
{
    int lowat;

    if(G_SlpdSocket == -1)
    {
        G_SlpdSocket = socket(AF_INET,SOCK_STREAM,0);
        if(G_SlpdSocket >= 0)
        {
            G_SlpdAddr.sin_family      = AF_INET;
            G_SlpdAddr.sin_port        = htons(SLP_RESERVED_PORT);
            G_SlpdAddr.sin_addr.s_addr = htonl(LOOPBACK_ADDRESS);
            if(connect(G_SlpdSocket,(struct sockaddr*)&G_SlpdAddr,sizeof(G_SlpdAddr)) == 0)
            {
                 /* set the receive and send buffer low water mark to 18 bytes 
                (the length of the smallest slpv2 message) */
                lowat = 18;
                setsockopt(G_SlpdSocket,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
                setsockopt(G_SlpdSocket,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat));

                if(peeraddrlen >= sizeof(G_SlpdAddr))
                {
                    memcpy(peeraddr,&G_SlpdAddr,sizeof(G_SlpdAddr));
                }
                else
                {

                    memcpy(peeraddr,&G_SlpdAddr,peeraddrlen);
                }
                
            }
            else
            {
                  /* Could not connect to the slpd through the loopback */
                  close(G_SlpdSocket);
                  G_SlpdSocket = -1;
            }
        }    
    }
    else
    {
        if(peeraddrlen >= sizeof(G_SlpdAddr))
        {
            memcpy(peeraddr,&G_SlpdAddr,sizeof(G_SlpdAddr));
        }
        else
        {

            memcpy(peeraddr,&G_SlpdAddr,peeraddrlen);
        }
    }
    
    return G_SlpdSocket;
}


/*=========================================================================*/ 
void NetworkCloseSlpdSocket()
/*=========================================================================*/ 
{
    close(G_SlpdSocket);
    G_SlpdSocket = -1;
}


/*=========================================================================*/ 
SLPError NetworkSendMessage(int sockfd,
                           SLPBuffer buf,
                           struct timeval* timeout,
                           struct sockaddr* peeraddr,
                           int peeraddrlen)
/* Returns  -    SLP_OK, SLP_NETWORK_TIMEOUT, SLP_NETWORK_ERROR, or        */
/*               SLP_PARSE_ERROR.                                          */
/*=========================================================================*/ 
{
    fd_set      writefds;
    int         xferbytes;
    SLPError    result      = SLP_OK;

    while(buf->curpos < buf->end)
    {
        FD_ZERO(&writefds);
        FD_SET(sockfd, &writefds);

        xferbytes = select(sockfd + 1, 0, &writefds, 0, timeout);
        if(xferbytes > 0)
        {
            xferbytes = sendto(sockfd,
                               buf->curpos, 
                               buf->end - buf->curpos, 
                               0,
                               peeraddr,
                               sizeof(struct sockaddr_in));
            if(xferbytes > 0)
            {
                buf->curpos = buf->curpos + xferbytes;
            }
            else
            {
                result = SLP_NETWORK_ERROR;
                break;
            }
        }
        else if(xferbytes == 0)
        {
            result = SLP_NETWORK_TIMED_OUT;
            break;
        }
        else
        {
            result = SLP_NETWORK_ERROR;
            break;
        }
    }
    
    return result;
}



/*=========================================================================*/ 
SLPError NetworkRecvMessage(int sockfd,
                           SLPBuffer buf,
                           struct timeval* timeout,
                           struct sockaddr* peeraddr,
                           int* peeraddrlen)
/* Receives a message                                                      */
/*                                                                         */
/* Returns  -    SLP_OK, SLP_NETWORK_TIMEOUT, SLP_NETWORK_ERROR, or        */
/*               SLP_PARSE_ERROR.                                          */
/*=========================================================================*/ 
{
    SLPError    result      = SLP_OK;
    int         xferbytes;
    fd_set      readfds;
    char        peek[16];
    
    /*---------------------------------------------------------------*/
    /* take a peek at the packet to get version and size information */
    /*---------------------------------------------------------------*/
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    xferbytes = select(sockfd + 1, &readfds, 0 , 0, timeout);
    if(xferbytes > 0)
    {
        xferbytes = recvfrom(sockfd,
                             peek,
                             16,
                             MSG_PEEK,
                             peeraddr,
                             peeraddrlen);
        if(xferbytes <= 0)
        {
            result = SLP_NETWORK_ERROR;
        } 
    }
    else if(xferbytes == 0)
    {
        result = SLP_NETWORK_TIMED_OUT;
    }
    else
    {
        result = SLP_NETWORK_ERROR;
    }

    /*---------------------------------------*/
    /* return now if peek was not successful */
    /*---------------------------------------*/
    if(result) return result;

    
    /*------------------------------*/
    /* Read the rest of the message */
    /*------------------------------*/
    /* check the version */
    if(*peek == 2)
    {
        /* allocate the recvmsg big enough for the whole message */
        if(SLPBufferRealloc(buf, AsUINT24(peek + 2)))
        {
            while(buf->curpos < buf->end)
            {
                FD_ZERO(&readfds);
                FD_SET(sockfd, &readfds);
                xferbytes = select(sockfd + 1, &readfds, 0 , 0, timeout);
                if(xferbytes > 0)
                {
                     xferbytes = recv(sockfd,
                                   buf->curpos, 
                                   buf->end - buf->curpos, 
                                   0);
                    if(xferbytes > 0)
                    {
                        buf->curpos = buf->curpos + xferbytes;
                    }
                    else
                    {
                        result =  SLP_NETWORK_ERROR;
                        break;
                    }
                }
                else if(xferbytes == 0)
                {
                    result = SLP_NETWORK_TIMED_OUT;
                    break;
                }
                else
                {
                    result =  SLP_NETWORK_ERROR;
                    break;
                }
            } /* end of main read while. */  
        }
        else
        {
            result = SLP_MEMORY_ALLOC_FAILED;
        }
    }
    else
    {
        result = SLP_PARSE_ERROR;
    }

    return result;
}


/*=========================================================================*/ 
int NetworkConnectToDA(struct sockaddr_in* peeraddr)
/*=========================================================================*/ 
{
    /*
    int lowat;
    lowat = 18;
    setsockopt(G_SlpdSocket,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
    setsockopt(G_SlpdSocket,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat));
    */
    return -1;
}


/*=========================================================================*/ 
int NetworkConnectToSlpMulticast(struct sockaddr_in* peeraddr)
/* Creates a socket and connects it to the SLP multicast address           */
/*                                                                         */
/* Returns  - Valid file descriptor on success, -1 on failure w/ errno set.*/
/*=========================================================================*/ 
{
    int                 sockfd;
    int                 ttl;
    
    /* connect to the SA (through the loopback) */
    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd >= 0)
    {
        peeraddr->sin_family = AF_INET;
        peeraddr->sin_port = htons(SLP_RESERVED_PORT);
        peeraddr->sin_addr.s_addr = htonl(SLP_MCAST_ADDRESS);

        /* TODO: Set TTL from conf file*/
        ttl = 8;
        setsockopt(sockfd,
                   IPPROTO_IP,
                   IP_MULTICAST_TTL,
                   &ttl,
                   sizeof(ttl));
    }

    return sockfd;
}

/*=========================================================================*/ 
int NetworkConnectToSlpBroadcast(struct sockaddr_in* peeraddr)
/* Creates a socket and connects it to the SLP multicast address           */
/*                                                                         */
/* Returns  - Valid file descriptor on success, -1 on failure w/ errno set.*/
/*=========================================================================*/ 
{
    /* TODO: implement broadcast later */
    return -1;
}
