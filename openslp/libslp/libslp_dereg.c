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
SLPError ProcessSrvDeReg(PSLPHandleInfo handle)
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
    size = handle->langtaglen + 14;                 /* 14 bytes for header  */

    size += handle->params.dereg.scopelistlen + 2; /*  2 bytes for len field*/
    size += handle->params.dereg.urllen + 8;       /*  1 byte for reserved  */
                                                   /*  2 bytes for lifetime */
                                                   /*  2 bytes for urllen   */
                                                   /*  1 byte for authcount */
    size += 2;                                     /*  2 bytes for taglistlen*/
    
    
    
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
    *(buf->start + 1)   = SLP_FUNCT_SRVDEREG;
    /*length*/
    ToUINT24(buf->start + 2,size);
    /*flags*/
    ToUINT16(buf->start + 5, 0);
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
    /* Add rest of the SrvDeReg */
    /*--------------------------*/
    buf->curpos = buf->curpos + handle->langtaglen + 14;
    
    /* scope list */
    ToUINT16(buf->curpos,handle->params.dereg.scopelistlen);
    buf->curpos = buf->curpos + 2;
    memcpy(buf->curpos,
           handle->params.dereg.scopelist,
           handle->params.dereg.scopelistlen);
    buf->curpos = buf->curpos + handle->params.dereg.scopelistlen;

    /* url-entry reserved */
    *buf->curpos = 0;        
    buf->curpos = buf->curpos + 1;
    /* url-entry lifetime */
    ToUINT16(buf->curpos, 0);
    buf->curpos = buf->curpos + 2;
    /* url-entry urllen */
    ToUINT16(buf->curpos,handle->params.dereg.urllen);
    buf->curpos = buf->curpos + 2;
    /* url-entry url */
    memcpy(buf->curpos,
           handle->params.dereg.url,
           handle->params.dereg.urllen);
    buf->curpos = buf->curpos + handle->params.dereg.urllen;
    /* url-entry authcount */
    *buf->curpos = 0;        
    buf->curpos = buf->curpos + 1;
    /* TODO: put in urlentry authentication stuff too */
    
    /* TODO: put tag list stuff in*/
    ToUINT16(buf->curpos,0);

    
    /*------------------------*/
    /* Send the SrvDeReg      */
    /*------------------------*/
    timeout.tv_sec = atoi(SLPGetProperty("net.slp.unicastMaximumWait")) / 1000;  
    timeout.tv_usec = 0;        
    buf->curpos = buf->start;
    error = NetworkSendMessage(slpdsock,
                               buf,
                               &timeout,
                               &slpdaddr);
    if(error == SLP_OK)
    {
        /* Recv the SrvAck */
        error = NetworkRecvMessage(slpdsock,
                                  buf,
                                  &timeout,
                                  &peeraddr);
        if(error == SLP_OK)
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
                }
                else
                {
                    error = SLP_NETWORK_ERROR;
                }
            }
        }
    }
    

    FINISHED:

    /* call callback function */
    handle->params.dereg.callback((SLPHandle)handle,
                                  error,
                                  handle->params.dereg.cookie);
    
    /* free resources */
    SLPBufferFree(buf);
    SLPMessageFree(msg);
    close(slpdsock);
        
    return 0;
}

/*-------------------------------------------------------------------------*/ 
SLPError AsyncProcessSrvDeReg(PSLPHandleInfo handle)
/*-------------------------------------------------------------------------*/
{
    SLPError result = ProcessSrvDeReg(handle);
    free((void*)handle->params.dereg.scopelist);
    free((void*)handle->params.dereg.url);
    handle->inUse = SLP_FALSE;
    return result;
}


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
    if( hSLP        == 0 ||
        *(unsigned long*)hSLP != SLP_HANDLE_SIG ||
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
    if(handle->isAsync)
    {
        /* COPY all the parameters */
        handle->params.dereg.scopelist = strdup(handle->params.dereg.scopelist);
        handle->params.dereg.url = strdup(handle->params.dereg.url);
        
        /* make sure the strdups did not fail */
        if(handle->params.dereg.scopelist &&
           handle->params.dereg.url )
        {
            result = ThreadCreate((ThreadStartProc)AsyncProcessSrvDeReg,handle);
        }
        else
        {
            result = SLP_MEMORY_ALLOC_FAILED;
        }

        if(result)
        {
            if(handle->params.dereg.scopelist) free((void*)handle->params.dereg.scopelist);
            if(handle->params.dereg.url) free((void*)handle->params.dereg.url);
            handle->inUse = SLP_FALSE;
        }
    }
    else
    {
        /* REFERENCE all the parameters */
         result = ProcessSrvDeReg(handle);
        handle->inUse = SLP_FALSE;
    }

    if(parsedurl) SLPFree(parsedurl);

    return result;
}
