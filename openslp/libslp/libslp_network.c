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

/*-------------------------------------------------------------------------*/
int LocateDAviaProperty(char** daaddresses)
/*-------------------------------------------------------------------------*/
{
    const char* property = SLPGetProperty("net.slp.DAAddresses");
    if(property && *property)
    {
        *daaddresses = strdup(property);
        if(*daaddresses)
        {
            return 0;
        }
    }
    
    return -1;
}

/*-------------------------------------------------------------------------*/
int LocateDAviaDHCP(char** daaddresses)
/*-------------------------------------------------------------------------*/
{
    return -1;
}             

/*-------------------------------------------------------------------------*/
int LocateDAviaSlpd(char** daaddresses)
/* Returns  zero if daaddresses found. <0 if slpd is not running and >0 if */
/*          slpd is running but no DAs were found.                         */
/*-------------------------------------------------------------------------*/
{
    return -1;
}

/*-------------------------------------------------------------------------*/
int LocateDAviaMulticast(char** daaddresses)
/*-------------------------------------------------------------------------*/
{
    return -1;
}


/*=========================================================================*/ 
int NetworkConnectToDA(const char* scopelist,
                       int scopelistlen,
                       struct sockaddr_in* peeraddr)
/* Connects to slpd and provides a peeraddr to send to                     */
/*                                                                         */
/* scopelist        (IN) Scope that must be supported by DA. Pass in NULL  */
/*                       for any scope                                     */
/*                                                                         */
/* scopelistlen     (IN) Length of the scope list in bytes.  Ignored if    */
/*                       scopelist is NULL                                 */
/*                                                                         */
/* peeraddr         (OUT) pointer to receive the connected DA's address    */
/*                                                                         */
/* peeraddrlen      (IN/OUT) Size of the peeraddr structure                */
/*                                                                         */
/* Returns          Connected socket or -1 if no DA connection can be made */
/*=========================================================================*/
{
    struct hostent*     dahostent;
    char*               begin;
    char*               end;
    int                 finished;
    int                 lowat;
    int                 result      = -1;
    char*               daaddresses = 0;

    
    /*-------------*/
    /* Locate a DA */
    /*-------------*/
    if(LocateDAviaProperty(&daaddresses) < 0)
    {
        if(LocateDAviaDHCP(&daaddresses) < 0)
        {
            if(LocateDAviaSlpd(&daaddresses) < 0)
            {
                if(LocateDAviaMulticast(&daaddresses) < 0)
                {
                    return -1;
                }
            }
        }
    }

    /*------------------------------------------------*/
    /* perform connect to the first da in daaddresses */
    /*------------------------------------------------*/
    result = socket(AF_INET,SOCK_STREAM,0);
    if(result >= 0)
    {
        begin = (char*)daaddresses;
        end = begin;
        finished = 0;
        while( finished == 0 )
        {
            while(*end && *end != ',') end ++;
            if(*end == 0) finished = 1;
            while(*end <=0x2f) 
            {
                *end = 0;
                end--;
            }
             
            memset(&peeraddr,0,sizeof(struct sockaddr_in));
            peeraddr->sin_family = AF_INET;
            peeraddr->sin_port = htons(SLP_RESERVED_PORT);
            dahostent = gethostbyname(begin);
            if(dahostent)
            {
                peeraddr->sin_addr.s_addr = *(unsigned long*)(dahostent->h_addr_list[0]);
            }
            else
            {
                peeraddr->sin_addr.s_addr = inet_addr(begin);
            }
    
            /* TODO: Make this connect non-blocking so that it will timeout */
            if(connect(result,(struct sockaddr*)&peeraddr,sizeof(peeraddr)) == 0)
            {
                /* set the receive and send buffer low water mark to 18 bytes 
                (the length of the smallest slpv2 message) */
                lowat = 18;
                setsockopt(result,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
                setsockopt(result,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat));
                goto FINISHED;
            }
        }

        close(result);
    }
    
    FINISHED:
    if(daaddresses)
    {
        free(daaddresses);
    }
    
    return result;
}


/*=========================================================================*/ 
int NetworkConnectToSlpd(struct sockaddr_in* peeraddr)       
/* Connects to slpd and provides a peeraddr to send to                     */
/*                                                                         */
/* peeraddr         (OUT) pointer to receive the connected DA's address    */
/*                                                                         */
/* peeraddrlen      (IN/OUT) Size of the peeraddr structure                */
/*                                                                         */
/* Returns          Connected socket or -1 if no DA connection can be made */
/*=========================================================================*/
{
    int lowat;
    int result;
     
    result = socket(AF_INET,SOCK_STREAM,0);
    if(result >= 0)
    {
        peeraddr->sin_family      = AF_INET;
        peeraddr->sin_port        = htons(SLP_RESERVED_PORT);
        peeraddr->sin_addr.s_addr = htonl(LOOPBACK_ADDRESS);
        if(connect(result,
                   (struct sockaddr*)peeraddr,
                   sizeof(struct sockaddr_in)) == 0)
        {
             /* set the receive and send buffer low water mark to 18 bytes 
            (the length of the smallest slpv2 message) */
            lowat = 18;
            setsockopt(result,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
            setsockopt(result,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat));
        }
        else
        {
              /* Could not connect to the slpd through the loopback */
              close(result);
        }
    }    
    
    return result;
}


/*=========================================================================*/ 
int NetworkConnectToSlpMulticast(struct sockaddr_in* peeraddr)
/* Creates a socket and provides a peeraddr to send to                     */
/*                                                                         */
/* peeraddr         (OUT) pointer to receive the connected DA's address    */
/*                                                                         */
/* peeraddrlen      (IN/OUT) Size of the peeraddr structure                */
/*                                                                         */
/* Returns          Valid socket or -1 if no DA connection can be made     */
/*=========================================================================*/
{
    
    const char*         bcastonly;
    int                 sockfd;
    int                 ttl;

    bcastonly = SLPGetProperty("net.slp.isBroadcastOnly"); 
    if(*bcastonly == 'T' ||
       *bcastonly == 't' ||
       *bcastonly == 'Y' ||
       *bcastonly == 'y')
    {
        return -1;
    }

    /* setup multicast socket */
    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd >= 0)
    {
        peeraddr->sin_family = AF_INET;
        peeraddr->sin_port = htons(SLP_RESERVED_PORT);
        peeraddr->sin_addr.s_addr = htonl(SLP_MCAST_ADDRESS);

        ttl = atoi(SLPGetProperty("net.slp.multicastTTL"));
        if(setsockopt(sockfd,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl)))
        {
            return -1;
        }
    }

    return sockfd;
}

/*=========================================================================*/ 
int NetworkConnectToSlpBroadcast(struct sockaddr_in* peeraddr)
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
SLPError NetworkSendMessage(int sockfd,
                           SLPBuffer buf,
                           struct timeval* timeout,
                           struct sockaddr_in* peeraddr)
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
                           struct sockaddr_in* peeraddr)
/* Receives a message                                                      */
/*                                                                         */
/* Returns  -    SLP_OK, SLP_NETWORK_TIMEOUT, SLP_NETWORK_ERROR, or        */
/*               SLP_PARSE_ERROR.                                          */
/*=========================================================================*/ 
{
    int         xferbytes;
    fd_set      readfds;
    char        peek[16];
    int         peeraddrlen = sizeof(struct sockaddr_in);
    SLPError    result      = SLP_OK;

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
