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

#include "slp.h"
#include "libslp.h"
#include "slp_net.h"

int NetworkGetMcastAddrs(const char msgtype, const char *msg, SLPIfaceInfo *ifaceinfo);

/*=========================================================================*/
int NetworkConnectToSlpd(struct sockaddr_storage* peeraddr)
/* Connects to slpd and provides a peeraddr to send to                     */
/*                                                                         */
/* peeraddr         (OUT) pointer to receive the connected DA's address    */
/*                                                                         */
/* Returns          Connected socket or -1 if no DA connection can be made */
/*=========================================================================*/
{
    int sock = -1;

	if (SLPNetIsIPV6()) {
		if (SLPNetSetAddr(peeraddr, AF_INET6, SLP_RESERVED_PORT, (unsigned char *)&slp_in6addr_loopback, sizeof(struct in6_addr)) == 0)
			sock = SLPNetworkConnectStream(peeraddr,NULL);
	}
	if (sock < 0 && SLPNetIsIPV4()) {
		int tempAddr = INADDR_LOOPBACK;
		if (SLPNetSetAddr(peeraddr, AF_INET, SLP_RESERVED_PORT, (unsigned char *)&tempAddr, sizeof(struct in_addr)) == 0)
			sock = SLPNetworkConnectStream(peeraddr,NULL);
	}
	return sock;
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
#ifdef _WIN32
	    closesocket(handle->dasock);
#else
	    close(handle->dasock);
#endif
        handle->dasock = -1;
    }

    /* Mark this DA as bad */
    KnownDABadDA(&(handle->daaddr));
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
#ifdef _WIN32
	    closesocket(handle->sasock);
#else
	    close(handle->sasock);
#endif
        handle->sasock = -1;
    }
}

/*=========================================================================*/ 
int NetworkConnectToDA(PSLPHandleInfo handle,
                       const char* scopelist,
                       int scopelistlen,
                       struct sockaddr_storage* peeraddr)
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
        memcpy(peeraddr,&(handle->daaddr),sizeof(struct sockaddr_storage));
    }
    else
    {
        /* close handle cause it can't support the scope */
        if(handle->dasock >= 0)
        {
#ifdef _WIN32
	        closesocket(handle->dasock);
#else
	        close(handle->dasock);
#endif
        }

        /* Attempt to connect to DA that does support the scope */
        handle->dasock = KnownDAConnect(handle,
                                        scopelistlen,
                                        scopelist,
                                        &(handle->daaddr));
        if(handle->dasock >= 0)
        {
            if(handle->dascope) xfree(handle->dascope);
            handle->dascope = memdup(scopelist,scopelistlen);
            handle->dascopelen = scopelistlen; 
            memcpy(peeraddr,&(handle->daaddr),sizeof(struct sockaddr_storage));
        }
    }

    return handle->dasock;
}

/*=========================================================================*/ 
int NetworkConnectToSA(PSLPHandleInfo handle,
                       const char* scopelist,
                       int scopelistlen,
                       struct sockaddr_storage* peeraddr)
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
        memcpy(peeraddr,&(handle->saaddr),sizeof(struct sockaddr_storage));
    }
    else
    {
        /* close handle cause it can't support the scope */
        if(handle->sasock >= 0)
        {
#ifdef _WIN32
	        closesocket(handle->sasock);
#else
	        close(handle->sasock);
#endif
        }

        /*-----------------------------------------*/
        /* Attempt to connect to slpd via loopback */
        /*-----------------------------------------*/
        handle->sasock = NetworkConnectToSlpd(&(handle->saaddr));

        /*----------------------------------------------------------*/
        /* if we connected to something, cache scope and addr info  */
        /*----------------------------------------------------------*/
        if(handle->sasock >= 0)
        {
            if(handle->sascope) xfree(handle->sascope);
            handle->sascope = memdup(scopelist,scopelistlen);
            handle->sascopelen = scopelistlen; 
            memcpy(peeraddr,&(handle->saaddr),sizeof(struct sockaddr_storage));
        }
    }

    return handle->sasock;
}



/*=========================================================================*/ 
SLPError NetworkRqstRply(int sock,
                         struct sockaddr_storage* destaddr,
                         const char* langtag,
                         int extoffset,
                         char* buf,
                         char buftype,
                         int bufsize,
                         NetworkRplyCallback callback,
                         void * cookie)
