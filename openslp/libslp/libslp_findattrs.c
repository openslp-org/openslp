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

/*-------------------------------------------------------------------------*/
SLPBoolean ProcessAttrRplyCallback(SLPError errorcode, 
                                   struct sockaddr_in* peerinfo,
                                   SLPBuffer replybuf,
                                   void* cookie)
/*-------------------------------------------------------------------------*/
{
    SLPMessage      replymsg;
    SLPAttrRply*    attrrply;
    PSLPHandleInfo  handle      = (PSLPHandleInfo) cookie;
    SLPBoolean      result      = SLP_TRUE;

#ifdef ENABLE_AUTHENTICATION 
    int             securityenabled;
    securityenabled = SLPPropertyAsBoolean(SLPGetProperty("net.slp.securityEnabled"));
#endif

    /*-------------------------------------------*/
    /* Check the errorcode and bail if it is set */
    /*-------------------------------------------*/
    if(errorcode)
    {
        handle->params.findattrs.callback((SLPHandle)handle,
                                          0,
                                          errorcode,
                                          handle->params.findattrs.cookie);
        return SLP_FALSE;
    }

    /*--------------------*/
    /* Parse the replybuf */
    /*--------------------*/
    replymsg = SLPMessageAlloc();
    if(replymsg)
    {
        if(SLPMessageParseBuffer(peerinfo,replybuf,replymsg) == 0 &&
           replymsg->header.functionid == SLP_FUNCT_ATTRRPLY &&
           replymsg->body.attrrply.errorcode == 0)
        {
            attrrply = &(replymsg->body.attrrply);
            
            if(attrrply->attrlistlen)
            {
     
#ifdef ENABLE_AUTHENTICATION
                /*-------------------------------*/
                /* Validate the authblocks       */
                /*-------------------------------*/
                if(SLPPropertyAsBoolean(SLPGetProperty("net.slp.securityEnabled")) &&
                   SLPAuthVerifyString(handle->hspi,
                                       1,
                                       attrrply->attrlistlen,
                                       attrrply->attrlist,
                                       attrrply->authcount,
                                       attrrply->autharray) == 0)
#endif
                {
                    /*---------------------------------------*/
                    /* Send the attribute list to the caller */
                    /*---------------------------------------*/
                    /* TRICKY: null terminate the attrlist by setting the authcount to 0 */
                    ((char*)(attrrply->attrlist))[attrrply->attrlistlen] = 0;
                    
                    /* Call the callback function */
                    result = handle->params.findattrs.callback((SLPHandle)handle,
                                                               attrrply->attrlist,
                                                               attrrply->errorcode * -1,
                                                               handle->params.findattrs.cookie);
                }
            }
        }
        
        SLPMessageFree(replymsg);
    }
    
    return result;
}


/*-------------------------------------------------------------------------*/
SLPError ProcessAttrRqst(PSLPHandleInfo handle)
/*-------------------------------------------------------------------------*/
{
    int                 sock;
    struct sockaddr_in  peeraddr;
    int                 bufsize     = 0;
    char*               buf         = 0;
    char*               curpos      = 0;
    SLPError            result      = 0;

#ifdef ENABLE_AUTHENTICATION
    int                 spistrlen   = 0;
    char*               spistr      = 0;

    if(SLPPropertyAsBoolean(SLPGetProperty("net.slp.securityEnabled")))
    {
        SLPSpiGetDefaultSPI(handle->hspi,
                            SLPSPI_KEY_TYPE_PUBLIC,
                            &spistrlen,
                            &spistr);
    }
#endif

    /*-------------------------------------------------------------------*/
    /* determine the size of the fixed portion of the ATTRRQST           */
    /*-------------------------------------------------------------------*/
    bufsize  = handle->params.findattrs.urllen + 2;       /*  2 bytes for len field */
    bufsize += handle->params.findattrs.scopelistlen + 2; /*  2 bytes for len field */
    bufsize += handle->params.findattrs.taglistlen + 2;   /*  2 bytes for len field */
    bufsize += 2;    /*  2 bytes for spistr len*/
#ifdef ENABLE_AUTHENTICATION
    bufsize += spistrlen;
#endif

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
#ifdef ENABLE_AUTHENTICATION
    ToUINT16(curpos,spistrlen);
    curpos = curpos + 2;
    memcpy(curpos,spistr,spistrlen);
    curpos = curpos + spistrlen;
#else
    ToUINT16(curpos,0);
#endif


    /*--------------------------*/
    /* Call the RqstRply engine */
    /*--------------------------*/
    do
    {
        sock = NetworkConnectToDA(handle,
                                  handle->params.findattrs.scopelist,
                                  handle->params.findattrs.scopelistlen,
                                  &peeraddr);
        if(sock == -1)
        {
            /* use multicast as a last resort */
            sock = NetworkConnectToMulticast(&peeraddr);
            result = NetworkRqstRply(sock,
                                     &peeraddr,
                                     handle->langtag,
                                     buf,
                                     SLP_FUNCT_ATTRRQST,
                                     bufsize,
                                     ProcessAttrRplyCallback,
                                     handle);
            close(sock);

            break;
        }

        result = NetworkRqstRply(sock,
                                 &peeraddr,
                                 handle->langtag,
                                 buf,
                                 SLP_FUNCT_ATTRRQST,
                                 bufsize,
                                 ProcessAttrRplyCallback,
                                 handle);
        if(result)
        {
            NetworkDisconnectDA(handle);
        }

    }while(result == SLP_NETWORK_ERROR);


    FINISHED:
    if(buf) free(buf);
#ifdef ENABLE_AUTHENTICATION
    if(spistr) free(spistr);
#endif

    return result;
}

#ifdef ENABLE_ASYNC_API
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
#endif


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
    if(hSLP == 0 ||
       *(unsigned int*)hSLP != SLP_HANDLE_SIG ||
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
    if(pcAttrIds && *pcAttrIds)
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
#ifdef ENABLE_ASYNC_API
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
#endif /* ifdef ENABLE_ASYNC_API */
    {
        /* Leave all parameters REFERENCED */

        result = ProcessAttrRqst(handle);

        handle->inUse = SLP_FALSE;
    }

    return result;
}




