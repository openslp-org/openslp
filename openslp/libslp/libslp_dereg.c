/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slplib_dereg.c                                             */
/*                                                                         */
/* Abstract:    Implementation for SLPDeReg() call.                        */
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
SLPBoolean CallbackSrvDeReg(SLPError errorcode, 
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
    handle->params.dereg.callback((SLPHandle)handle,
                                  errorcode,
                                  handle->params.dereg.cookie);
    return SLP_FALSE;
}


/*-------------------------------------------------------------------------*/ 
SLPError ProcessSrvDeReg(PSLPHandleInfo handle)
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
    if(SLPPropertyAsBoolean(SLPGetProperty("net.slp.securityEnabled")))
    {
        result = SLPAuthSignUrl(handle->hspi,
                                0,
                                0,
                                handle->params.dereg.urllen,
                                handle->params.dereg.url,
                                &urlauthlen,
                                &urlauth);
        bufsize += urlauthlen;
    }
#endif

    /*-------------------------------------------------------------------*/
    /* determine the size of the fixed portion of the SRVDEREG           */
    /*-------------------------------------------------------------------*/
    bufsize += handle->params.dereg.scopelistlen + 2; /*  2 bytes for len field*/
    bufsize += handle->params.dereg.urllen + 6;       /*  1 byte for reserved  */
                                                      /*  2 bytes for lifetime */
                                                      /*  2 bytes for urllen   */
                                                      /*  1 byte for authcount */
    bufsize += 2;                                     /*  2 bytes for taglistlen*/
    
    /*--------------------------------------------*/
    /* Allocate a buffer for the SRVDEREG message */
    /*--------------------------------------------*/
    buf = curpos = (char*)xmalloc(bufsize);
    if(buf == 0)
    {
        result = SLP_MEMORY_ALLOC_FAILED;
        goto FINISHED;
    }

    /*------------------------------------------------------------*/
    /* Build a buffer containing the fixed portion of the SRVDEREG*/
    /*------------------------------------------------------------*/
    /* scope list */
    ToUINT16(curpos,handle->params.dereg.scopelistlen);
    curpos = curpos + 2;
    memcpy(curpos,
           handle->params.dereg.scopelist,
           handle->params.dereg.scopelistlen);
    curpos = curpos + handle->params.dereg.scopelistlen;
    /* url-entry reserved */
    *curpos = 0;        
    curpos = curpos + 1;
    /* url-entry lifetime */
    ToUINT16(curpos, 0);
    curpos = curpos + 2;
    /* url-entry urllen */
    ToUINT16(curpos,handle->params.dereg.urllen);
    curpos = curpos + 2;
    /* url-entry url */
    memcpy(curpos,
           handle->params.dereg.url,
           handle->params.dereg.urllen);
    curpos = curpos + handle->params.dereg.urllen;
    /* url-entry authcount */
#ifdef ENABLE_SLPv2_SECURITY
    if(urlauth)
    {
        /* authcount */
        *(curpos) = 1;
        curpos = curpos + 1;
        /* authblock */
        memcpy(curpos,urlauth,urlauthlen);
        curpos = curpos + urlauthlen;
    }
    else
#endif
    {
        /* authcount */
        *(curpos) = 0;
        curpos = curpos + 1;
    } 
    /* tag list */
    /* TODO: No tag list for now put in taglist stuff */
    ToUINT16(curpos,0);


    
    /*--------------------------*/
    /* Call the RqstRply engine */
    /*--------------------------*/
    sock = NetworkConnectToSA(handle,
                              handle->params.reg.scopelist,
                              handle->params.reg.scopelistlen,
                              &peeraddr);
    if(sock)
    {
        result = NetworkRqstRply(sock,
                                 &peeraddr,
                                 handle->langtag,
                                 0,
                                 buf,
                                 SLP_FUNCT_SRVDEREG,
                                 bufsize,
                                 CallbackSrvDeReg,
                                 handle);
        if (result)
        {
            NetworkDisconnectSA(handle);
        }   
    }

    FINISHED:
    if(buf) xfree(buf);
#ifdef ENABLE_SLPv2_SECURITY
    if(urlauth) xfree(urlauth);
#endif

    return result;
}

#ifdef ENABLE_ASYNC_API
/*-------------------------------------------------------------------------*/ 
SLPError AsyncProcessSrvDeReg(PSLPHandleInfo handle)
/*-------------------------------------------------------------------------*/
{
    SLPError result = ProcessSrvDeReg(handle);
    xfree((void*)handle->params.dereg.scopelist);
    xfree((void*)handle->params.dereg.url);
    handle->inUse = SLP_FALSE;
    return result;
}
#endif

/*=========================================================================*/
SLPError SLPDereg(SLPHandle  hSLP,
                  const char *srvUrl,
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
       *srvUrl     == 0 ||  /* url can't be empty string */
       callback    == 0)
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
    handle->params.dereg.scopelist     = SLPGetProperty("net.slp.useScopes");
    if(handle->params.dereg.scopelist)
    {
        handle->params.dereg.scopelistlen  = strlen(handle->params.dereg.scopelist);
    }
    handle->params.dereg.urllen        = strlen(srvUrl); 
    handle->params.dereg.url           = srvUrl;
    handle->params.dereg.callback      = callback;
    handle->params.dereg.cookie        = cookie;


    /*----------------------------------------------*/
    /* Check to see if we should be async or sync   */
    /*----------------------------------------------*/
#ifdef ENABLE_ASYNC_API
    if(handle->isAsync)
    {
        /* COPY all the parameters */
        handle->params.dereg.scopelist = xstrdup(handle->params.dereg.scopelist);
        handle->params.dereg.url = xstrdup(handle->params.dereg.url);

        /* make sure the strdups did not fail */
        if(handle->params.dereg.scopelist &&
           handle->params.dereg.url)
        {
            result = ThreadCreate((ThreadStartProc)AsyncProcessSrvDeReg,handle);
        }
        else
        {
            result = SLP_MEMORY_ALLOC_FAILED;
        }

        if(result)
        {
            if(handle->params.dereg.scopelist) xfree((void*)handle->params.dereg.scopelist);
            if(handle->params.dereg.url) xfree((void*)handle->params.dereg.url);
            handle->inUse = SLP_FALSE;
        }
    }
    else
#endif /* ifdef ENABLE_ASYNC_API */
    {
        /* REFERENCE all the parameters */
        result = ProcessSrvDeReg(handle);
        handle->inUse = SLP_FALSE;
    }

    if(parsedurl) SLPFree(parsedurl);

    return result;
}
