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
SLPError ProcessSrvReg(PSLPHandleInfo handle)
/*-------------------------------------------------------------------------*/
{
    struct timeval      timeout;
    struct sockaddr_in  slpdaddr;
    struct sockaddr_in  peeraddr;
    int                 slpdsock    = -1; 
    int                 size        = 0;
    SLPError            error       = 0;
    SLPBuffer           buf         = 0;
    SLPMessage          msg         = 0;
    int                 xid         = SLPXidGenerate();
    
    /*-----------------------*/
    /* Connect to slpd       */
    /*-----------------------*/
    slpdsock = NetworkConnectToSlpd(&slpdaddr);
    if(slpdsock < 0)
    {
        error = SLP_NETWORK_INIT_FAILED;
        goto FINISHED;
    }

    /*-----------------------*/
    /* allocate a SLPMessage */
    /*-----------------------*/
    msg = SLPMessageAlloc();
    if(msg == 0)
    {
        error = SLP_MEMORY_ALLOC_FAILED;
        goto FINISHED;
    }

    /*-------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvreg  */
    /*-------------------------------------------------------------*/
    size = handle->langtaglen + 14;                 /* 14 bytes for header     */

    size += handle->params.reg.urllen + 6;          /*  1 byte for reserved  */
                                                    /*  2 bytes for lifetime */
                                                    /*  2 bytes for urllen   */
                                                    /*  1 byte for authcount */
    
    size += handle->params.reg.srvtypelen + 2;   /*  2 bytes for len field */
    size += handle->params.reg.scopelistlen + 2; /*  2 bytes for len field */
    size += handle->params.reg.attrlistlen + 2;  /*  2 bytes for len field */
    size += 1;                                   /*  1 byte for authcount */
    
    /* TODO: Fix this for authentication */
    
    buf = SLPBufferAlloc(size);
    if(buf == 0)
    {
        error = SLP_MEMORY_ALLOC_FAILED;
        goto FINISHED;
    }


    /*----------------*/
    /* Add the header */
    /*----------------*/
    /*version*/
    *(buf->start)       = 2;
    /*function id*/
    *(buf->start + 1)   = SLP_FUNCT_SRVREG;
    /*length*/
    ToUINT24(buf->start + 2,size);
    /*flags*/
    ToUINT16(buf->start + 5,
             handle->params.reg.fresh ? SLP_FLAG_FRESH : 0);
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
    /* Add rest of the SrvReg  */
    /*-------------------------*/
    buf->curpos = buf->curpos + handle->langtaglen + 14;
    /* url-entry reserved */
    *buf->curpos = 0;        
    buf->curpos = buf->curpos + 1;
    /* url-entry lifetime */
    ToUINT16(buf->curpos,handle->params.reg.lifetime);
    buf->curpos = buf->curpos + 2;
    /* url-entry urllen */
    ToUINT16(buf->curpos,handle->params.reg.urllen);
    buf->curpos = buf->curpos + 2;
    /* url-entry url */
    memcpy(buf->curpos,
           handle->params.reg.url,
           handle->params.reg.urllen);
    buf->curpos = buf->curpos + handle->params.reg.urllen;
    /* url-entry authcount */
    *buf->curpos = 0;        
    buf->curpos = buf->curpos + 1;
    /* TODO: put in urlentry authentication stuff too */
    
    /* service type */
    ToUINT16(buf->curpos,handle->params.reg.srvtypelen);
    buf->curpos = buf->curpos + 2;
    memcpy(buf->curpos,
           handle->params.reg.srvtype,
           handle->params.reg.srvtypelen);
    buf->curpos = buf->curpos + handle->params.reg.srvtypelen;
    
    /* scope list */
    ToUINT16(buf->curpos,handle->params.reg.scopelistlen);
    buf->curpos = buf->curpos + 2;
    memcpy(buf->curpos,
           handle->params.reg.scopelist,
           handle->params.reg.scopelistlen);
    buf->curpos = buf->curpos + handle->params.reg.scopelistlen;

    /* attr list */
    ToUINT16(buf->curpos,handle->params.reg.attrlistlen);
    buf->curpos = buf->curpos + 2;
    memcpy(buf->curpos,
           handle->params.reg.attrlist,
           handle->params.reg.attrlistlen);
    buf->curpos = buf->curpos + handle->params.reg.attrlistlen;
    
    /* attr auths*/
    *(buf->curpos) = 0;

    
    /*------------------------*/
    /* Send the SrvReg        */
    /*------------------------*/
    timeout.tv_sec = atoi(SLPGetProperty("net.slp.unicastMaximumWait")) / 1000; 
    timeout.tv_usec = 0;        
    buf->curpos = buf->start;
    if(SLPNetworkSendMessage(slpdsock,
                             buf,
                             &timeout,
                             &slpdaddr) == 0)
    {
        /* Recv the SrvAck */
        if(SLPNetworkRecvMessage(slpdsock,
                                 buf,
                                 &timeout,
                                 &peeraddr) == 0)
        {
            /* parse the SrvAck message */
            error = SLPMessageParseBuffer(buf,msg);
            if(error == SLP_OK)
            {
                if(msg->header.xid == xid &&
                   msg->header.functionid == SLP_FUNCT_SRVACK)
                {
                    /* map and use errorcode from message */
                    error = -(msg->body.srvack.errorcode);
                    goto FINISHED;
                }
                else
                {
                    error = SLP_NETWORK_ERROR;
                    goto FINISHED;
                }
            }
        }
    }
    switch(errno)
    {
    case 0:
        error = SLP_OK;
    case ENOTCONN:
    case EPIPE:
        error = SLP_NETWORK_ERROR;
        break;
    case ETIME:
        error = SLP_NETWORK_TIMED_OUT;
        break;
    case ENOMEM:
        error = SLP_MEMORY_ALLOC_FAILED;
        break;
    case EINVAL:
        error = SLP_PARSE_ERROR;
        break;
    default:
        error = SLP_INTERNAL_SYSTEM_ERROR;
    }


    FINISHED:

    /* call callback function */
    handle->params.reg.callback((SLPHandle)handle,
                                     error,
                                     handle->params.reg.cookie);
    
    /* free resources */
    SLPBufferFree(buf);
    SLPMessageFree(msg);
    close(slpdsock);

    return 0;
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
