/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        libslp_network.c                                           */
/*                                                                         */
/* Abstract:    Implementation for functions that are related to INTERNAL  */
/*              library network (and ipc) communication.                   */
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
time_t      G_LastDADiscovery = 0;
/*=========================================================================*/ 


/*=========================================================================*/
int NetworkConnectToMulticast(struct sockaddr_in* peeraddr)
/*=========================================================================*/
{
    int                 sock;
    
    if(SLPPropertyAsBoolean(SLPGetProperty("net.slp.isBroadcastOnly")))
    {
        sock = SLPNetworkConnectToBroadcast(peeraddr);

    }
    else
    {

        sock = SLPNetworkConnectToMulticast(peeraddr, 
                                            atoi(SLPGetProperty("net.slp.multicastTTL")));    
    }

    return sock;    
}


/*=========================================================================*/ 
int NetworkConnectToDA(const char* scopelist,
                       int scopelistlen,
                       struct sockaddr_in* peeraddr,
                       struct timeval* timeout)
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
/* Returns          Connected socket or -1 if no DA connection can be made */
/*=========================================================================*/
{
    time_t      curtime;
    int         dinterval;
    int         sock;

    sock = KnownDAConnect(scopelist,scopelistlen,peeraddr,timeout);
    if(sock < 0)
    {
        time(&curtime);
        dinterval = atoi(SLPGetProperty("net.slp.DAActiveDiscoveryInterval"));
        if(curtime - G_LastDADiscovery > dinterval)
        {
            KnownDADiscover(timeout);

            sock = KnownDAConnect(scopelist,scopelistlen,peeraddr,timeout);
        }   
    }       

    return sock;
}

/*=========================================================================*/ 
int NetworkConnectToSlpd(struct sockaddr_in* peeraddr)       
/* Connects to slpd and provides a peeraddr to send to                     */
/*                                                                         */
/* peeraddr         (OUT) pointer to receive the connected DA's address    */
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
SLPError NetworkRqstRply(int sock,
                         struct sockaddr_in* destaddr,
                         const char* langtag,
                         char* buf,
                         char buftype,
                         int bufsize,
                         NetworkRqstRplyCallback callback,
                         void * cookie)
