/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slplib_findattrs.c                                         */
/*                                                                         */
/* Abstract:    Implementation for SLPFindAttrs() call.                    */
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
SLPError ProcessAttrRqst(PSLPHandleInfo handle)
/*-------------------------------------------------------------------------*/
{
    struct timeval      timeout;
    struct sockaddr_in  peeraddr;
    
    SLPError            result      = 0;
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
    sock = NetworkConnectToDA(handle->params.findattrs.scopelist,
                              handle->params.findattrs.scopelistlen,
                              &peeraddr);
    if(sock < 0)
    {
        ismcast = 1;
        maxwait = atoi(SLPGetProperty("net.slp.multicastMaximumWait")) / 1000;
        wait    = 1;

        sock = NetworkConnectToSlpMulticast(&peeraddr);
        if(sock < 0)
        {
            sock = NetworkConnectToSlpBroadcast(&peeraddr);
            if(sock < 0)
            {
                result = SLP_NETWORK_INIT_FAILED;
                goto FINISHED;
            }
        }
    }
    else
    {
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
    
    /*--------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole attrrqst */
    /*--------------------------------------------------------------*/
    size = handle->langtaglen + 14;                 /* 14 bytes for header     */
    size += 2;
    /* we add in the size of the prlist later */
    size += handle->params.findattrs.urllen + 2;      /*  2 bytes for len field */
    size += handle->params.findattrs.scopelistlen + 2; /*  2 bytes for len field */
    size += handle->params.findattrs.taglistlen + 2;  /*  2 bytes for len field */
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
        *(buf->start + 1)   = SLP_FUNCT_ATTRRQST;
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
    
        /*--------------------------*/
        /* Add rest of the AttrRqst */
        /*--------------------------*/
        buf->curpos = buf->start + handle->langtaglen + 14 ;
        /* prlist */
        ToUINT16(buf->curpos,prlistlen);
        buf->curpos = buf->curpos + 2;
        memcpy(buf->curpos,
               prlist,
               prlistlen);
        buf->curpos = buf->curpos + prlistlen;
    
        /* url */
        ToUINT16(buf->curpos,handle->params.findattrs.urllen);
        buf->curpos = buf->curpos + 2;
        memcpy(buf->curpos,
               handle->params.findattrs.url,
               handle->params.findattrs.urllen);
        buf->curpos = buf->curpos + handle->params.findattrs.urllen;
        
        /* scope list */
        ToUINT16(buf->curpos,handle->params.findattrs.scopelistlen);
        buf->curpos = buf->curpos + 2;
        memcpy(buf->curpos,
               handle->params.findattrs.scopelist,
               handle->params.findattrs.scopelistlen);
        buf->curpos = buf->curpos + handle->params.findattrs.scopelistlen;
    
        /* taglist  */
        ToUINT16(buf->curpos,handle->params.findattrs.taglistlen);
        buf->curpos = buf->curpos + 2;
        memcpy(buf->curpos,
               handle->params.findattrs.taglist,
               handle->params.findattrs.taglistlen);
        buf->curpos = buf->curpos + handle->params.findattrs.taglistlen;
    
        /* TODO: add spi list stuff here later*/
        ToUINT16(buf->curpos,0);
        
    
        /*------------------------*/
        /* Send the AttrRqst      */
        /*------------------------*/
        timeout.tv_sec = wait; 
        timeout.tv_usec = 0;
        
        buf->curpos = buf->start;
        result = NetworkSendMessage(sock,
                                    buf,
                                    &timeout,
                                    &peeraddr);
        if(result != SLP_OK)
        {
            /* we could not send the message for some reason */
            /* we're done */
            break;
        }
            
        rplynet = 0;

        while(1)
        {
            /* Recv the SrvAck */
            result = NetworkRecvMessage(sock,
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
            
            /* parse the AttrRply message */
            result = SLPMessageParseBuffer(buf,msg);
            if(result != SLP_OK)
            {
                /* we probably got a parse error. Ignore the message and get
                   the next one */
                result = SLP_OK;
                continue;
            }
            
            if(msg->header.xid != xid ||
               msg->header.functionid != SLP_FUNCT_ATTRRPLY)
            {
                /* we probably got an out of sequence message. Ignore the 
                   message and get the next one */                    
                continue;
            }
            
            if(msg->body.attrrply.errorcode == 0)
            {
                
                /* TODO: Check the authblock */
                
                rplynet ++;    

                /* TRICKY: null terminate the attrlist by setting the authcount to 0 */
                msg->body.attrrply.authcount = 0;
                
                /* Call the callback function */
                if(handle->params.findattrs.callback((SLPHandle)handle,
                                                    msg->body.attrrply.attrlist,
                                                    0,
                                                    handle->params.findattrs.cookie) == 0)
                {
                    /* callback does not want any more data */
                    goto FINISHED;
                }
                   
            }while(ismcast) /* repeat again if rqst was mcast */

            if(ismcast == 0)
            {
                /* no need to recv again because we were talking to DA */
                break;
            }
        }

        /* determine if no replies have been received */
        rplytot += rplynet;                        
        if(rplytot && rplynet == 0)
        {
            break;
        }
        
        /* calculate the wait for the next retry */
        maxwait = maxwait - wait;
        wait = wait * 2;

    }while(ismcast && maxwait > 0 && size + prlistlen < mtu);

    /*----------------*/
    /* We're all done */
    /*----------------*/
    if(rplytot)
    {   
        /* call callback with last call */
        result = SLP_OK;
        handle->params.findattrs.callback((SLPHandle)handle,
                                          msg->body.attrrply.attrlist,
                                          SLP_LAST_CALL,
                                          handle->params.findattrs.cookie);
    }
    else
    {
        /* call calback with network timed out because no srvs were available */
        result = SLP_NETWORK_TIMED_OUT;
        handle->params.findattrs.callback((SLPHandle)handle,
                                          msg->body.attrrply.attrlist,
                                          SLP_NETWORK_TIMED_OUT,
                                          handle->params.findattrs.cookie);
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
SLPError AsyncProcessAttrRqst(PSLPHandleInfo handle)
/*-------------------------------------------------------------------------*/
{
    SLPError result = ProcessAttrRqst(handle);
    free((void*)handle->params.findattrs.url);
    free((void*)handle->params.findattrs.scopelist);
    free((void*)handle->params.findattrs.taglist);
    handle->inUse = SLP_FALSE;
    return result;
}


/*=========================================================================*/
SLPError SLPFindAttrs(SLPHandle   hSLP,
                      const char *pcURLOrServiceType,
                      const char *pcScopeList,
                      const char *pcAttrIds,
                      SLPAttrCallback callback,
                      void *pvCookie)
/*                                                                         */
/* This function returns service attributes matching the attribute ids     */
/* for the indicated service URL or service type.  If pcURLOrServiceType   */
/* is a service URL, the attribute information returned is for that        */
/* particular advertisement in the language locale of the SLPHandle.       */
/*                                                                         */
/* If pcURLOrServiceType is a service type name (including naming          */
/* authority if any), then the attributes for all advertisements of that   */
/* service type are returned regardless of the language of registration.   */
/* Results are returned through the callback.                              */
/*                                                                         */
/* The result is filtered with an SLP attribute request filter string      */
/* parameter, the syntax of which is described in RFC 2608. If the filter  */
/* string is the empty string, i.e.  "", all attributes are returned.      */
/*                                                                         */
/* hSLP                 The language specific SLPHandle on which to search */
/*                      for attributes.                                    */
/*                                                                         */
/* pcURLOrServiceType   The service URL or service type.  See RFC 2608 for */
/*                      URL and service type syntax.  May not be the empty */
/*                      string.                                            */
/*                                                                         */
/* pcScopeList          A pointer to a char containing a comma separated   */
/*                      list of scope names. Pass in NULL or the empty     */
/*                      string "" to find services in all the scopes the   */
/*                      local host is configured query.                    */
/*                                                                         */
/* pcAttrIds            A comma separated list of attribute ids to return. */
/*                      Use NULL or the empty string, "", to indicate all  */
/*                      values. Wildcards are not currently supported      */
/*                                                                         */
/* callback             A callback function through which the results of   */
/*                      the operation are reported.                        */
/*                                                                         */
/* pvCookie             Memory passed to the callback code from the client.*/  
/*                      May be NULL.                                       */
/*                                                                         */
/* Returns:             If an error occurs in starting the operation, one  */
/*                      of the SLPError codes is returned.                 */
/*=========================================================================*/
{
    PSLPHandleInfo      handle;
    SLPError            result;
    
    /*------------------------------*/
    /* check for invalid parameters */
    /*------------------------------*/
    if( hSLP == 0 ||
        *(unsigned long*)hSLP != SLP_HANDLE_SIG ||
        pcURLOrServiceType == 0 ||
        *pcURLOrServiceType == 0 || 
        callback == 0) 
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
    handle->params.findattrs.urllen   = strlen(pcURLOrServiceType);
    handle->params.findattrs.url      = pcURLOrServiceType;
    if(pcScopeList && *pcScopeList)
    {   
        handle->params.findattrs.scopelistlen = strlen(pcScopeList);
        handle->params.findattrs.scopelist    = pcScopeList;
    }
    else
    {
        handle->params.findattrs.scopelist    = SLPGetProperty("net.slp.useScopes");
        handle->params.findattrs.scopelistlen = strlen(handle->params.findattrs.scopelist);
    }
    if(pcScopeList && *pcScopeList)
    {
        handle->params.findattrs.taglistlen = strlen(pcAttrIds);
        handle->params.findattrs.taglist    = pcAttrIds;
    }
    else
    {
        handle->params.findattrs.taglistlen = 0;
        handle->params.findattrs.taglist    = (char*)&handle->params.findattrs.taglistlen;
    }
    handle->params.findattrs.callback   = callback;
    handle->params.findattrs.cookie     = pvCookie; 


    /*----------------------------------------------*/
    /* Check to see if we should be async or sync   */
    /*----------------------------------------------*/
    if(handle->isAsync)
    {
        /* COPY all the referenced parameters */
        handle->params.findattrs.url = strdup(handle->params.findattrs.url);
        handle->params.findattrs.scopelist = strdup(handle->params.findattrs.scopelist);
        handle->params.findattrs.taglist = strdup(handle->params.findattrs.taglist);
        
        /* make sure strdups did not fail */
        if(handle->params.findattrs.url &&
           handle->params.findattrs.scopelist &&
           handle->params.findattrs.taglist)
        {
            result = ThreadCreate((ThreadStartProc)AsyncProcessAttrRqst,handle);
        }
        else
        {
            result = SLP_MEMORY_ALLOC_FAILED;    
        }
    
        if(result)
        {
            if(handle->params.findattrs.url) free((void*)handle->params.findattrs.url);
            if(handle->params.findattrs.scopelist) free((void*)handle->params.findattrs.scopelist);
            if(handle->params.findattrs.taglist) free((void*)handle->params.findattrs.taglist);
            handle->inUse = SLP_FALSE;
        }
    }
    else
    {
        /* Leave all parameters REFERENCED */
        
        result = ProcessAttrRqst(handle);
        
        handle->inUse = SLP_FALSE;
    }

    return result;
}




