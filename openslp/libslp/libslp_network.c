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
int NetworkConnectToMulticast(struct sockaddr_in* peeraddr)
/*=========================================================================*/
{
    int                 sock = -1;
    
    if(SLPPropertyAsBoolean(SLPGetProperty("net.slp.isBroadcastOnly")) == 0)
    {
        sock = SLPNetworkConnectToMulticast(peeraddr, 
                                            atoi(SLPGetProperty("net.slp.multicastTTL")));
    }
    
    if (sock < 0)
    {
        sock = SLPNetworkConnectToBroadcast(peeraddr);
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
#ifdef WIN32 /* on WIN32 setsockopt takes a const char * argument */
  char lowat;
#else
    int lowat;
#endif
    int result;

    result = socket(AF_INET,SOCK_STREAM,0);
    if(result >= 0)
    {
        peeraddr->sin_family      = AF_INET;
        peeraddr->sin_port        = htons(SLP_RESERVED_PORT);
        peeraddr->sin_addr.s_addr = htonl(LOOPBACK_ADDRESS);
        
        /* TODO: the following connect() could block for a long time.  */

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
              result = -1;
        }
    }

    return result;
}

/*=========================================================================*/ 
void NetworkDisconnectDA(PSLPHandleInfo handle)
/* Called after DA fails to respond                                        */
/*                                                                         */
/* handle   (IN) a handle previously passed to NetworkConnectToDA()        */
/*=========================================================================*/ 
{
    if(handle->dasock) 
    {
        close(handle->dasock);
        handle->dasock = -1;
    }
    
}


/*=========================================================================*/ 
void NetworkDisconnectSA(PSLPHandleInfo handle)
/* Called after SA fails to respond                                        */
/*                                                                         */
/* handle   (IN) a handle previously passed to NetworkConnectToSA()        */
/*=========================================================================*/ 
{
    if(handle->sasock) 
    {
        close(handle->sasock);
        handle->sasock = -1;
    }
    
}

/*=========================================================================*/ 
int NetworkConnectToDA(PSLPHandleInfo handle,
                       const char* scopelist,
                       int scopelistlen,
                       struct sockaddr_in* peeraddr)
/* Connects to slpd and provides a peeraddr to send to                     */
/*                                                                         */
/* handle           (IN) SLPHandle info  (caches connection info           */
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
    /*-----------------------------------------------------------------*/
    /* attempt to use a cached socket if scope is supported  otherwise */
    /* discover a DA that supports the scope                           */
    /*-----------------------------------------------------------------*/
    if(handle->dasock >= 0 &&
       handle->dascope &&
       SLPCompareString(handle->dascopelen,
                        handle->dascope,
                        scopelistlen,
                        scopelist) == 0)
    {
        memcpy(peeraddr,&(handle->daaddr),sizeof(struct sockaddr_in));
    }
    else
    {
        if( handle->dasock > 0)
        {
            close(handle->dasock);
        }

        handle->dasock = KnownDAConnect(scopelistlen,scopelist,&(handle->daaddr));
        if(handle->dasock)
        {
            if(handle->dascope) free(handle->dascope);
            handle->dascope = memdup(scopelist,scopelistlen);
            handle->dascopelen = scopelistlen; 
            memcpy(peeraddr,&(handle->daaddr),sizeof(struct sockaddr_in));
        }
    }

    return handle->dasock;
}

/*=========================================================================*/ 
int NetworkConnectToSA(PSLPHandleInfo handle,
                       const char* scopelist,
                       int scopelistlen,
                       struct sockaddr_in* peeraddr)
