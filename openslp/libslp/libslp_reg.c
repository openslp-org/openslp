/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slplib_reg.c                                               */
/*                                                                         */
/* Abstract:    Implementation for functions register and deregister       */
/*              services -- SLPReg() call.                                 */
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
SLPBoolean CallbackSrvReg(SLPError errorcode,
                          struct sockaddr_in* peerinfo,
                          SLPBuffer replybuf, 
                          void* cookie)
/*-------------------------------------------------------------------------*/
{
    SLPMessage      replymsg;
    PSLPHandleInfo  handle      = (PSLPHandleInfo) cookie;

    /*-------------------------------------------*/
    /* Check the errorcode and bail if it is set */
    /*-------------------------------------------*/
    if(errorcode == 0)
    {
        /*--------------------*/
        /* Parse the replybuf */
        /*--------------------*/
        replymsg = SLPMessageAlloc();
        if(replymsg)
        {
            errorcode = SLPMessageParseBuffer(peerinfo,replybuf,replymsg);
            if(errorcode == 0)
            {
                if(replymsg->header.functionid == SLP_FUNCT_SRVACK)
                {
                    errorcode = replymsg->body.srvack.errorcode * - 1;
                }
            }
    
            SLPMessageFree(replymsg);
        }
        else
        {
            errorcode = SLP_MEMORY_ALLOC_FAILED;
        }
    }

    /*----------------------------*/
    /* Call the callback function */
    /*----------------------------*/
    handle->params.reg.callback((SLPHandle)handle,
                                errorcode,
                                handle->params.reg.cookie);
    
    return SLP_FALSE;
}