/* Transmits and receives SLP messages via multicast convergence algorithm */
/*                                                                         */
/* Returns  -    SLP_OK on success                                         */
/*=========================================================================*/ 
{
    struct timeval      timeout;
    struct sockaddr_in  peeraddr;
    SLPBuffer           sendbuf         = 0;
    SLPBuffer           recvbuf         = 0;
    SLPMessage          msg             = 0;
    SLPError            result          = 0;
    int                 socktype        = 0;
    int                 langtaglen      = 0;
    int                 prlistlen       = 0;
    char*               prlist          = 0;
    int                 xid             = 0;
    int                 mtu             = 0;
    int                 size            = 0;
    int                 xmitcount       = 0;
    int                 rplycount       = 0;
    int                 maxwait         = 0;
    int                 totaltimeout    = 0;
    int                 timeouts[MAX_RETRANSMITS];

    /*----------------------------------------------------*/
    /* Save off a few things we don't want to recalculate */
    /*----------------------------------------------------*/
    langtaglen = strlen(langtag);
    xid = SLPXidGenerate();
    mtu = SLPPropertyAsInteger(SLPGetProperty("net.slp.MTU"));
    sendbuf = SLPBufferAlloc(mtu);
    if(sendbuf == 0)
    {                 
        result = SLP_MEMORY_ALLOC_FAILED;
        goto CLEANUP;
    }

    /* Figure unicast/multicast,TCP/UDP, wait and time out stuff */
    if(ntohl(destaddr->sin_addr.s_addr) > 0xe0000000)
    {
        maxwait = SLPPropertyAsInteger(SLPGetProperty("net.slp.multicastMaximumWait"));
        SLPPropertyAsIntegerVector(SLPGetProperty("net.slp.multicastTimeouts"), 
                                   timeouts, 
                                   MAX_RETRANSMITS );
        socktype = SOCK_DGRAM;
    }
    else
    {
        maxwait = SLPPropertyAsInteger(SLPGetProperty("net.slp.unicastMaximumWait"));
        SLPPropertyAsIntegerVector(SLPGetProperty("net.slp.unicastTimeouts"), 
                                   timeouts, 
                                   MAX_RETRANSMITS );
        size = sizeof(socktype);
        getsockopt(sock,SOL_SOCKET,SO_TYPE,&socktype,&size);
    }
    
    /*--------------------------------*/
    /* Allocate memory for the prlist */
    /*--------------------------------*/
    prlist = (char*)malloc(mtu);
    if(prlist == 0)
    {
        result = SLP_MEMORY_ALLOC_FAILED;
        goto CLEANUP;
    }
    *prlist = 0;
    prlistlen = 0;
    
    
    /*--------------------------*/
    /* Main retransmission loop */
    /*--------------------------*/
    for(xmitcount = 0; xmitcount < MAX_RETRANSMITS; xmitcount++)
    {
        size = 14 + langtaglen + 2 + prlistlen + bufsize;
        if(SLPBufferRealloc(sendbuf,size) == 0)
        {
            result = SLP_MEMORY_ALLOC_FAILED;
            goto CLEANUP;
        }

        /*-----------------------------------*/
        /* Add the header to the send buffer */
        /*-----------------------------------*/
        /*version*/
        *(sendbuf->start)       = 2;
        /*function id*/
        *(sendbuf->start + 1)   = buftype;
        /*length*/
        ToUINT24(sendbuf->start + 2, size);
        /*flags*/
        ToUINT16(sendbuf->start + 5, socktype == SOCK_STREAM ? 0 : SLP_FLAG_MCAST);
        /*ext offset*/
        ToUINT24(sendbuf->start + 7,0);
        /*xid*/
        ToUINT16(sendbuf->start + 10,xid);
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
        memcpy(sendbuf->curpos, buf, bufsize);
        
        /*----------------------*/
        /* send the send buffer */
        /*----------------------*/
        result = SLPNetworkSendMessage(sock,
                                       sendbuf,
                                       destaddr,
                                       &timeout);
        if(result != 0)
        {
            /* we could not send the message for some reason */
            /* we're done */
            result = SLP_NETWORK_ERROR;
            goto FINISHED;
        }
            
        /*--------------------*/
        /* setup recv timeout */
        /*--------------------*/
        if(socktype == SOCK_DGRAM)
        {   
            totaltimeout += timeouts[xmitcount];
            if(totaltimeout >= maxwait || timeouts[xmitcount] == 0)
            {
                break;
            }
            timeout.tv_sec = timeouts[xmitcount] / 1000;
            timeout.tv_usec = (timeouts[xmitcount] % 1000) * 1000;
        }
        else
        {
            timeout.tv_sec = maxwait / 1000;
            timeout.tv_usec = (maxwait % 1000) * 1000;
        }

        /*----------------*/
        /* Main recv loop */
        /*----------------*/
        while(1)
        {
            
            if(SLPNetworkRecvMessage(sock,
                                     &recvbuf,
                                     &peeraddr,
                                     &timeout) != 0)
            {
                /* An error occured while receiving the message */
                /* probably a just time out error. Retry send.  */
                break;
            }
             
            /* Parse the message and call callback */
            msg = SLPMessageRealloc(msg);
            if(msg == 0)
            {
                result = SLP_MEMORY_ALLOC_FAILED;
                goto FINISHED;
            }

            if(SLPMessageParseBuffer(recvbuf, msg) == 0)
            {
                if (msg->header.xid == xid)
                {
                    rplycount = rplycount + 1;
                    if(callback(msg, cookie) == 0)
                    {
                        goto CLEANUP;
                    }
                }
            }
            
            if(socktype == SOCK_STREAM)
            {
                goto FINISHED;
            }

            /* add the peer to the previous responder list */
            if(prlistlen != 0)
            {
                strcat(prlist,",");
            }
            strcat(prlist,inet_ntoa(peeraddr.sin_addr));
            prlistlen =  strlen(prlist); 
        }
    }

    FINISHED:
    /*----------------*/
    /* We're all done */
    /*----------------*/
    if(rplycount == 0)
    {
        result = SLP_NETWORK_TIMED_OUT;
    }
    
    
    /*----------------*/
    /* Free resources */
    /*----------------*/
    CLEANUP:
    if(prlist) free(prlist);
    SLPBufferFree(sendbuf);
    SLPBufferFree(recvbuf);
    SLPMessageFree(msg);
    close(sock);

    return result;
}