/* Connects to slpd and provides a peeraddr to send to                     */
/*                                                                         */
/* handle           (IN) SLPHandle info  (caches connection info)          */
/*                                                                         */
/* scopelist        (IN) Scope that must be supported by SA. Pass in NULL  */
/*                       for any scope                                     */
/*                                                                         */
/* scopelistlen     (IN) Length of the scope list in bytes.  Ignored if    */
/*                       scopelist is NULL                                 */
/*                                                                         */
/* peeraddr         (OUT) pointer to receive the connected SA's address    */
/*                                                                         */
/* Returns          Connected socket or -1 if no SA connection can be made */ 
{
    
    /*-----------------------------------------------------------------*/
    /* attempt to use a cached socket if scope is supported  otherwise */
    /* look to connect to local slpd or a DA that supports the scope   */
    /*-----------------------------------------------------------------*/
    if(handle->sasock >= 0 &&
       handle->sascope && 
       SLPCompareString(handle->sascopelen,
                        handle->sascope,
                        scopelistlen,
                        scopelist) == 0)
    {
        memcpy(peeraddr,&(handle->saaddr),sizeof(struct sockaddr_in));
    }
    else
    {
        /*-----------------------------------------*/
        /* Attempt to connect to slpd via loopback */
        /*-----------------------------------------*/
        handle->sasock = NetworkConnectToSlpd(&(handle->saaddr));
        if(handle->sasock < 0)
        {
            /*----------------------------*/
            /* Attempt to connect to a DA */                        
            /*----------------------------*/  
            handle->sasock = NetworkConnectToDA(handle,
                                                scopelist,
                                                scopelistlen,
                                                &(handle->saaddr));
        }

        /*----------------------------------------------------------*/
        /* if we connected to something, cache scope and addr info  */
        /*----------------------------------------------------------*/
        if(handle->sasock >= 0)
        {
            if(handle->sascope) free(handle->sascope);
            handle->sascope = memdup(scopelist,scopelistlen);
            handle->sascopelen = scopelistlen; 
            memcpy(peeraddr,&(handle->saaddr),sizeof(struct sockaddr_in));
        }                                                                 
    }
    
    return handle->sasock;
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
    int                 looprecv        = 0;
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
#ifdef WIN32 /* on WIN32 setsockopt takes a const char * argument */
    char                socktype        = 0;
#else
    int                 socktype        = 0;
#endif
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
    if(ISMCAST(destaddr->sin_addr) )
    {
        /* Multicast or broadcast */
        maxwait = SLPPropertyAsInteger(SLPGetProperty("net.slp.multicastMaximumWait"));
        SLPPropertyAsIntegerVector(SLPGetProperty("net.slp.multicastTimeouts"), 
                                   timeouts, 
                                   MAX_RETRANSMITS );
        xmitcount = 0;
        looprecv = 1;
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
        if(socktype == SOCK_DGRAM)
        {
            xmitcount = 0;
            looprecv  = 1;
        }
        else
        {
            xmitcount = MAX_RETRANSMITS;
            looprecv  = 0;
        }
    }

    /* Special case for fake SLP_FUNCT_DASRVRQST */
    if(buftype == SLP_FUNCT_DASRVRQST)
    {
        /* do something special for SRVRQST that will be discovering DAs */
        maxwait = SLPPropertyAsInteger(SLPGetProperty("net.slp.DADiscoveryMaximumWait"));
        SLPPropertyAsIntegerVector(SLPGetProperty("net.slp.DADiscovertTimeouts"),
                                   timeouts,
                                   MAX_RETRANSMITS );
        /* SLP_FUNCT_DASRVRQST is a fake function.  We really want to */
        /* send a SRVRQST                                             */
        buftype  = SLP_FUNCT_SRVRQST;
        looprecv = 1;
    }
    
    
    /*--------------------------------*/
    /* Allocate memory for the prlist */
    /*--------------------------------*/
    if( buftype == SLP_FUNCT_SRVRQST ||
        buftype == SLP_FUNCT_ATTRRQST ||
        buftype == SLP_FUNCT_SRVTYPERQST)
    {
        prlist = (char*)malloc(mtu);
        if(prlist == 0)
        {
            result = SLP_MEMORY_ALLOC_FAILED;
            goto CLEANUP;
        }
        *prlist = 0;
        prlistlen = 0;
    }
    
    
    /*--------------------------*/
    /* Main retransmission loop */
    /*--------------------------*/
    while(xmitcount <= MAX_RETRANSMITS)
    {
        xmitcount++;

        /*--------------------*/
        /* setup recv timeout */
        /*--------------------*/
        if(socktype == SOCK_DGRAM)
        {   
            totaltimeout += timeouts[xmitcount];
            if(totaltimeout >= maxwait || timeouts[xmitcount] == 0)
            {
                /* we are all done */
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
        
        /*--------------------*/
        /* re-allocate buffer */
        /*--------------------*/
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
        ToUINT16(sendbuf->start + 5, ISMCAST(destaddr->sin_addr) ? SLP_FLAG_MCAST : 0);
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
        if(prlist)
        {
            ToUINT16(sendbuf->curpos,prlistlen);
            sendbuf->curpos = sendbuf->curpos + 2;
            memcpy(sendbuf->curpos, prlist, prlistlen);
            sendbuf->curpos = sendbuf->curpos + prlistlen;
        }
         
        /*-----------------------------*/
        /* Add the rest of the message */
        /*-----------------------------*/
        memcpy(sendbuf->curpos, buf, bufsize);
        
        /*----------------------*/
        /* send the send buffer */
        /*----------------------*/
        result = SLPNetworkSendMessage(sock,
                                       socktype,
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
            
 
        /*----------------*/
        /* Main recv loop */
        /*----------------*/
        do
        {
            if(SLPNetworkRecvMessage(sock,
                                     socktype,
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
                    if(callback(result, msg, cookie) == 0)
                    {
                        goto CLEANUP;
                    }
                }
            }
            
            /* add the peer to the previous responder list */
            if(prlistlen != 0)
            {
                strcat(prlist,",");
            }
            strcat(prlist,inet_ntoa(peeraddr.sin_addr));
            prlistlen =  strlen(prlist);

        }while(looprecv);
    }

    FINISHED:
    /*----------------*/
    /* We're all done */
    /*----------------*/
    if(rplycount == 0)
    {
        result = SLP_NETWORK_TIMED_OUT;
    }

    /*-------------------------------------*/
    /* Notify the callback that we're done */
    /*-------------------------------------*/
    callback(SLP_LAST_CALL,msg,cookie);
    
    /*----------------*/
    /* Free resources */
    /*----------------*/
    CLEANUP:
    if(prlist) free(prlist);
    SLPBufferFree(sendbuf);
    SLPBufferFree(recvbuf);
    SLPMessageFree(msg);

    return result;
}