/* Transmits and receives SLP messages via multicast convergence algorithm */
/*                                                                         */
/* Returns  -    SLP_OK on success                                         */
/*=========================================================================*/ 
{
    struct timeval			timeout;
    struct sockaddr_storage  peeraddr;
    SLPBuffer				sendbuf         = 0;
    SLPBuffer				recvbuf         = 0;
    SLPError				result          = 0;
    int						looprecv        = 0;
    int						langtaglen      = 0;
    int						prlistlen       = 0;
    char*					prlist          = 0;
    int						xid             = 0;
    int						mtu             = 0;
    int						size            = 0;
    int						xmitcount       = 0;
    int						rplycount       = 0;
    int						maxwait         = 0;
    int						totaltimeout    = 0;
#ifdef _WIN32 /* on WIN32 setsockopt takes a const char * argument */
    char					socktype        = 0;
#else
    int						socktype        = 0;
#endif
    int						timeouts[MAX_RETRANSMITS];
    unsigned short			flags;


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
    if(SLPNetIsMCast(destaddr))
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
        SLPPropertyAsIntegerVector(SLPGetProperty("net.slp.DADiscoveryTimeouts"),
                                   timeouts,
                                   MAX_RETRANSMITS );
        /* SLP_FUNCT_DASRVRQST is a fake function.  We really want to */
        /* send a SRVRQST                                             */
        buftype  = SLP_FUNCT_SRVRQST;
        looprecv = 1;
    }

    /*---------------------------------------------------------------------*/
    /* Allocate memory for the prlist for appropriate messages.            */
    /* Notice that the prlist is as large as the MTU -- thus assuring that */
    /* there will not be any buffer overwrites regardless of how many      */
    /* previous responders there are.   This is because the retransmit     */
    /* code terminates if ever MTU is exceeded for any datagram message.   */
    /*---------------------------------------------------------------------*/
    if(buftype == SLP_FUNCT_SRVRQST ||
       buftype == SLP_FUNCT_ATTRRQST ||
       buftype == SLP_FUNCT_SRVTYPERQST)
    {
        prlist = (char*)xmalloc(mtu);
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

        /*------------------------------------------------------------------*/
        /* re-allocate buffer and make sure that the send buffer does not   */
        /* exceed MTU for datagram transmission                             */
        /*------------------------------------------------------------------*/
        size = 14 + langtaglen + bufsize;
        if(buftype == SLP_FUNCT_SRVRQST ||
           buftype == SLP_FUNCT_ATTRRQST ||
           buftype == SLP_FUNCT_SRVTYPERQST)
        {
            /* add in room for the prlist */
            size += 2 + prlistlen;
        }
        if(size > mtu && socktype == SOCK_DGRAM)
        {
            if(xmitcount == 0)
            {
                result = SLP_BUFFER_OVERFLOW;
            }
            goto FINISHED;
        }
        if((sendbuf = SLPBufferRealloc(sendbuf,size)) == 0)
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
        flags = (SLPNetIsMCast(destaddr) ? SLP_FLAG_MCAST : 0);
        if (buftype == SLP_FUNCT_SRVREG)
        {
            flags |= SLP_FLAG_FRESH;
        }
        ToUINT16(sendbuf->start + 5, flags);
        /*ext offset*/
        /* TRICKY: the extoffset passed into us was the offset not
         * including the header.  We need to fix up the offset so
         * that it is from the beginning of the SLP message
         */
        if(extoffset != 0)
	{
            ToUINT24(sendbuf->start + 7,extoffset + langtaglen + 14);
	}
        else
        {
	    ToUINT24(sendbuf->start + 7, 0);
	}
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
            if(errno == ETIMEDOUT)
            {
                result = SLP_NETWORK_TIMED_OUT;
            }
            else
            {
                result = SLP_NETWORK_ERROR;    
            }
            goto FINISHED;
        }

        /*----------------*/
        /* Main recv loop */
        /*----------------*/
        do
        {
			peeraddr.ss_family = AF_UNSPEC;

            if(SLPNetworkRecvMessage(sock,
                                     socktype,
                                     &recvbuf,
                                     &peeraddr,
                                     &timeout) != 0)
            {
                /* An error occured while receiving the message        */
                /* probably a just time out error. break for re-send.  */
                if(errno == ETIMEDOUT)
                {
                    result = SLP_NETWORK_TIMED_OUT;
                }
                else
                {
                    result = SLP_NETWORK_ERROR;
                }

                break;
            }
            else
            {
                /* Sneek in and check the XID */
                if(AsUINT16(recvbuf->start+10) == xid)
                {
                    rplycount += 1;

					/* Check the family type of the peeraddr. If it is not set, 
					   assume the dstaddr is the peeraddr. peeraddr is only
					   set on SOCK_DGM sockets in the SLPNetworkRecvMessage() 
					   call. */
					if (peeraddr.ss_family == AF_UNSPEC)
						memcpy(&peeraddr, destaddr, sizeof(struct sockaddr_storage));

                    /* Call the callback with the result and recvbuf */
                    if(callback(result,&peeraddr,recvbuf,cookie) == SLP_FALSE)
                    {
                        /* Caller does not want any more info */
                        /* We are done!                       */
                        goto CLEANUP;
                    }
                    
                    /* add the peer to the previous responder list          */
                    /* Note that prlist will be NULL if message type is not */
                    /* SLP_FUNCT_SRVRQST, SLP_FUNCT_ATTRRQST, or            */
                    /* SLP_FUNCT_SRVTYPERQST)                               */
                    if(prlist && socktype == SOCK_DGRAM)
                    {
                        /* calculate the peeraddr string and length */
                        char peeraddrstr[INET6_ADDRSTRLEN];
                        
                        peeraddrstr[0] = '\0';
                        if (peeraddr.ss_family == AF_INET) {
                            inet_ntop(peeraddr.ss_family, &((struct sockaddr_in*) &peeraddr)->sin_addr, peeraddrstr, INET6_ADDRSTRLEN);
                        }
                        else if (peeraddr.ss_family == AF_INET6) {
                            inet_ntop(peeraddr.ss_family, &((struct sockaddr_in6*) &peeraddr)->sin6_addr, peeraddrstr, INET6_ADDRSTRLEN);
                        }
						if (strcmp(peeraddrstr, "") != 0) 
                        {
                            int peeraddrstrlen = strlen(peeraddrstr);
                            
                            /* Append to the prlist if we won't overflow */
                            if((prlistlen + peeraddrstrlen + 1) < mtu )
                            {
                                /* append comma if necessary */
                                if(prlistlen != 0)
                                {
                                    strcat(prlist,",");
                                    prlistlen ++;
                                }
                                /* append address string */
                                strcat(prlist,peeraddrstr);
                                prlistlen += peeraddrstrlen;
                            }
                        }
                    }
                }
            }

        }while(looprecv);
    }

    FINISHED:
    
    /*-----------------------------------------------*/
    /* Notify the last time callback that we're done */
    /*-----------------------------------------------*/

    if(rplycount)
    {
        result = SLP_LAST_CALL; 
    }
    
    if(result == SLP_NETWORK_TIMED_OUT && SLPNetIsMCast(destaddr))
    {
        result = SLP_LAST_CALL;
    }

    callback(result, &peeraddr, recvbuf, cookie);
    
    if(result == SLP_LAST_CALL)
    {
        result = 0;
    }

    /*----------------*/
    /* Free resources */
    /*----------------*/
    CLEANUP:
    if(prlist) xfree(prlist);
    SLPBufferFree(sendbuf);
    SLPBufferFree(recvbuf);
    
    return result;
}


