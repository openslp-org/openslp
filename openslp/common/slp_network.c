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

#include <slp_network.h>

/*=========================================================================*/ 
int SLPNetworkConnectStream(struct sockaddr_in* peeraddr, 
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
#ifdef WIN32
  char lowat;
#else
    int lowat;
#endif
    int result;

    /* TODO: Make this connect non-blocking so that it will timeout */
    
    result = socket(AF_INET,SOCK_STREAM,0);
    if(result >= 0)
    {
        if(connect(result,
                   (struct sockaddr*)peeraddr,
                   sizeof(struct sockaddr_in)) == 0)
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
int SLPNetworkConnectToMulticast(struct sockaddr_in* peeraddr, int ttl)
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
    int 		optarg;
#else
    /* Solaris and Tru64 expect a unsigned char parameter */
    unsigned char	optarg;
#endif
    
    /* setup multicast socket */
    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd >= 0)
    {
        peeraddr->sin_family = AF_INET;
        peeraddr->sin_port = htons(SLP_RESERVED_PORT);
        peeraddr->sin_addr.s_addr = htonl(SLP_MCAST_ADDRESS);
	optarg = ttl;
        
        if(setsockopt(sockfd,IPPROTO_IP,IP_MULTICAST_TTL,&optarg,sizeof(optarg)))
        {
            return -1;
        }
    }

    return sockfd;
}

/*=========================================================================*/ 
int SLPNetworkConnectToBroadcast(struct sockaddr_in* peeraddr)
/* Creates a socket and provides a peeraddr to send to                     */
/*                                                                         */
/* peeraddr         (OUT) pointer to receive the connected DA's address    */                                                       
/*                                                                         */
/* Returns          Valid socket or -1 if no DA connection can be made     */
/*=========================================================================*/
{
    int                 sockfd;
#ifdef WIN32
    char on = 1;
#else
    int                 on = 1;
#endif

    /* setup broadcast */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd >= 0)
    {
        peeraddr->sin_family = AF_INET;
        peeraddr->sin_port = htons(SLP_RESERVED_PORT);
        peeraddr->sin_addr.s_addr = htonl(SLP_BCAST_ADDRESS);

        if(setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)))
        {
            return -1;
        }
    }

    return sockfd;
}


/*=========================================================================*/ 
int SLPNetworkSendMessage(int sockfd,
                          SLPBuffer buf,
                          struct sockaddr_in* peeraddr,
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
            xferbytes = sendto(sockfd,
                               buf->curpos, 
                               buf->end - buf->curpos, 
                               flags,
                               (struct sockaddr *)peeraddr,
                               sizeof(struct sockaddr_in));
            
            if(xferbytes > 0)
            {
                buf->curpos = buf->curpos + xferbytes;
            }
            else
            {
#ifndef WIN32
                errno = EPIPE;
#endif
                return -1;
            }
        }
        else if(xferbytes == 0)
        {
            /* timed out */
#ifndef WIN32
            errno = ETIMEDOUT;
#endif
            return -1;
        }
        else
        {
#ifndef WIN32
            errno = EPIPE;
#endif
            return -1;
        }
    }
    
    return 0;
}



/*=========================================================================*/ 
int SLPNetworkRecvMessage(int sockfd,
                          SLPBuffer* buf,
                          struct sockaddr_in* peeraddr,
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
    int         peeraddrlen = sizeof(struct sockaddr_in);
    
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
                             (struct sockaddr *)peeraddr,
                             &peeraddrlen);
        if(xferbytes <= 0)
        {
#ifndef WIN32
            errno = ENOTCONN;
#endif
            return -1;
        } 
    }
    else if(xferbytes == 0)
    {
#ifndef WIN32
        errno = ETIMEDOUT;
#endif
        return -1;
    }
    else
    {
#ifndef WIN32
        errno = ENOTCONN;
#endif
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
#ifndef WIN32
                        errno = ENOTCONN;
#endif
                        return -1;
                    }
                }
                else if(xferbytes == 0)
                {
#ifndef WIN32
                    errno = ETIMEDOUT;
#endif
                    return -1;
                }
                else
                {
#ifndef WIN32
                    errno =  ENOTCONN;
#endif
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
