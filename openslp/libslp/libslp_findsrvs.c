/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slplib_findsrvs.c                                          */
/*                                                                         */
/* Abstract:    Implementation for SLPFindSrvs() call.                     */
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
SLPError ProcessSrvRqst(PSLPHandleInfo handle)
/*-------------------------------------------------------------------------*/
{
    struct timeval      timeout;
    struct sockaddr_in  peeraddr;
    
    SLPError            result      = 0;
    const char*         bcastonly   = 0;
    char*               prlist      = 0;
    int                 prlistlen   = 0;
    int                 rplytot     = 0;
    int                 rplynet     = 0;
    int                 ismcast     = 0;
    int                 size        = 0;
    int                 maxwait     = 0;
    int                 wait        = 0;
    SLPBuffer           buf         = 0;
    SLPMessage          msg         = 0;
    int                 sock        = -1;
    int                 mtu         = atoi(SLPGetProperty("net.slp.MTU"));
    int                 xid         = SLPXidGenerate();   
    

    /*---------------------------------------*/
    /* Connect to DA, multicast or broadcast */
    /*---------------------------------------*/
    sock = NetworkConnectToDA(handle->params.findsrvs.scopelist,
                              handle->params.findsrvs.scopelistlen,
                              &peeraddr);
    if(sock < 0)
    {
        /* Lets try multicast / broadcast */
        ismcast = 1;
        maxwait = atoi(SLPGetProperty("net.slp.multicastMaximumWait")) / 1000;
        wait    = 1;
        bcastonly = SLPPropertyGet("net.slp.isBroadcastOnly"); 
        if(*bcastonly == 'T' ||
           *bcastonly == 't' ||
           *bcastonly == 'Y' ||
           *bcastonly == 'y')
        {
            sock = SLPNetworkConnectToBroadcast(&peeraddr);
        }
        else                                                                 
        {
            sock = SLPNetworkConnectToMulticast(&peeraddr,
                                                atoi(SLPGetProperty("net.slp.ttl")));
        }
        
        if(sock < 0)
        {
            if(sock < 0)
            {
                result = SLP_NETWORK_INIT_FAILED;
                goto FINISHED;
            }
        }
    }
    else
    {
        /* Going with unicast tcp */
        ismcast = 0;
        maxwait = atoi(SLPGetProperty("net.slp.unicastMaximumWait")) / 1000;
        wait    = maxwait;
    }


    /*--------------------------------*/
    /* allocate memory for the prlist */
    /*--------------------------------*/
    prlist = (char*)malloc(mtu);
    if(prlist == 0)
    {
        result = SLP_MEMORY_ALLOC_FAILED;
        goto FINISHED;   
    }
    memset(prlist,0,SLP_MAX_DATAGRAM_SIZE);


    /*-----------------------*/
    /* allocate a SLPMessage */
    /*-----------------------*/
    msg = SLPMessageAlloc();
    if(msg == 0)
    {
        result = SLP_MEMORY_ALLOC_FAILED;
        goto FINISHED;
    }
    
    /*-------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvrqst */
    /*-------------------------------------------------------------*/
    size = handle->langtaglen + 14;                 /* 14 bytes for header     */
    size += 2;
    /* we add in the size of the prlist later */
    size += handle->params.findsrvs.srvtypelen + 2;   /*  2 bytes for len field */
    size += handle->params.findsrvs.scopelistlen + 2; /*  2 bytes for len field */
    size += handle->params.findsrvs.predicatelen + 2; /*  2 bytes for len field */
    size += 2;                                        /*  2 bytes for spistr len*/    
                                  
    /* make sure that we don't exceed the MTU */
    if(ismcast && size > mtu)
    {
        result = SLP_BUFFER_OVERFLOW;
        goto FINISHED;
    }                 

    /* allocate memory for the prlist */
    buf = SLPBufferAlloc(size + mtu);  
    if(buf == 0)
    {
        result = SLP_MEMORY_ALLOC_FAILED;
        goto FINISHED;
    }
    
    /* repeat loop until timeout or until the retransmit exceeds mtu */
    do
    {
        if(SLPBufferRealloc(buf, size + prlistlen) == 0)
        {
            result = SLP_MEMORY_ALLOC_FAILED;
            goto FINISHED;
        }
        
        /*----------------*/
        /* Add the header */
        /*----------------*/
        /*version*/
        *(buf->start)       = 2;
        /*function id*/
        *(buf->start + 1)   = SLP_FUNCT_SRVRQST;
        /*length*/
        ToUINT24(buf->start + 2, size + prlistlen);
        /*flags*/
        ToUINT16(buf->start + 5, ismcast ? SLP_FLAG_MCAST : 0);
        /*ext offset*/
        ToUINT24(buf->start + 7,0);
        /*xid*/
        ToUINT16(buf->start + 10,xid);
        /*lang tag len*/
        ToUINT16(buf->start + 12,handle->langtaglen);
        /*lang tag*/
        memcpy(buf->start + 14,
               handle->langtag,
               handle->langtaglen);
    
        /*-------------------------*/
        /* Add rest of the SrvRqst */
        /*-------------------------*/
        buf->curpos = buf->start + handle->langtaglen + 14 ;
        /* prlist */
        ToUINT16(buf->curpos,prlistlen);
        buf->curpos = buf->curpos + 2;
        memcpy(buf->curpos,
               prlist,
               prlistlen);
        buf->curpos = buf->curpos + prlistlen;
    
        /* service type */
        ToUINT16(buf->curpos,handle->params.findsrvs.srvtypelen);
        buf->curpos = buf->curpos + 2;
        memcpy(buf->curpos,
               handle->params.findsrvs.srvtype,
               handle->params.findsrvs.srvtypelen);
        buf->curpos = buf->curpos + handle->params.findsrvs.srvtypelen;
        
        /* scope list */
        ToUINT16(buf->curpos,handle->params.findsrvs.scopelistlen);
        buf->curpos = buf->curpos + 2;
        memcpy(buf->curpos,
               handle->params.findsrvs.scopelist,
               handle->params.findsrvs.scopelistlen);
        buf->curpos = buf->curpos + handle->params.findsrvs.scopelistlen;
    
        /* predicate */
        ToUINT16(buf->curpos,handle->params.findsrvs.predicatelen);
        buf->curpos = buf->curpos + 2;
        memcpy(buf->curpos,
               handle->params.findsrvs.predicate,
               handle->params.findsrvs.predicatelen);
        buf->curpos = buf->curpos + handle->params.findsrvs.predicatelen;
    
        /* TODO: add spi list stuff here later*/
        ToUINT16(buf->curpos,0);
        
        /*------------------------*/
        /* Send the SrvRqst       */
        /*------------------------*/
        timeout.tv_sec = wait; 
        timeout.tv_usec = 0;
        
        
        buf->curpos = buf->start;
        if(SLPNetworkSendMessage(sock,
                                 buf,
                                 &timeout,
                                 &peeraddr) != 0)
        {
            /* we could not send the message for some reason */
            /* we're done */
            break;
        }
            
        rplynet = 0;
        do
        {
            /* Recv the SrvAck */
            result = SLPNetworkRecvMessage(sock,
                                           buf,
                                           &timeout,
                                           &peeraddr);
            if(result != SLP_OK)
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
            
            /* parse the SrvRply message */
            result = SLPMessageParseBuffer(buf,msg);
            if(result != SLP_OK)
            {
                /* we probably got a parse error. Ignore the message and get
                   the next one */
                result = SLP_OK;
                continue;
            }
            
            if(msg->header.xid != xid ||
               msg->header.functionid != SLP_FUNCT_SRVRPLY)
            {
                /* we probably got an out of sequence message. Ignore the 
                   message and get the next one */                    
                continue;
            }
            
            if(msg->body.srvrply.errorcode == 0)
            {
                for(rplynet=0;rplynet<msg->body.srvrply.urlcount;rplynet++)
                {
                    /* TODO: Check the authblock */
                    
                    /* TRICKY: null terminate the url by setting the authcount to 0 */
                    msg->body.srvrply.urlarray[rplynet].authcount = 0;

                    /* TODO: Colate Results */

                    if(handle->params.findsrvs.callback((SLPHandle)handle,
                                                        msg->body.srvrply.urlarray[rplynet].url,
                                                        msg->body.srvrply.urlarray[rplynet].lifetime,
                                                        0,
                                                        handle->params.findsrvs.cookie) == 0)
                    {
                        /* callback does not want any more data */
                        goto FINISHED;
                    }
                }   
            }
        }while(ismcast);

        /* determine if no replies have been received */
        rplytot += rplynet;                        
        if(rplytot && rplynet == 0)
        {
            break;
        }
        
        if(ismcast == 0)
        {
            /* no retry necessary we were talking to DA */
            break;
        }
        
        /* calculate the wait for the next retry */
        maxwait = maxwait - wait;
        wait = wait * 2;
    }while(ismcast && maxwait > 0 && size + prlistlen < mtu);
    /* retry if mcast message and wait and size still are valid */

    /*----------------*/
    /* We're all done */
    /*----------------*/
    if(rplytot)
    {   
        /* call callback with last call */
        result = SLP_OK;
        handle->params.findsrvs.callback((SLPHandle)handle,
                                         0,
                                         0,
                                         SLP_LAST_CALL,
                                         handle->params.findsrvs.cookie);
    }
    else
    {
        /* call calback with network timed out because no srvs were available */
        result = SLP_NETWORK_TIMED_OUT;
        handle->params.findsrvs.callback((SLPHandle)handle,
                                         0,
                                         0,
                                         SLP_NETWORK_TIMED_OUT,
                                         handle->params.findsrvs.cookie);
    }
    
    FINISHED:
    /* free resources */
    if(prlist) free(prlist);
    SLPBufferFree(buf);
    SLPMessageFree(msg);
    close(sock);

    return result;
}