/*=========================================================================*/ 
#ifndef MI_NOT_SUPPORTED
SLPError NetworkMcastRqstRply(PSLPHandleInfo handle,
#else
SLPError NetworkMcastRqstRply(const char* langtag,
#endif /* MI_NOT_SUPPORTED */
                              char* buf,
                              char buftype,
                              int bufsize,
                              NetworkRplyCallback callback,
                              void * cookie)
/* Description:                                                            */
/*                                                                         */
/* Broadcasts/multicasts SLP messages via multicast convergence algorithm  */
/*                                                                         */
/* langtag  (IN) Language tag to use in SLP message header                 */
/*                                                                         */
/* buf      (IN) pointer to the portion of the SLP message to send. The    */
/*               portion to that should be pointed to is everything after  */
/*               the pr-list. NetworkXcastRqstRply() automatically         */
/*               generates the header and the prlist.                      */
/*                                                                         */
/* buftype  (IN) the function-id to use in the SLPMessage header           */
/*                                                                         */
/* bufsize  (IN) the size of the buffer pointed to by buf                  */
/*                                                                         */
/* callback (IN) the callback to use for reporting results                 */
/*                                                                         */
/* cookie   (IN) the cookie to pass to the callback                        */
/*                                                                         */
/* Returns  -    SLP_OK on success. SLP_ERROR on failure                   */
/*=========================================================================*/ 
{
    struct timeval			timeout;
    struct sockaddr_storage peeraddr;
    SLPBuffer				sendbuf         = 0;
    SLPBuffer				recvbuf         = 0;
    SLPError				result          = 0;
    int						langtaglen      = 0;
    int						prlistlen       = 0;
    char*					prlist          = 0;
    int						xid             = 0;
    int						mtu             = 0;
    int						size            = 0;
    int						xmitcount       = 0;
    int						rplycount       = 0;
    int						maxwait         = 0;
    int						totaltimeout    = 0;
    int						usebroadcast    = 0;
    int						timeouts[MAX_RETRANSMITS];
    SLPIfaceInfo			dstifaceinfo;
	SLPIfaceInfo			v4outifaceinfo;
	SLPIfaceInfo			v6outifaceinfo;
    SLPXcastSockets			xcastsocks;
	int						currIntf		= 0;
    int                     requestSent;

#ifdef DEBUG
    /* This function only supports multicast or broadcast of the following
     *  messages
     */
    if(buftype != SLP_FUNCT_SRVRQST &&
       buftype != SLP_FUNCT_ATTRRQST &&
       buftype != SLP_FUNCT_SRVTYPERQST &&
       buftype != SLP_FUNCT_DASRVRQST)
    {
        return SLP_PARAMETER_BAD;
    }
#endif

    /*----------------------------------------------------*/
    /* Save off a few things we don't want to recalculate */
    /*----------------------------------------------------*/
#ifndef MI_NOT_SUPPORTED
    langtaglen = strlen(handle->langtag);
#else
    langtaglen = strlen(langtag);
#endif /* MI_NOT_SUPPORTED */
    xid = SLPXidGenerate();
    mtu = SLPPropertyAsInteger(SLPGetProperty("net.slp.MTU"));
    sendbuf = SLPBufferAlloc(mtu);
    if(sendbuf == 0)
    {
        result = SLP_MEMORY_ALLOC_FAILED;
        goto FINISHED;
    }

	v4outifaceinfo.iface_count = 0;
	v6outifaceinfo.iface_count = 0;
	xcastsocks.sock_count = 0;
    
#ifndef MI_NOT_SUPPORTED
	/* Determine which multicast addresses to send to. */
	NetworkGetMcastAddrs(buftype, buf, &dstifaceinfo);
	/* Determine which interfaces to send out on. */
    if(handle->McastIFList != NULL) 
    {
        #ifdef DEBUG
        fprintf(stderr, "McastIFList = %s\n", handle->McastIFList);
        #endif
        SLPIfaceGetInfo(handle->McastIFList, &v4outifaceinfo, AF_INET);
		SLPIfaceGetInfo(handle->McastIFList, &v6outifaceinfo, AF_INET6);
    }
	else { 
#endif /* MI_NOT_SUPPORTED */
	if (SLPNetIsIPV4())
		SLPIfaceGetInfo(SLPGetProperty("net.slp.interfaces"),&v4outifaceinfo, AF_INET);
	if (SLPNetIsIPV6())
	   SLPIfaceGetInfo(SLPGetProperty("net.slp.interfaces"),&v6outifaceinfo, AF_INET6);
	if (v4outifaceinfo.iface_count == 0 && v6outifaceinfo.iface_count == 0) {
        result = SLP_NETWORK_ERROR;
        goto FINISHED;
    }


#ifndef MI_NOT_SUPPORTED
	}
#endif
    usebroadcast = SLPPropertyAsBoolean(SLPGetProperty("net.slp.useBroadcast"));

    /*-----------------------------------*/
    /* Multicast/broadcast wait timeouts */
    /*-----------------------------------*/
    maxwait = SLPPropertyAsInteger(SLPGetProperty("net.slp.multicastMaximumWait"));
    SLPPropertyAsIntegerVector(SLPGetProperty("net.slp.multicastTimeouts"), 
                               timeouts, 
                               MAX_RETRANSMITS );

    /* Special case for fake SLP_FUNCT_DASRVRQST */
    if(buftype == SLP_FUNCT_DASRVRQST)
    {
        /* do something special for SRVRQST that will be discovering DAs */
        maxwait = SLPPropertyAsInteger(SLPGetProperty("net.slp.DADiscoveryMaximumWait"));
        SLPPropertyAsIntegerVector(SLPGetProperty("net.slp.DADiscoveryTimeouts"),
                                   timeouts,
                                   MAX_RETRANSMITS );
        /* SLP_FUNCT_DASRVRQST is a fake function.  We really want to */
        /* send a SRVRQST                                             */
        buftype  = SLP_FUNCT_SRVRQST;
    }

    /*---------------------------------------------------------------------*/
    /* Allocate memory for the prlist for appropriate messages.            */
    /* Notice that the prlist is as large as the MTU -- thus assuring that */
    /* there will not be any buffer overwrites regardless of how many      */
    /* previous responders there are.   This is because the retransmit     */
    /* code terminates if ever MTU is exceeded for any datagram message.   */
    /*---------------------------------------------------------------------*/
    prlist = (char*)xmalloc(mtu);
    if(prlist == 0)
    {
        result = SLP_MEMORY_ALLOC_FAILED;
        goto FINISHED;
    }
    *prlist = 0;
    prlistlen = 0; 

#ifndef MI_NOT_SUPPORTED
	/* Iterate through each multicast scope until we found a provider. */
	while (currIntf < dstifaceinfo.iface_count) {
#endif
		/*--------------------------*/
		/* Main retransmission loop */
		/*--------------------------*/
		xmitcount = 0;
		totaltimeout = 0;
        requestSent = 0;
		while(xmitcount <= MAX_RETRANSMITS)
		{
			xmitcount++;

			totaltimeout += timeouts[xmitcount];
			if(totaltimeout >= maxwait ||  timeouts[xmitcount] == 0)
			{
				/* we are all done */
				break;
			}
			timeout.tv_sec = timeouts[xmitcount] / 1000;
			timeout.tv_usec = (timeouts[xmitcount] % 1000) * 1000;
	        
			/*------------------------------------------------------------------*/
			/* re-allocate buffer and make sure that the send buffer does not   */
			/* exceed MTU for datagram transmission                             */
			/*------------------------------------------------------------------*/
			size = 14 + langtaglen + bufsize;
			if(buftype == SLP_FUNCT_SRVRQST ||
			buftype == SLP_FUNCT_ATTRRQST ||
			buftype == SLP_FUNCT_SRVTYPERQST)
			{
				/* add in room for the prlist */
				size += 2 + prlistlen;
			}
			if(size > mtu)
			{
				if(xmitcount == 0)
				{
					result = SLP_BUFFER_OVERFLOW;
				}
				goto FINISHED;
			}
			if((sendbuf = SLPBufferRealloc(sendbuf,size)) == 0)
			{
				result = SLP_MEMORY_ALLOC_FAILED;
				goto FINISHED;
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
			ToUINT16(sendbuf->start + 5, SLP_FLAG_MCAST);
			/*ext offset*/
			ToUINT24(sendbuf->start + 7,0);
			/*xid*/
			ToUINT16(sendbuf->start + 10,xid);
			/*lang tag len*/
			ToUINT16(sendbuf->start + 12,langtaglen);
			/*lang tag*/
	#ifndef MI_NOT_SUPPORTED
			memcpy(sendbuf->start + 14, handle->langtag, langtaglen);
	#else
			memcpy(sendbuf->start + 14, langtag, langtaglen);
	#endif /* MI_NOT_SUPPORTED */
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
			if(usebroadcast)
			{
				result = SLPBroadcastSend(&v4outifaceinfo,sendbuf,&xcastsocks);
			}
			else
			{
				if (dstifaceinfo.iface_addr[currIntf].ss_family == AF_INET)
					result = SLPMulticastSend(&v4outifaceinfo,sendbuf,&xcastsocks, &dstifaceinfo.iface_addr[currIntf]);
				else if (dstifaceinfo.iface_addr[currIntf].ss_family == AF_INET6)
					result = SLPMulticastSend(&v6outifaceinfo,sendbuf,&xcastsocks, &dstifaceinfo.iface_addr[currIntf]);
			}
			if(result == 0)
			{
                requestSent = 1;
			}

			/*----------------*/
			/* Main recv loop */
			/*----------------*/
			while(1)
			{
				#ifndef UNICAST_NOT_SUPPORTED
				int retval = 0;
			if((retval = SLPXcastRecvMessage(&xcastsocks,
									&recvbuf,
									&peeraddr,
									&timeout)) != 0)
				#else
				
			if(SLPXcastRecvMessage(&xcastsocks,
									&recvbuf,
									&peeraddr,
									&timeout) != 0)
				#endif

				{
					/* An error occured while receiving the message        */
					/* probably a just time out error. break for re-send.  */
					if(errno == ETIMEDOUT)
					{
						result = SLP_NETWORK_TIMED_OUT;
					}
					else
					{
						result = SLP_NETWORK_ERROR;
					}
	#ifndef UNICAST_NOT_SUPPORTED
					/* retval = SLP_RETRY_UNICAST signifies that we received a
			* multicast packet of size > MTU and hence we are now sending
			* a unicast request to this IP-address
			*/
			if ( retval == SLP_RETRY_UNICAST ) 
			{
				int tcpsockfd, retval1, retval2, unicastwait = 0;
				unicastwait = SLPPropertyAsInteger(SLPGetProperty("net.slp.unicastMaximumWait"));
				timeout.tv_sec = unicastwait / 1000;
				timeout.tv_usec = (unicastwait % 1000) * 1000;
			
				tcpsockfd = SLPNetworkConnectStream(&peeraddr, &timeout);
				if ( tcpsockfd >= 0 ) 
				{
					ToUINT16(sendbuf->start + 5, SLP_FLAG_UCAST);
				xid = SLPXidGenerate();
				ToUINT16(sendbuf->start + 10,xid);
				   
				retval1 = SLPNetworkSendMessage(tcpsockfd, SOCK_STREAM, sendbuf, &peeraddr, &timeout);
				if ( retval1 != 0 ) 
				{
					/* we could not send the message for some reason */
					/* we close the TCP connection and break */
					if(errno == ETIMEDOUT) 
					{
						result = SLP_NETWORK_TIMED_OUT;
					} 
					else 
					{
						result = SLP_NETWORK_ERROR;
					}
#ifdef _WIN32
	                closesocket(tcpsockfd);
#else
	                close(tcpsockfd);
#endif
	                             
					break;
				}
				    
				retval2 = SLPNetworkRecvMessage(tcpsockfd, SOCK_STREAM, &recvbuf, &peeraddr, &timeout);
				if ( retval2 != 0 ) 
				{
					/* An error occured while receiving the message */
					/* probably a just time out error. break for re-send.  */
					if(errno == ETIMEDOUT) 
					{
						result = SLP_NETWORK_TIMED_OUT;
					} 
					else 
					{
					result = SLP_NETWORK_ERROR;
					}
				
#ifdef _WIN32
	                closesocket(tcpsockfd);
#else
	                close(tcpsockfd);
#endif
					break;
				}
				 
#ifdef _WIN32
	            closesocket(tcpsockfd);
#else
	            close(tcpsockfd);
#endif
				result = SLP_OK;
				goto SNEEK;			                        
				} 
				else 
				{
				/* Unsuccessful in opening a TCP connection */
				/* just break and retry everything */
				break;			                       
				}               
			}
			else
			{
	#endif
					break;
	#ifndef UNICAST_NOT_SUPPORTED
			}
	#endif
			}
	#ifndef UNICAST_NOT_SUPPORTED
				SNEEK:
	#endif
				/* Sneek in and check the XID */
				if(AsUINT16(recvbuf->start+10) == xid)
				{
					char peeraddrstr[INET6_ADDRSTRLEN];
					rplycount += 1;

					/* Call the callback with the result and recvbuf */
	#ifndef MI_NOT_SUPPORTED
					if (cookie == NULL)
					{
						cookie = (PSLPHandleInfo)handle;
					}
	#endif /* MI_NOT_SUPPORTED */
			        if(callback(result,&peeraddr,recvbuf,cookie) == SLP_FALSE)
					{
						/* Caller does not want any more info */
						/* We are done!                       */
						goto CLEANUP;
					}
	                 
					/* add the peer to the previous responder list */
					if(prlistlen != 0)
					{
						strcat(prlist,",");
					}
                    peeraddrstr[0] = '\0';
                    if (peeraddr.ss_family == AF_INET) {
                        inet_ntop(peeraddr.ss_family, &((struct sockaddr_in*) &peeraddr)->sin_addr, peeraddrstr, INET6_ADDRSTRLEN);
                    }
                    else if (peeraddr.ss_family == AF_INET6) {
                        inet_ntop(peeraddr.ss_family, &((struct sockaddr_in6*) &peeraddr)->sin6_addr, peeraddrstr, INET6_ADDRSTRLEN);
                    }
					if (strcmp(peeraddrstr, "") != 0) 
                    {
					    strcat(prlist, peeraddrstr);
					    prlistlen = strlen(prlist);
                    }
				}
			}

			SLPXcastSocketsClose(&xcastsocks);
		}
#ifndef MI_NOT_SUPPORTED
		currIntf++;
	}
#endif


    FINISHED:
    /*---------------------------------------------------------------------*/
    /* Notify the callback with SLP_LAST_CALL so that they know we're done */
    /*---------------------------------------------------------------------*/
    if(rplycount || result == SLP_NETWORK_TIMED_OUT)
    {
        result = SLP_LAST_CALL;
    }
    
#ifndef MI_NOT_SUPPORTED
    if (cookie == NULL)
    {
        cookie = (PSLPHandleInfo)handle;
    }
#endif /* MI_NOT_SUPPORTED */

    callback(result, NULL,NULL,cookie);

    if(result == SLP_LAST_CALL)
    {
        result = SLP_OK;
    }
    
    
    CLEANUP:
    /*----------------*/
    /* Free resources */
    /*----------------*/
    if(prlist) xfree(prlist);
    SLPBufferFree(sendbuf);
    SLPBufferFree(recvbuf);
    SLPXcastSocketsClose(&xcastsocks);
    
    return result;
}


#ifndef UNICAST_NOT_SUPPORTED
/*=========================================================================*/
SLPError NetworkUcastRqstRply(PSLPHandleInfo handle,
                              char* buf,
                              char buftype,
                              int bufsize,
                              NetworkRplyCallback callback,
                              void * cookie)
/* Description:                                                            */
/*                                                                         */
/* Unicasts SLP messages */
/*                                                                         */
/* handle  (IN) pointer to the SLP handle                                 */
/*                                                                         */
/* buf      (IN) pointer to the portion of the SLP message to send.       */
/*                                                                         */
/* buftype  (IN) the function-id to use in the SLPMessage header           */
/*                                                                         */
/* bufsize  (IN) the size of the buffer pointed to by buf                  */
/*                                                                         */
/* callback (IN) the callback to use for reporting results                 */
/*                                                                         */
/* cookie   (IN) the cookie to pass to the callback                        */
/*                                                                         */
/* Returns  -    SLP_OK on success. SLP_ERROR on failure                   */
/*=========================================================================*/
{
    struct timeval			timeout;
    struct sockaddr_storage peeraddr;
    SLPBuffer				sendbuf         = 0;
    SLPBuffer				recvbuf         = 0;
    SLPError				result          = 0;
    int						langtaglen      = 0;
    int						prlistlen       = 0;
    char*					prlist          = 0;
    int						xid             = 0;
    int						mtu             = 0;
    int						size            = 0;
    int						rplycount       = 0;
    int						maxwait         = 0;
    int						timeouts[MAX_RETRANSMITS];
    int                     retval1, retval2;
#ifdef DEBUG
    /* This function only supports unicast of the following messages
     */
    if(buftype != SLP_FUNCT_SRVRQST &&
       buftype != SLP_FUNCT_ATTRRQST &&
       buftype != SLP_FUNCT_SRVTYPERQST &&
       buftype != SLP_FUNCT_DASRVRQST)
    {
        return SLP_PARAMETER_BAD;
    }
#endif

    /*----------------------------------------------------*/
    /* Save off a few things we don't want to recalculate */
    /*----------------------------------------------------*/
    langtaglen = strlen(handle->langtag);
    xid = SLPXidGenerate();
    mtu = SLPPropertyAsInteger(SLPGetProperty("net.slp.MTU"));
    sendbuf = SLPBufferAlloc(mtu);
    if(sendbuf == 0)
    {
        result = SLP_MEMORY_ALLOC_FAILED;
        goto FINISHED;
    }

    /*-----------------------------------*/
    /* Unicast wait timeouts */
    /*-----------------------------------*/
    maxwait = SLPPropertyAsInteger(SLPGetProperty("net.slp.unicastMaximumWait"));
    SLPPropertyAsIntegerVector(SLPGetProperty("net.slp.unicastTimeouts"),
                               timeouts,
                               MAX_RETRANSMITS );

    /* Special case for fake SLP_FUNCT_DASRVRQST */
    if(buftype == SLP_FUNCT_DASRVRQST)
    {
        /* do something special for SRVRQST that will be discovering DAs */
        maxwait = SLPPropertyAsInteger(SLPGetProperty("net.slp.DADiscoveryMaximumWait"));
        SLPPropertyAsIntegerVector(SLPGetProperty("net.slp.DADiscoveryTimeouts"), timeouts, MAX_RETRANSMITS );
        /* SLP_FUNCT_DASRVRQST is a fake function.  We really want to */
        /* send a SRVRQST                                             */
        buftype  = SLP_FUNCT_SRVRQST;
    
    }

    /*---------------------------------------------------------------------*/
    /* Allocate memory for the prlist for appropriate messages.            */
    /* Notice that the prlist is as large as the MTU -- thus assuring that */
    /* there will not be any buffer overwrites regardless of how many      */
    /* previous responders there are.   This is because the retransmit     */
    /* code terminates if ever MTU is exceeded for any datagram message.   */
    /*---------------------------------------------------------------------*/
    prlist = (char*)xmalloc(mtu);
    if(prlist == 0)
    {
        result = SLP_MEMORY_ALLOC_FAILED;
        goto FINISHED;
    }

    *prlist = 0;
    prlistlen = 0;

    /*--------------------------*/
    /* Main unicast segment     */
    /*--------------------------*/
    timeout.tv_sec = timeouts[0] / 1000;
    timeout.tv_usec = (timeouts[0] % 1000) * 1000;

	size = 14 + langtaglen + bufsize;
	if(buftype == SLP_FUNCT_SRVRQST ||
	   buftype == SLP_FUNCT_ATTRRQST ||
	   buftype == SLP_FUNCT_SRVTYPERQST)
	{
	    /* add in room for the prlist */
	    size += 2 + prlistlen;
	}
	
	if((sendbuf = SLPBufferRealloc(sendbuf,size)) == 0)
	{
	    result = SLP_MEMORY_ALLOC_FAILED;
	    goto FINISHED;
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
	ToUINT16(sendbuf->start + 5, SLP_FLAG_UCAST);  /*this is a unicast */
	/*ext offset*/
	ToUINT24(sendbuf->start + 7,0);
	/*xid*/
	ToUINT16(sendbuf->start + 10,xid);
	/*lang tag len*/
	ToUINT16(sendbuf->start + 12,langtaglen);
	/*lang tag*/
	memcpy(sendbuf->start + 14, handle->langtag, langtaglen);
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
	/* Add the rest	of the message */
	/*-----------------------------*/
	memcpy(sendbuf->curpos,	buf, bufsize);

	/*----------------------*/
	/* send	the	send buffer	*/
	/*----------------------*/

	handle->unicastsock	= SLPNetworkConnectStream(&(handle->unicastaddr), &timeout);
	if ( handle->unicastsock >=	0 )
    {
		retval1	= SLPNetworkSendMessage(handle->unicastsock, SOCK_STREAM, sendbuf, &(handle->unicastaddr), &timeout);
		if ( retval1 !=	0 )
        {
			/* we could	not	send the message for some reason */
			/* we close	the	TCP	connection and break */
			if(errno ==	ETIMEDOUT)
            {
				result = SLP_NETWORK_TIMED_OUT;
			}
            else
            {
		        result = SLP_NETWORK_ERROR;
	        }
#ifdef _WIN32
	        closesocket(handle->unicastsock);
#else
	        close(handle->unicastsock);
#endif
	        goto FINISHED;
	    }
		
		retval2	= SLPNetworkRecvMessage(handle->unicastsock, SOCK_STREAM, &recvbuf,	&(handle->unicastaddr),	&timeout);
		if ( retval2 !=	0 )
        {
			/* An error	occured	while receiving	the	message	*/
			/* probably	just a time	out	error.				*/
			/* we close	the	TCP	connection and break */
			if(errno ==	ETIMEDOUT)
            {
				result = SLP_NETWORK_TIMED_OUT;
	        }
            else
            {
				result = SLP_NETWORK_ERROR;
            }
#ifdef _WIN32
	        closesocket(handle->unicastsock);
#else
	        close(handle->unicastsock);
#endif
			goto FINISHED;
		}
#ifdef _WIN32
	    closesocket(handle->unicastsock);
#else
	    close(handle->unicastsock);
#endif
		result = SLP_OK;
	}
    else
    {
		result = SLP_NETWORK_TIMED_OUT;
		/* Unsuccessful	in opening a TCP connection	*/
		/* just	break								*/
		goto FINISHED;
	}
	/* Sneek in	and	check the XID */
	if(AsUINT16(recvbuf->start+10) == xid)
	{
		char peeraddrstr[INET6_ADDRSTRLEN];
		rplycount += 1;
		/* Call	the	callback with the result and recvbuf */
		if(callback(result,&peeraddr,recvbuf,cookie) ==	SLP_FALSE)
		{
			/* Caller does not want	any	more info */
			/* We are done!						  */
			goto CLEANUP;
		}
		/* add the peer	to the previous	responder list */
		if(prlistlen !=	0)
		{
			strcat(prlist,",");
		}
		peeraddrstr[0] = '\0';
		if (peeraddr.ss_family == AF_INET)
        {
			inet_ntop(peeraddr.ss_family, &((struct	sockaddr_in*) &peeraddr)->sin_addr,	peeraddrstr, INET6_ADDRSTRLEN);
		}
		else if	(peeraddr.ss_family	== AF_INET6)
        {
			inet_ntop(peeraddr.ss_family, &((struct	sockaddr_in6*) &peeraddr)->sin6_addr, peeraddrstr, INET6_ADDRSTRLEN);
		}
		if (strcmp(peeraddrstr,	"")	!= 0) 
		{
			strcat(prlist, peeraddrstr);
			prlistlen =	 strlen(prlist);
		}
	}
		
	FINISHED:
	/*---------------------------------------------------------------------*/
	/* Notify the callback with	SLP_LAST_CALL so that they know	we're done */
	/*---------------------------------------------------------------------*/
	if(rplycount ||	result == SLP_NETWORK_TIMED_OUT)
	{
		result = SLP_LAST_CALL;
	}
 
	callback(result, NULL,NULL,cookie);
	if(result == SLP_LAST_CALL)
	{
		result = SLP_OK;
	}

	CLEANUP:
	/*----------------*/
	/* Free	resources */
	/*----------------*/
	if(prlist) xfree(prlist);
	SLPBufferFree(sendbuf);
	SLPBufferFree(recvbuf);

	return result;

}
#endif
	
/*=========================================================================*/
int NetworkGetMcastAddrs(const char msgtype, const char *msg, SLPIfaceInfo *ifaceinfo)
/* Returns all the multicast addresses the msgtype can be sent out to. If  */
/* there is more than one address returned, the address will be in the     */
/* order that they should be sent to.
/*                                                                         */
/* msgtype   (IN)  The function-id to use in the SLPMessage header         */
/* msg       (IN)  Pointer to the portion of the SLP message to send. The  */
/*                 portion to that should be pointed to is everything after*/
/*                 the pr-list. Only needed for Service Requests. Set to   */
/*                 NULL if not needed.								       */
/* ifaceinfo (OUT) Interface info to send the msg to.    		   	       */
/*                                                                         */
/* Returns  -    SLP_OK on success. SLP_ERROR on failure                   */
/*=========================================================================*/
{
	if (ifaceinfo == NULL)
		return SLP_PARAMETER_BAD;

	ifaceinfo->bcast_count = ifaceinfo->iface_count = 0;
	switch (msgtype) {
		case SLP_FUNCT_SRVRQST:
			if (msg == NULL)
				return SLP_PARAMETER_BAD;
			if (SLPNetIsIPV6()) {
				unsigned short srvtype_len = AsUINT16(msg);
				/* Add IPv6 multicast groups in order they should appear. */
				SLPNetGetSrvMcastAddr(msg+2, (unsigned long)srvtype_len, SLP_SCOPE_NODE_LOCAL, &ifaceinfo->iface_addr[ifaceinfo->iface_count]);
				SLPNetSetPort(&ifaceinfo->iface_addr[ifaceinfo->iface_count], SLP_RESERVED_PORT);
				ifaceinfo->iface_count++;
				SLPNetGetSrvMcastAddr(msg+2, srvtype_len, SLP_SCOPE_LINK_LOCAL, &ifaceinfo->iface_addr[ifaceinfo->iface_count]);
				SLPNetSetPort(&ifaceinfo->iface_addr[ifaceinfo->iface_count], SLP_RESERVED_PORT);
				ifaceinfo->iface_count++;
				SLPNetGetSrvMcastAddr(msg+2, srvtype_len, SLP_SCOPE_SITE_LOCAL, &ifaceinfo->iface_addr[ifaceinfo->iface_count]);
				SLPNetSetPort(&ifaceinfo->iface_addr[ifaceinfo->iface_count], SLP_RESERVED_PORT);
				ifaceinfo->iface_count++;
			}
			if (SLPNetIsIPV4()) {
				struct in_addr mcastaddr;
				mcastaddr.s_addr = SLP_MCAST_ADDRESS;
				SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count], AF_INET, SLP_RESERVED_PORT, (unsigned char *)&mcastaddr, sizeof(mcastaddr));
				ifaceinfo->iface_count++;
			}
			break;
		case SLP_FUNCT_ATTRRQST:
			if (SLPNetIsIPV6()) {
				/* Add IPv6 multicast groups in order they should appear. */
				SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],AF_INET6, SLP_RESERVED_PORT, (char *)&in6addr_srvloc_node, sizeof(struct in6_addr));
				ifaceinfo->iface_count++;
				SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],AF_INET6, SLP_RESERVED_PORT, (char *)&in6addr_srvloc_link, sizeof(struct in6_addr));
				ifaceinfo->iface_count++;
				SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],AF_INET6, SLP_RESERVED_PORT, (char *)&in6addr_srvloc_site, sizeof(struct in6_addr));
				ifaceinfo->iface_count++;
			}
			if (SLPNetIsIPV4()) {
				struct in_addr mcastaddr;
				mcastaddr.s_addr = SLP_MCAST_ADDRESS;
				SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count], AF_INET, SLP_RESERVED_PORT, (unsigned char *)&mcastaddr, sizeof(mcastaddr));

				ifaceinfo->iface_count++;
			}
			break;
		case SLP_FUNCT_SRVTYPERQST:
			if (SLPNetIsIPV6()) {
				/* Add IPv6 multicast groups in order they should appear. */
				SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],AF_INET6, SLP_RESERVED_PORT, (char *)&in6addr_srvloc_node, sizeof(struct in6_addr));
				ifaceinfo->iface_count++;
				SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],AF_INET6, SLP_RESERVED_PORT, (char *)&in6addr_srvloc_link, sizeof(struct in6_addr));
				ifaceinfo->iface_count++;
				SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],AF_INET6, SLP_RESERVED_PORT, (char *)&in6addr_srvloc_site, sizeof(struct in6_addr));
				ifaceinfo->iface_count++;
			}
			if (SLPNetIsIPV4()) {
				struct in_addr mcastaddr;
				mcastaddr.s_addr = SLP_MCAST_ADDRESS;
				SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count], AF_INET, SLP_RESERVED_PORT, (unsigned char *)&mcastaddr, sizeof(mcastaddr));
				ifaceinfo->iface_count++;
			}
			break;
		case SLP_FUNCT_DASRVRQST:
if (SLPNetIsIPV6()) {
				/* Add IPv6 multicast groups in order they should appear. */
				SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],AF_INET6, SLP_RESERVED_PORT, (char *)&in6addr_srvlocda_node, sizeof(struct in6_addr));
				ifaceinfo->iface_count++;
				SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],AF_INET6, SLP_RESERVED_PORT, (char *)&in6addr_srvlocda_link, sizeof(struct in6_addr));
				ifaceinfo->iface_count++;
				SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],AF_INET6, SLP_RESERVED_PORT, (char *)&in6addr_srvlocda_site, sizeof(struct in6_addr));
				ifaceinfo->iface_count++;
			}
			if (SLPNetIsIPV4()) {
				struct in_addr mcastaddr;
				mcastaddr.s_addr = SLP_MCAST_ADDRESS;
				SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count], AF_INET, SLP_RESERVED_PORT, (unsigned char *)&mcastaddr, sizeof(mcastaddr));
				ifaceinfo->iface_count++;
			}
			break;
		default:
			return SLP_PARAMETER_BAD;
	}

	return SLP_OK;
}		

		
