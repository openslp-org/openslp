/***************************************************************************/
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slplib_findsrvtypes.c                                      */
/*                                                                         */
/* Abstract:    Implementation for SLPFindSrvType() call.                  */
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

/*----------------------------------------------------------------------------*/
SLPBoolean CallbackSrvTypeRqst(SLPError errorcode, SLPMessage msg, void* cookie)
/*----------------------------------------------------------------------------*/
{
    PSLPHandleInfo  handle      = (PSLPHandleInfo) cookie;
     
    if(errorcode == SLP_LAST_CALL)
    {
        handle->params.findsrvtypes.callback((SLPHandle)handle,
                                             0,
                                             errorcode,
                                             handle->params.findsrvtypes.cookie);
        return 0; 
    }

    if(msg->header.functionid == SLP_FUNCT_SRVTYPERPLY)
    {
        if(msg->body.srvtyperply.errorcode == 0)
        {
	    /* TRICKY: null terminate the srvtypelist, an extra byte
	       is already allocated in the receive buffer */
	    *((char*)(msg->body.srvtyperply.srvtypelist) +
	      msg->body.srvtyperply.srvtypelistlen) = 0; 
	}
	    
	if(handle->params.findsrvtypes.callback(
                  (SLPHandle)handle,
                  msg->body.srvtyperply.srvtypelist,
                  msg->body.srvtyperply.errorcode,
                  handle->params.findsrvtypes.cookie) == 0)
	{
	    return 0;
	}
    }
            
    return 1;
}

/*-------------------------------------------------------------------------*/
SLPError ProcessSrvTypeRqst(PSLPHandleInfo handle)
/*-------------------------------------------------------------------------*/
{
    int                 sock;
    struct sockaddr_in  peeraddr;
    int                 bufsize     = 0;
    char*               buf         = 0;
    char*               curpos      = 0;
    SLPError            result      = 0;
    
    /*-------------------------------------------------------------------*/
    /* determine the size of the fixed portion of the SRVTYPERQST            */
    /*-------------------------------------------------------------------*/
    bufsize  = handle->params.findsrvtypes.namingauthlen + 2;   /*  2 bytes for len field */
    bufsize += handle->params.findsrvtypes.scopelistlen + 2; /*  2 bytes for len field */
    
    /* TODO: make sure that we don't exceed the MTU */
    buf = curpos = (char*)malloc(bufsize);
    if(buf == 0)
    {
        result = SLP_MEMORY_ALLOC_FAILED;
        goto FINISHED;
    }
    
    /*----------------------------------------------------------------*/
    /* Build a buffer containing the fixed portion of the SRVTYPERQST */
    /*----------------------------------------------------------------*/
    /* naming authority */
    if (strcmp(handle->params.findsrvtypes.namingauth, "*") == 0)
    {
        ToUINT16(curpos,0xffff); /* 0xffff indicates all service types */
        curpos += 2;
    } else
    {
	ToUINT16(curpos,handle->params.findsrvtypes.namingauthlen);
	curpos += 2;
	memcpy(curpos,
	       handle->params.findsrvtypes.namingauth,
	       handle->params.findsrvtypes.namingauthlen);
	curpos += handle->params.findsrvtypes.namingauthlen;
    }
    /* scope list */
    ToUINT16(curpos,handle->params.findsrvtypes.scopelistlen);
    curpos = curpos + 2;
    memcpy(curpos,
           handle->params.findsrvtypes.scopelist,
           handle->params.findsrvtypes.scopelistlen);
    curpos = curpos + handle->params.findsrvtypes.scopelistlen;
    
    
    /*--------------------------*/
    /* Call the RqstRply engine */
    /*--------------------------*/
    do
    {
        sock = NetworkConnectToDA(handle,
                                  handle->params.findsrvtypes.scopelist,
                                  handle->params.findsrvtypes.scopelistlen,
                                  &peeraddr);
        if(sock == -1)
        {
            /* use multicast as a last resort */
            sock = NetworkConnectToMulticast(&peeraddr);
            result = NetworkRqstRply(sock,
                                 &peeraddr,
                                 handle->langtag,
                                 buf,
                                 SLP_FUNCT_SRVTYPERQST,
                                 bufsize,
                                 CallbackSrvTypeRqst,
                                 handle);
            close(sock);
            break;
        }

        result = NetworkRqstRply(sock,
                                 &peeraddr,
                                 handle->langtag,
                                 buf,
                                 SLP_FUNCT_SRVTYPERQST,
                                 bufsize,
                                 CallbackSrvTypeRqst,
                                 handle);
        if(result)
        {
            NetworkDisconnectDA(handle);
        }

    }while(result);

    
    FINISHED:
    if(buf) free(buf);

    return result;
}                                   
                                      