/*-------------------------------------------------------------------------*/ 
SLPError ProcessSrvReg(PSLPHandleInfo handle)
/*-------------------------------------------------------------------------*/
{
    int                 sock;
    struct sockaddr_in  peeraddr;
    int                 bufsize     = 0;
    char*               buf         = 0;
    char*               curpos      = 0;
    SLPError            result      = 0;

#ifdef ENABLE_SLPv2_SECURITY
    int                 urlauthlen  = 0;
    unsigned char*      urlauth     = 0;
    int                 attrauthlen = 0;
    unsigned char*      attrauth    = 0;

    if(SLPPropertyAsBoolean(SLPGetProperty("net.slp.securityEnabled")))
    {
        result = SLPAuthSignUrl(handle->hspi,
                                0,
                                0,
                                handle->params.reg.urllen,
                                handle->params.reg.url,
                                &urlauthlen,
                                &urlauth);
        if(result == 0)
        {
            result = SLPAuthSignString(handle->hspi,
                                       0,
                                       0,
                                       handle->params.reg.attrlistlen,
                                       handle->params.reg.attrlist,
                                       &attrauthlen,
                                       &attrauth);
        }
        bufsize += urlauthlen;
        bufsize += attrauthlen;
    }
#endif


    /*-------------------------------------------------------------------*/
    /* determine the size of the fixed portion of the SRVREG             */
    /*-------------------------------------------------------------------*/
    bufsize += handle->params.reg.urllen + 6;       /*  1 byte for reserved  */
                                                    /*  2 bytes for lifetime */
                                                    /*  2 bytes for urllen   */
                                                    /*  1 byte for authcount */
    bufsize += handle->params.reg.srvtypelen + 2;   /*  2 bytes for len field */
    bufsize += handle->params.reg.scopelistlen + 2; /*  2 bytes for len field */
    bufsize += handle->params.reg.attrlistlen + 2;  /*  2 bytes for len field */
    bufsize += 1;                                   /*  1 byte for authcount */

    buf = curpos = (char*)xmalloc(bufsize);
    if(buf == 0)
    {
        result = SLP_MEMORY_ALLOC_FAILED;
        goto FINISHED;
    }

    /*------------------------------------------------------------*/
    /* Build a buffer containing the fixed portion of the SRVREG  */
    /*------------------------------------------------------------*/
    /* url-entry reserved */
    *curpos= 0;        
    curpos = curpos + 1;
    /* url-entry lifetime */
    ToUINT16(curpos,handle->params.reg.lifetime);
    curpos = curpos + 2;
    /* url-entry urllen */
    ToUINT16(curpos,handle->params.reg.urllen);
    curpos = curpos + 2;
    /* url-entry url */
    memcpy(curpos,
           handle->params.reg.url,
           handle->params.reg.urllen);
    curpos = curpos + handle->params.reg.urllen;
    /* url-entry authblock */
#ifdef ENABLE_SLPv2_SECURITY
    if(urlauth)
    {
        /* authcount */
        *curpos = 1;
        curpos = curpos + 1;
        /* authblock */
        memcpy(curpos,urlauth,urlauthlen);
        curpos = curpos + urlauthlen;
    }
    else
#endif
    {
        /* authcount */
        *curpos = 0;
        curpos = curpos + 1;
    } 
    /* service type */
    ToUINT16(curpos,handle->params.reg.srvtypelen);
    curpos = curpos + 2;
    memcpy(curpos,
           handle->params.reg.srvtype,
           handle->params.reg.srvtypelen);
    curpos = curpos + handle->params.reg.srvtypelen;
    /* scope list */
    ToUINT16(curpos,handle->params.reg.scopelistlen);
    curpos = curpos + 2;
    memcpy(curpos,
           handle->params.reg.scopelist,
           handle->params.reg.scopelistlen);
    curpos = curpos + handle->params.reg.scopelistlen;
    /* attr list */
    ToUINT16(curpos,handle->params.reg.attrlistlen);
    curpos = curpos + 2;
    memcpy(curpos,
           handle->params.reg.attrlist,
           handle->params.reg.attrlistlen);
    curpos = curpos + handle->params.reg.attrlistlen;
    /* attribute auth block */
#ifdef ENABLE_SLPv2_SECURITY
    if(attrauth)
    {
        /* authcount */
        *curpos = 1;
        curpos = curpos + 1;
        /* authblock */
        memcpy(curpos,attrauth,attrauthlen);
        curpos = curpos + attrauthlen;
    }
    else
#endif
    {
        /* authcount */
        *curpos = 0;
        curpos = curpos + 1;
    }

    /*--------------------------*/
    /* Call the RqstRply engine */
    /*--------------------------*/
    sock = NetworkConnectToSA(handle,
                              handle->params.reg.scopelist,
                              handle->params.reg.scopelistlen,
                              &peeraddr);
    if(sock >= 0)
    {
        result = NetworkRqstRply(sock,
                                 &peeraddr,
                                 handle->langtag,
                                 buf,
                                 SLP_FUNCT_SRVREG,
                                 bufsize,
                                 CallbackSrvReg,
                                 handle);
        if (result)
        {
            NetworkDisconnectSA(handle);
        }   
    }
    else
    {
        result = SLP_NETWORK_INIT_FAILED;
    }


    FINISHED:
    if(buf) xfree(buf);
    
#ifdef ENABLE_SLPv2_SECURITY
    if(urlauth) xfree(urlauth);
    if(attrauth) xfree(attrauth);
#endif 

    return result;
}

#ifdef ENABLE_ASYNC_API
/*-------------------------------------------------------------------------*/ 
SLPError AsyncProcessSrvReg(PSLPHandleInfo handle)
/*-------------------------------------------------------------------------*/
{
    SLPError result = ProcessSrvReg(handle);

    xfree((void*)handle->params.reg.url);
    xfree((void*)handle->params.reg.srvtype);
    xfree((void*)handle->params.reg.scopelist);
    xfree((void*)handle->params.reg.attrlist);

    handle->inUse = SLP_FALSE;

    return result;
}
#endif


/*=========================================================================*/
SLPError SLPReg(SLPHandle   hSLP,
                const char  *srvUrl,
                const unsigned short lifetime,
                const char  *srvType,
                const char  *attrList,
                SLPBoolean  fresh,
                SLPRegReport callback,
                void *cookie)