/*-------------------------------------------------------------------------*/ 
SLPError AsyncProcessSrvRqst(PSLPHandleInfo handle)
/*-------------------------------------------------------------------------*/
{
    SLPError result = ProcessSrvRqst(handle);
    free((void*)handle->params.findsrvs.srvtype);
    free((void*)handle->params.findsrvs.scopelist);
    free((void*)handle->params.findsrvs.predicate);
    handle->inUse = SLP_FALSE;
    return result;
}


/*=========================================================================*/
SLPError SLPFindSrvs(SLPHandle  hSLP,
                     const char *pcServiceType,
                     const char *pcScopeList,
                     const char *pcSearchFilter,
                     SLPSrvURLCallback callback,
                     void *pvCookie)
/*                                                                         */
/* Issue the query for services on the language specific SLPHandle and     */
/* return the results through the callback.  The parameters determine      */
/* the results                                                             */
/*                                                                         */
/* hSLP             The language specific SLPHandle on which to search for */
/*                  services.                                              */
/*                                                                         */
/* pcServiceType    The Service Type String, including authority string if */
/*                  any, for the request, such as can be discovered using  */
/*                  SLPSrvTypes(). This could be, for example              */
/*                  "service:printer:lpr" or "service:nfs".  May not be    */
/*                  the empty string or NULL.                              */
/*                                                                         */
/*                                                                         */
/* pcScopeList      A pointer to a char containing comma separated list of */
/*                  scope names.  Pass in the NULL or the empty string ""  */
/*                  to find services in all the scopes the local host is   */
/*                  configured query.                                      */
/*                                                                         */
/* pcSearchFilter   A query formulated of attribute pattern matching       */
/*                  expressions in the form of a LDAPv3 Search Filter.     */
/*                  If this filter is NULL or empty, i.e.  "", all         */
/*                  services of the requested type in the specified scopes */
/*                  are returned.                                          */
/*                                                                         */
/* callback         A callback function through which the results of the   */
/*                  operation are reported. May not be NULL                */
/*                                                                         */
/* pvCookie         Memory passed to the callback code from the client.    */
/*                  May be NULL.                                           */
/*                                                                         */
/* Returns:         If an error occurs in starting the operation, one of   */
/*                  the SLPError codes is returned.                        */
/*                                                                         */
/*=========================================================================*/
{
    PSLPHandleInfo      handle;
    SLPError            result;
    
    /*------------------------------*/
    /* check for invalid parameters */
    /*------------------------------*/
    if( hSLP            == 0 ||
        *(unsigned long*)hSLP != SLP_HANDLE_SIG ||
        pcServiceType   == 0 ||
        *pcServiceType  == 0 ||  /* srvtype can't be empty string */
        callback        == 0) 
    {
        return SLP_PARAMETER_BAD;
    }
    
    
    /*-----------------------------------------*/
    /* cast the SLPHandle into a SLPHandleInfo */
    /*-----------------------------------------*/
    handle = (PSLPHandleInfo)hSLP; 

    /*-----------------------------------------*/
    /* Check to see if the handle is in use    */
    /*-----------------------------------------*/
    if(handle->inUse == SLP_TRUE)
    {
        return SLP_HANDLE_IN_USE;
    }
    handle->inUse = SLP_TRUE;


    /*-------------------------------------------*/
    /* Set the handle up to reference parameters */
    /*-------------------------------------------*/
    handle->params.findsrvs.srvtypelen   = strlen(pcServiceType);
    handle->params.findsrvs.srvtype      = pcServiceType;
    if(pcScopeList && *pcScopeList)
    {   
        handle->params.findsrvs.scopelistlen = strlen(pcScopeList);
        handle->params.findsrvs.scopelist    = pcScopeList;
    }
    else
    {
        handle->params.findsrvs.scopelist    = SLPGetProperty("net.slp.useScopes");
        handle->params.findsrvs.scopelistlen = strlen(handle->params.findsrvs.scopelist);
    }

    if(pcSearchFilter)
    {
        handle->params.findsrvs.predicatelen = strlen(pcSearchFilter);
        handle->params.findsrvs.predicate    = pcSearchFilter;
    }
    else
    {   
        handle->params.findsrvs.predicatelen = 0;
        handle->params.findsrvs.predicate  = (char*)&handle->params.findsrvs.predicatelen;
    }
    handle->params.findsrvs.callback     = callback;
    handle->params.findsrvs.cookie       = pvCookie; 


    /*----------------------------------------------*/
    /* Check to see if we should be async or sync   */
    /*----------------------------------------------*/
    if(handle->isAsync)
    {
        /* COPY all the referenced parameters */
        handle->params.findsrvs.srvtype = strdup(handle->params.findsrvs.srvtype);
        handle->params.findsrvs.scopelist = strdup(handle->params.findsrvs.scopelist);
        handle->params.findsrvs.predicate = strdup(handle->params.findsrvs.predicate);
        
        /* make sure strdups did not fail */
        if(handle->params.findsrvs.srvtype &&
           handle->params.findsrvs.scopelist &&
           handle->params.findsrvs.predicate)
        {
            result = ThreadCreate((ThreadStartProc)AsyncProcessSrvRqst,handle);
        }
        else
        {
            result = SLP_MEMORY_ALLOC_FAILED;    
        }
    
        if(result)
        {
            if(handle->params.findsrvs.srvtype) free((void*)handle->params.findsrvs.srvtype);
            if(handle->params.findsrvs.scopelist) free((void*)handle->params.findsrvs.scopelist);
            if(handle->params.findsrvs.predicate) free((void*)handle->params.findsrvs.predicate);
            handle->inUse = SLP_FALSE;
        }
    }
    else
    {
        /* Leave all parameters REFERENCED */
        
        result = ProcessSrvRqst(handle);
        
        handle->inUse = SLP_FALSE;
    }

    return result;
}

