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
SLPBoolean CallbackSrvReg(SLPMessage msg, void* cookie)
/*-------------------------------------------------------------------------*/
{
    PSLPHandleInfo  handle      = (PSLPHandleInfo) cookie;
    
    if(msg->header.functionid == SLP_FUNCT_SRVACK)
    {
        /* Call the callback function */
        handle->params.reg.callback((SLPHandle)handle, 
                                    msg->body.srvack.errorcode,
                                    handle->params.reg.cookie);
    }
    else
    {
        /* TODO: what should we do here? */
    }
    
    return 0;
}


/*-------------------------------------------------------------------------*/ 
SLPError ProcessSrvReg(PSLPHandleInfo handle)
/*-------------------------------------------------------------------------*/
{
    struct sockaddr_in  peeraddr;
    int                 sock        = 0;
    int                 bufsize     = 0;
    char*               buf         = 0;
    char*               curpos      = 0;
    SLPError            result      = 0;
    
    /*-------------------------------------------------------------------*/
    /* determine the size of the fixed portion of the SRVREG             */
    /*-------------------------------------------------------------------*/
    bufsize  = handle->params.reg.urllen + 6;       /*  1 byte for reserved  */
                                                    /*  2 bytes for lifetime */
                                                    /*  2 bytes for urllen   */
                                                    /*  1 byte for authcount */
    bufsize += handle->params.reg.srvtypelen + 2;   /*  2 bytes for len field */
    bufsize += handle->params.reg.scopelistlen + 2; /*  2 bytes for len field */
    bufsize += handle->params.reg.attrlistlen + 2;  /*  2 bytes for len field */
    bufsize += 1;                                   /*  1 byte for authcount */
    
    /* TODO: Fix this for authentication */
    
    buf = curpos = (char*)malloc(bufsize);
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
    /* url-entry authcount */
    *curpos = 0;        
    curpos = curpos + 1;
    /* TODO: put in urlentry authentication stuff too */
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
    /* attr auths*/
    *(curpos) = 0;
    
    /*---------------------------------------*/
    /* Connect to slpd or a DA               */
    /*---------------------------------------*/
    sock = NetworkConnectToSlpd(&peeraddr);
    if(sock < 0)
    {
        sock = NetworkConnectToDA(handle->params.findsrvs.scopelist,
                                  handle->params.findsrvs.scopelistlen,
                                  &peeraddr);

        {
            result = SLP_NETWORK_INIT_FAILED;
            goto FINISHED;
        }
    }

    result = NetworkRqstRply(sock,
                             &peeraddr,
                             handle->langtag,
                             buf,
                             SLP_FUNCT_SRVREG,
                             bufsize,
                             CallbackSrvReg,
                             handle);
    
    FINISHED:
    if(buf) free(buf);

    return result;
}


/*-------------------------------------------------------------------------*/ 
SLPError AsyncProcessSrvReg(PSLPHandleInfo handle)
/*-------------------------------------------------------------------------*/
{
    SLPError result = ProcessSrvReg(handle);

    free((void*)handle->params.reg.url);
    free((void*)handle->params.reg.srvtype);
    free((void*)handle->params.reg.scopelist);
    free((void*)handle->params.reg.attrlist);
    
    handle->inUse = SLP_FALSE;

    return result;
}




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
    if( hSLP        == 0 ||
        *(unsigned long*)hSLP != SLP_HANDLE_SIG ||
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

    /*----------------------------------------------*/
    /* Check to see if we should be async or sync   */
    /*----------------------------------------------*/
    if(handle->isAsync)
    {
        /* Copy all of the referenced parameters before making thread */
        handle->params.reg.url = strdup(handle->params.reg.url);
        handle->params.reg.srvtype = strdup(handle->params.reg.url);
        handle->params.reg.scopelist = strdup(handle->params.reg.scopelist);
        handle->params.reg.attrlist = strdup(handle->params.reg.attrlist);
        
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
            if(handle->params.reg.url) free((void*)handle->params.reg.url);
            if(handle->params.reg.srvtype) free((void*)handle->params.reg.srvtype);
            if(handle->params.reg.scopelist) free((void*)handle->params.reg.scopelist);
            if(handle->params.reg.attrlist) free((void*)handle->params.reg.attrlist);
            handle->inUse = SLP_FALSE;
        } 
    }
    else
    {
        result = ProcessSrvReg(handle);            
        handle->inUse = SLP_FALSE;
    }
    
    if(parsedurl) SLPFree(parsedurl);

    return result;
}
