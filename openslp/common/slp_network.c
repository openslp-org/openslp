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
    int lowat;
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
    
    /* setup multicast socket */
    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd >= 0)
    {
        peeraddr->sin_family = AF_INET;
        peeraddr->sin_port = htons(SLP_RESERVED_PORT);
        peeraddr->sin_addr.s_addr = htonl(SLP_MCAST_ADDRESS);
        
        if(setsockopt(sockfd,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl)))
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
/* peeraddrlen      (IN/OUT) Size of the peeraddr structure                */
/*                                                                         */
/* Returns          Valid socket or -1 if no DA connection can be made     */
/*=========================================================================*/
{
    int                 sockfd;
    int                 on = 1;

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
                               MSG_NOSIGNAL,
                               peeraddr,
                               sizeof(struct sockaddr_in));
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
            errno = ETIME;
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
                          SLPBuffer buf,
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
                             peeraddr,
                             &peeraddrlen);
        if(xferbytes <= 0)
        {
            errno = ENOTCONN;
            return -1;
        } 
    }
    else if(xferbytes == 0)
    {
        errno = ETIME;
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
                        errno = ENOTCONN;
                        return -1;
                    }
                }
                else if(xferbytes == 0)
                {
                    errno = ETIME;
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


/*=========================================================================*/ 
int SLPNetworkMcastConverge(const SLPNetworkMcastInfo *mcastinfo,
                            const char* langtag,
                            char buftype,
                            const void* buf,
                            int* bufsize,
                            SLPBuffer* rplybufs,
                            int* rplybufcount)
/* Receives a message                                                      */
/*                                                                         */
/* Returns  -    zero on success, non-zero on failure                      */
/*                                                                         */
/*               EPIPE error during write                                  */
/*               ETIME read timed out                                      */
/*               ENOMEM out of memory                                      */
/*=========================================================================*/ 
{
    struct timeval      timeout;
    struct sockaddr_in  peeraddr;
    SLPBuffer           sendbuf         = 0;
    int                 result          = 0;
    int                 sock            = 0;
    int                 langtaglen      = 0;
    int                 prlistlen       = 0;
    char*               prlist          = 0;
    int                 size            = 0;
    int                 xmitcount       = 0;
    int                 rplycount       = 0;
    int                 totaltimeout    = 0;
    
    /*-----------------------------*/
    /* Hook up a socket to talk on */
    /*-----------------------------*/
    if(mcastinfo->broadcast)
    {
        sock = SLPNetworkConnectToBroadcast(&peeraddr);
    }
    else
    {
        sock = SLPNetworkConnectToMulticast(&peeraddr,
                                            mcastinfo->ttl);
    }                                
    if(sock < 0)
    {
        result = EPIPE;
        goto FINISHED;
    }

    /*--------------------------------*/
    /* Allocate memory for the prlist */
    /*--------------------------------*/
    prlist = (char*)malloc(mcastinfo->mtu);
    if(prlist == 0)
    {
        result = ENOMEM;
        goto FINISHED;
    }
    *prlist = 0;
    prlistlen = 0;
    
    /*----------------------------------------------------*/
    /* Save off a few things we don't want to recalculate */
    /*----------------------------------------------------*/
    sendbuf = SLPBufferAlloc(mcastinfo->mtu);
    if(sendbuf == 0)
    {
        result = ENOMEM;
        goto FINISHED;
    }
    langtaglen = strlen(langtag);

    /*--------------------------*/
    /* Main retransmission loop */
    /*--------------------------*/
    for(xmitcount = 0; xmitcount < MAX_RETRANSMITS; xmitcount++)
    {
        size = 14 + langtaglen + 2 + prlistlen + *bufsize;
        if(SLPBufferRealloc(sendbuf,size) == 0)
        {
            goto FINISHED;
        }

        /*----------------------------------*/
        /* Add the header to the send buffer*/
        /*----------------------------------*/
        /*version*/
        *(sendbuf->start)       = 2;
        /*function id*/
        *(sendbuf->start + 1)   = buftype;
        /*length*/
        ToUINT24(sendbuf->start + 2, size);
        /*flags*/
        ToUINT16(sendbuf->start + 5, SLP_FLAG_MCAST);
        /*ext offset*/
        ToUINT24(sendbuf->start + 7,0);
        /*xid*/
        ToUINT16(sendbuf->start + 10,SLPXidGenerate());
        /*lang tag len*/
        ToUINT16(sendbuf->start + 12,langtaglen);
        /*lang tag*/
        memcpy(sendbuf->start + 14, langtag, langtaglen);
        sendbuf->curpos = sendbuf->start + langtaglen + 14 ;

        /*-----------------------------------*/
        /* Add the prlist to the send buffer */
        /*-----------------------------------*/
        ToUINT16(sendbuf->curpos,prlistlen);
        sendbuf->curpos = sendbuf->curpos + 2;
        memcpy(sendbuf->curpos, prlist, prlistlen);
        sendbuf->curpos = sendbuf->curpos + prlistlen;
         
        /*-----------------------------*/
        /* Add the rest of the message */
        /*-----------------------------*/
        memcpy(sendbuf->curpos, buf, *bufsize);
        
        /*----------------------*/
        /* send the send buffer */
        /*----------------------*/
        result = SLPNetworkSendMessage(sock,
                                       sendbuf,
                                       &peeraddr,
                                       &timeout);
        if(result != 0)
        {
            /* we could not send the message for some reason */
            /* we're done */
            result = EPIPE;
            break;
        }
            
        /*--------------------*/
        /* setup recv timeout */
        /*--------------------*/
        if(mcastinfo->timeouts[xmitcount] == 0)
        {
            break;
        }                  
        timeout.tv_sec = mcastinfo->timeouts[xmitcount] / 1000;
        timeout.tv_usec = (mcastinfo->timeouts[xmitcount] % 1000) * 1000;

        /*----------------*/
        /* Main recv loop */
        /*----------------*/
        for(;rplycount < *rplybufcount; rplycount++)
        {
            
            if(SLPNetworkRecvMessage(sock,
                                     rplybufs[rplycount],
                                     &peeraddr,
                                     &timeout) != 0)
            {
                /* An error occured while receiving the message */
                /* probably a just time out error. Retry send.  */
                break;
            }
             
            /* add the peer to the previous responder list */
            if(prlistlen != 0)
            {
                strcat(prlist,",");
            }
            strcat(prlist,inet_ntoa(peeraddr.sin_addr));
            prlistlen =  strlen(prlist); 
        }

        /*----------------------------------------------*/
        /* Make some final checks to see if we are done */
        /*----------------------------------------------*/
        totaltimeout += mcastinfo->timeouts[xmitcount];
        if(totaltimeout >= mcastinfo->maxwait)
        {
            break;
        }
        if(rplycount >= *rplybufcount);
        {
            break;
        }
    }

    /*----------------*/
    /* We're all done */
    /*----------------*/
    if(rplycount == 0)
    {
        result = ETIME;
    }
    *rplybufcount = rplycount;
    

    /*----------------*/
    /* Free resources */
    /*----------------*/
    FINISHED:
    if(prlist) free(prlist);
    SLPBufferFree(sendbuf);
    close(sock);

    return result;
}