/*-------------------------------------------------------------------------*/ 
SLPError AsyncProcessSrvTypeRqst(PSLPHandleInfo handle)
/*-------------------------------------------------------------------------*/
{
    SLPError result = ProcessSrvTypeRqst(handle);
    free((void*)handle->params.findsrvtypes.namingauth);
    free((void*)handle->params.findsrvtypes.scopelist);
    handle->inUse = SLP_FALSE;
    return result;
}

/*=========================================================================*/
SLPError SLPFindSrvTypes(SLPHandle    hSLP,
                         const char  *pcNamingAuthority,
                         const char  *pcScopeList,
                         SLPSrvTypeCallback callback,
                         void *pvCookie)
/*                                                                         */
/* Issue the query for service types on the language specific SLPHandle    */
/* and return the results through the callback.  The parameters determine  */
/* the results                                                             */
/*                                                                         */
/* hSLP              The language specific SLPHandle on which to search    */
/*                   for service types.                                    */
/*                                                                         */
/* pcNamingAuthority The Naming Authority string for which service types   */
/*                   for which service types are returned                  */
/*                                                                         */
/* pcScopeList      A pointer to a char containing comma separated list of */
/*                  scope names.  Pass in the NULL or the empty string ""  */
/*                  to find services in all the scopes the local host is   */
/*                  configured query.                                      */
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
    if( hSLP == 0 ||
        *(unsigned int*)hSLP != SLP_HANDLE_SIG ||
        !pcNamingAuthority ||
        strcmp(pcNamingAuthority, "IANA") == 0 ||
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
    handle->params.findsrvtypes.namingauthlen = strlen(pcNamingAuthority);
    handle->params.findsrvtypes.namingauth = pcNamingAuthority;
    if(pcScopeList && *pcScopeList)
    {   
        handle->params.findsrvtypes.scopelist = pcScopeList;
    }
    else
    {
        handle->params.findsrvtypes.scopelist =
                        SLPGetProperty("net.slp.useScopes"); 
    }
    handle->params.findsrvtypes.scopelistlen =
	strlen(handle->params.findsrvtypes.scopelist); 
    handle->params.findsrvtypes.callback     = callback;
    handle->params.findsrvtypes.cookie       = pvCookie; 

    /*----------------------------------------------*/
    /* Check to see if we should be async or sync   */
    /*----------------------------------------------*/
    if(handle->isAsync)
    {
        /* COPY all the referenced parameters */
        handle->params.findsrvtypes.namingauth = strdup(handle->params.findsrvtypes.namingauth);
        handle->params.findsrvtypes.scopelist = strdup(handle->params.findsrvtypes.scopelist);
        
        /* make sure strdups did not fail */
        if(handle->params.findsrvtypes.namingauth &&
           handle->params.findsrvtypes.scopelist)
        {
            result = ThreadCreate((ThreadStartProc)AsyncProcessSrvTypeRqst,handle);
        }
        else
        {
            result = SLP_MEMORY_ALLOC_FAILED;    
        }
    
        if(result)
        {
            if(handle->params.findsrvtypes.namingauth) free((void*)handle->params.findsrvtypes.namingauth);
            if(handle->params.findsrvtypes.scopelist) free((void*)handle->params.findsrvtypes.scopelist);
            handle->inUse = SLP_FALSE;
        }
    }
    else
    {
        /* Leave all parameters REFERENCED */
        
        result = ProcessSrvTypeRqst(handle);
        
        handle->inUse = SLP_FALSE;
    }

    return result;
}