/*                                                                         */
/* See slplib.h for detailed documentation                                 */
/*=========================================================================*/
{
    PSLPHandleInfo      handle      = 0;
    SLPError            result      = SLP_OK;
    SLPSrvURL*          parsedurl   = 0;

    /*------------------------------*/
    /* check for invalid parameters */
    /*------------------------------*/
    if(hSLP        == 0 ||
       *(unsigned int*)hSLP != SLP_HANDLE_SIG ||
       srvUrl      == 0 ||
       *srvUrl     == 0 ||  /* srvUrl can't be empty string */
       lifetime    == 0 ||  /* lifetime can not be zero */
       attrList    == 0 ||
       callback    == 0)
    {
        return SLP_PARAMETER_BAD;
    }

    /*-----------------------------------------*/
    /* We don't handle fresh registrations     */
    /*-----------------------------------------*/
    if(fresh == SLP_FALSE)
    {
        return SLP_NOT_IMPLEMENTED;
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

    /*------------------*/
    /* Parse the srvurl */
    /*------------------*/
    result = SLPParseSrvURL(srvUrl,&parsedurl);
    if(result)
    {
        if(result == SLP_PARSE_ERROR)
        {
            result = SLP_INVALID_REGISTRATION;
        }

        if(parsedurl) SLPFree(parsedurl);
        handle->inUse = SLP_FALSE;
        return result;
    }

    /*-------------------------------------------*/
    /* Set the handle up to reference parameters */
    /*-------------------------------------------*/
    handle->params.reg.fresh         = fresh;
    handle->params.reg.lifetime      = lifetime;
    handle->params.reg.urllen        = strlen(srvUrl);
    handle->params.reg.url           = srvUrl;
    handle->params.reg.srvtype       = parsedurl->s_pcSrvType;
    handle->params.reg.srvtypelen    = strlen(handle->params.reg.srvtype);
    handle->params.reg.scopelist     = SLPGetProperty("net.slp.useScopes");
    if(handle->params.reg.scopelist)
    {
        handle->params.reg.scopelistlen  = strlen(handle->params.reg.scopelist);
    }
    handle->params.reg.attrlistlen   = strlen(attrList);
    handle->params.reg.attrlist      = attrList;
    handle->params.reg.callback      = callback;
    handle->params.reg.cookie        = cookie; 

#ifdef ENABLE_ASYNC_API
    /*----------------------------------------------*/
    /* Check to see if we should be async or sync   */
    /*----------------------------------------------*/
    if(handle->isAsync)
    {
        /* Copy all of the referenced parameters before making thread */
        handle->params.reg.url = xstrdup(handle->params.reg.url);
        handle->params.reg.srvtype = xstrdup(handle->params.reg.url);
        handle->params.reg.scopelist = xstrdup(handle->params.reg.scopelist);
        handle->params.reg.attrlist = xstrdup(handle->params.reg.attrlist);

        /* make sure that the strdups did not fail */
        if(handle->params.reg.url &&
           handle->params.reg.srvtype &&
           handle->params.reg.scopelist &&
           handle->params.reg.attrlist)
        {
            result = ThreadCreate((ThreadStartProc)AsyncProcessSrvReg,handle);
        }
        else
        {
            result = SLP_MEMORY_ALLOC_FAILED;    
        }

        if(result)
        {
            if(handle->params.reg.url) xfree((void*)handle->params.reg.url);
            if(handle->params.reg.srvtype) xfree((void*)handle->params.reg.srvtype);
            if(handle->params.reg.scopelist) xfree((void*)handle->params.reg.scopelist);
            if(handle->params.reg.attrlist) xfree((void*)handle->params.reg.attrlist);
            handle->inUse = SLP_FALSE;
        }
    }
    else
#endif //ffdef ENABLE_ASYNC_API    
    {
        result = ProcessSrvReg(handle);            
        handle->inUse = SLP_FALSE;
    }

    if(parsedurl) SLPFree(parsedurl);

    return result;
}
