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
SLPBoolean CallbackAttrRqst(SLPError errorcode, SLPMessage msg, void* cookie)
/*-------------------------------------------------------------------------*/
{
    PSLPHandleInfo  handle      = (PSLPHandleInfo) cookie;
    
    if(errorcode == SLP_LAST_CALL)
    {
        handle->params.findattrs.callback((SLPHandle)handle,
					  (msg ?
					   msg->body.attrrply.attrlist : 0),
                                          errorcode,
                                          handle->params.findattrs.cookie);
        return 0;    
    }


    if(msg->header.functionid == SLP_FUNCT_ATTRRPLY)
    {
        if(msg->body.attrrply.errorcode == 0)
        {
            if(msg->body.attrrply.attrlistlen)
            {
                /* TRICKY: null terminate the attrlist by setting the authcount to 0 */
                *((char*)(msg->body.attrrply.attrlist)+msg->body.attrrply.attrlistlen) = 0;
                
                /* Call the callback function */
                if(handle->params.findattrs.callback((SLPHandle)handle,
						     msg->body.attrrply.attrlist,
						     0,
						     handle->params.findattrs.cookie) == 0)
		{
		    return 0;
		}
            }
        }
    }
    
    return 1;    
}

/*-------------------------------------------------------------------------*/
SLPError ProcessAttrRqst(PSLPHandleInfo handle)
/*-------------------------------------------------------------------------*/
{
    struct sockaddr_in  peeraddr;
    int                 sock        = 0;
    int                 bufsize     = 0;
    char*               buf         = 0;
    char*               curpos      = 0;
    SLPError            result      = 0;
    
    /*-------------------------------------------------------------------*/
    /* determine the size of the fixed portion of the ATTRRQST           */
    /*-------------------------------------------------------------------*/
    bufsize  = handle->params.findattrs.urllen + 2;       /*  2 bytes for len field */
    bufsize += handle->params.findattrs.scopelistlen + 2; /*  2 bytes for len field */
    bufsize += handle->params.findattrs.taglistlen + 2;   /*  2 bytes for len field */
    bufsize += 2;                                         /*  2 bytes for spistr len*/    
    
    /* TODO: make sure that we don't exceed the MTU */
    buf = curpos = (char*)malloc(bufsize);
    if(buf == 0)
    {
        result = SLP_MEMORY_ALLOC_FAILED;
        goto FINISHED;
    }
    
    /*------------------------------------------------------------*/
    /* Build a buffer containing the fixed portion of the ATTRRQST*/
    /*------------------------------------------------------------*/
    /* url */
    ToUINT16(curpos,handle->params.findattrs.urllen);
    curpos = curpos + 2;
    memcpy(curpos,
           handle->params.findattrs.url,
           handle->params.findattrs.urllen);
    curpos = curpos + handle->params.findattrs.urllen;
    /* scope list */
    ToUINT16(curpos,handle->params.findattrs.scopelistlen);
    curpos = curpos + 2;
    memcpy(curpos,
           handle->params.findattrs.scopelist,
           handle->params.findattrs.scopelistlen);
    curpos = curpos + handle->params.findattrs.scopelistlen;
    /* taglist  */
    ToUINT16(curpos,handle->params.findattrs.taglistlen);
    curpos = curpos + 2;
    memcpy(curpos,
           handle->params.findattrs.taglist,
           handle->params.findattrs.taglistlen);
    curpos = curpos + handle->params.findattrs.taglistlen;
    /* TODO: add spi list stuff here later*/
    ToUINT16(curpos,0);
    
    /*---------------------------------------*/
    /* Connect to DA, multicast or broadcast */
    /*---------------------------------------*/
    sock = NetworkConnectToDA(handle->params.findsrvs.scopelist,
                              handle->params.findsrvs.scopelistlen,
                              &peeraddr);
    if(sock < 0)
    {
        sock = NetworkConnectToMulticast(&peeraddr);
        if(sock < 0)
        {
            result = SLP_NETWORK_INIT_FAILED;
            goto FINISHED;
        }
    }

    result = NetworkRqstRply(sock,
                             &peeraddr,
                             handle->langtag,
                             buf,
                             SLP_FUNCT_ATTRRQST,
                             bufsize,
                             CallbackAttrRqst,
                             handle);
    
    FINISHED:
    if(buf) free(buf);

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




