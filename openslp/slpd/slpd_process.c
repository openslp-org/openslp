/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_process.c                                             */
/*                                                                         */
/* Abstract:    Processes incoming SLP messages                            */
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

/*=========================================================================*/
/* slpd includes                                                           */
/*=========================================================================*/
#include "slpd_process.h"
#include "slpd_property.h"
#include "slpd_database.h"
#include "slpd_knownda.h"
#include "slpd_log.h"


/*=========================================================================*/
/* common code includes                                                    */
/*=========================================================================*/
#include "../common/slp_message.h"
#include "../common/slp_da.h"
#include "../common/slp_compare.h"


/*-------------------------------------------------------------------------*/
int ProcessSASrvRqst(SLPMessage message,
                     SLPBuffer* sendbuf,
                     int errorcode)
/*-------------------------------------------------------------------------*/
{
    int size = 0;
    SLPBuffer result = *sendbuf;

    if(message->body.srvrqst.scopelistlen == 0 ||
       SLPIntersectStringList(message->body.srvrqst.scopelistlen,
                              message->body.srvrqst.scopelist,
                              G_SlpdProperty.useScopesLen,
                              G_SlpdProperty.useScopes) != 0)
    {
        /*----------------------*/
        /* Send back a SAAdvert */
        /*----------------------*/

        /*--------------------------------------------------------------*/
        /* ensure the buffer is big enough to handle the whole SAAdvert */
        /*--------------------------------------------------------------*/
        size = message->header.langtaglen + 21; /* 14 bytes for header     */
                                                /*  2 bytes for url count  */
                                                /*  2 bytes for scope list len */
                                                /*  2 bytes for attr list len */
                                                /*  1 byte for authblock count */
        size += G_SlpdProperty.myUrlLen;
        size += G_SlpdProperty.useScopesLen;
        /* TODO: size += G_SlpdProperty.SAAttributes */

        result = SLPBufferRealloc(result,size);
        if(result == 0)
        {
            /* TODO: out of memory, what should we do here! */
            errorcode = SLP_ERROR_INTERNAL_ERROR;
            goto FINISHED;
        }

        /*----------------*/
        /* Add the header */
        /*----------------*/
        /*version*/
        *(result->start)       = 2;
        /*function id*/
        *(result->start + 1)   = SLP_FUNCT_SAADVERT;
        /*length*/
        ToUINT24(result->start + 2, size);
        /*flags*/
        ToUINT16(result->start + 5,
                 (size > SLP_MAX_DATAGRAM_SIZE ? SLP_FLAG_OVERFLOW : 0));
        /*ext offset*/
        ToUINT24(result->start + 7,0);
        /*xid*/
        ToUINT16(result->start + 10,message->header.xid);
        /*lang tag len*/
        ToUINT16(result->start + 12,message->header.langtaglen);
        /*lang tag*/
        memcpy(result->start + 14,
               message->header.langtag,
               message->header.langtaglen);

        /*--------------------------*/
        /* Add rest of the SAAdvert */
        /*--------------------------*/
        result->curpos = result->start + 14 + message->header.langtaglen;
        /* url len */
        ToUINT16(result->curpos, G_SlpdProperty.myUrlLen);
        result->curpos = result->curpos + 2;
        /* url */
        memcpy(result->curpos,G_SlpdProperty.myUrl,G_SlpdProperty.myUrlLen);
        result->curpos = result->curpos + G_SlpdProperty.myUrlLen;
        /* scope list len */
        ToUINT16(result->curpos, G_SlpdProperty.useScopesLen);
        result->curpos = result->curpos + 2;
        /* scope list */
        memcpy(result->curpos,G_SlpdProperty.useScopes,G_SlpdProperty.useScopesLen);
        result->curpos = result->curpos + G_SlpdProperty.useScopesLen;
        /* attr list len */
        /* ToUINT16(result->curpos,G_SlpdProperty.SAAttributesLen) */
        ToUINT16(result->curpos, 0);
        result->curpos = result->curpos + 2;
        /* attr list */
        /* memcpy(result->start,G_SlpdProperty.SAAttributes,G_SlpdProperty.SAAttributesLen) */
        /* authblock count */
        *(result->curpos) = 0;
    }

    FINISHED:

    *sendbuf = result;

    return errorcode;
}


/*-------------------------------------------------------------------------*/
int ProcessDASrvRqst(SLPMessage message,
                     SLPBuffer* sendbuf,
                     int errorcode)
/*-------------------------------------------------------------------------*/
{
    SLPDAEntry      daentry;
    SLPBuffer       tmp     = 0;
    SLPDAEntry*     entry   = 0;
    void*           i       = 0;

    /* set up local data */
    memset(&daentry,0,sizeof(daentry));    

    /*---------------------------------------------------------------------*/
    /* Special case for when libslp asks slpd (through the loopback) about */
    /* a known DAs. Fill sendbuf with DAAdverts from all known DAs.        */
    /*---------------------------------------------------------------------*/
    if(ISLOCAL(message->peer.sin_addr))
    {
        /* TODO: be smarter about how much memory is allocated here! */
        /* 4096 may not be big enough to handle all DAAdverts        */
        *sendbuf = SLPBufferRealloc(*sendbuf, 4096);
        if(*sendbuf == 0)
        {
            return SLP_ERROR_INTERNAL_ERROR;
        }

        if(errorcode == 0)
        {

            /* Note: The weird *sendbuf code is making a single SLPBuffer */
            /*       that contains multiple DAAdverts.  This is a special */
            /*       process that only happens for the DA SrvRqst through */
            /*       loopback to the SLPAPI                               */
            while(SLPDKnownDAEnum(&i,&entry) == 0)
            {
                if(SLPDKnownDAEntryToDAAdvert(errorcode,
                                              message->header.xid,
                                              entry,
                                              &tmp) == 0)
                {
                    if(((*sendbuf)->curpos) + (tmp->end - tmp->start) > (*sendbuf)->end)
                    {
                        break;
                    }

                    memcpy((*sendbuf)->curpos, tmp->start, tmp->end - tmp->start);
                    (*sendbuf)->curpos = ((*sendbuf)->curpos) + (tmp->end - tmp->start);
                }
            }

            /* Tack on a "terminator" DAAdvert */
            SLPDKnownDAEntryToDAAdvert(SLP_ERROR_INTERNAL_ERROR,
                                       message->header.xid,
                                       &daentry,
                                       &tmp);
            if(((*sendbuf)->curpos) + (tmp->end - tmp->start) <= (*sendbuf)->end)
            {
                memcpy((*sendbuf)->curpos, tmp->start, tmp->end - tmp->start);
                (*sendbuf)->curpos = ((*sendbuf)->curpos) + (tmp->end - tmp->start);
            }

            /* mark the end of the sendbuf */
            (*sendbuf)->end = (*sendbuf)->curpos;


            if(tmp)
            {
                SLPBufferFree(tmp);
            }
        }

        return errorcode;
    }


    /*---------------------------------------------------------------------*/
    /* Normal case where a remote Agent asks for a DA                      */
    /*---------------------------------------------------------------------*/
    if(G_SlpdProperty.isDA)
    {
        if(message->body.srvrqst.scopelistlen == 0 ||
           SLPIntersectStringList(message->body.srvrqst.scopelistlen, 
                                  message->body.srvrqst.scopelist,
                                  G_SlpdProperty.useScopesLen,
                                  G_SlpdProperty.useScopes))
        {
            /* fill out real structure */
            G_SlpdProperty.DATimestamp += 1;
            daentry.bootstamp = G_SlpdProperty.DATimestamp;
            daentry.langtaglen = G_SlpdProperty.localeLen;
            daentry.langtag = (char*)G_SlpdProperty.locale;
            daentry.urllen = G_SlpdProperty.myUrlLen;
            daentry.url = (char*)G_SlpdProperty.myUrl;
            daentry.scopelistlen = G_SlpdProperty.useScopesLen;
            daentry.scopelist = (char*)G_SlpdProperty.useScopes;
            daentry.attrlistlen = 0;
            daentry.attrlist = 0;
            daentry.spilistlen = 0;
            daentry.spilist = 0;
        }
        else
        {
            errorcode =  SLP_ERROR_SCOPE_NOT_SUPPORTED;
        }
    }
    else
    {
        errorcode = SLP_ERROR_MESSAGE_NOT_SUPPORTED;       
    }

    /* don't return errorcodes to multicast messages */
    if(errorcode != 0)
    {
        if(message->header.flags & SLP_FLAG_MCAST ||
           ISMCAST(message->peer.sin_addr))
        {
            (*sendbuf)->end = (*sendbuf)->start;
            return errorcode;
        }
    }

    errorcode = SLPDKnownDAEntryToDAAdvert(errorcode,
                                           message->header.xid,
                                           &daentry,
                                           sendbuf);

    return errorcode;
}


/*-------------------------------------------------------------------------*/
int ProcessSrvRqst(SLPMessage message,
                   SLPBuffer* sendbuf,
                   int errorcode)
/*-------------------------------------------------------------------------*/
{
    int                         i;
    SLPUrlEntry*                urlentry;
    SLPDDatabaseSrvRqstResult*  db          = 0;
    int                         size        = 0;
    SLPBuffer                   result      = *sendbuf;

#ifdef ENABLE_AUTHENTICATION
    int                         authcount   = 0;
    SLPAuthBlock*               authblock   = 0;
#endif
    
    /*--------------------------------------------------------------*/
    /* If errorcode is set, we can not be sure that message is good */
    /* Go directly to send response code                            */
    /*--------------------------------------------------------------*/
    if(errorcode)
    {
        goto RESPOND;
    }

    /*-------------------------------------------------*/
    /* Check for one of our IP addresses in the prlist */
    /*-------------------------------------------------*/
    if(SLPIntersectStringList(message->body.srvrqst.prlistlen,
                              message->body.srvrqst.prlist,
                              G_SlpdProperty.interfacesLen,
                              G_SlpdProperty.interfaces) )
    {
        /* silently ignore */
        result->end = result->start;
        goto FINISHED;
    }

    /*------------------------------------*/
    /* Make sure that we handle the SPI   */
    /*------------------------------------*/
#ifdef ENABLE_AUTHENTICATION
    if(G_SlpdProperty.enableSecurity && 
       SLPSpiCanVerify(SLPSpiHandle hspi,
                       message->body.srvrqst.spistrlen,
                       message->body.srvrqst.spistr) == 0)
        {
            result = SLP_ERROR_AUTHENTICATION_UNKNOWN;
            goto RESPOND;
        }
    }
    else if(message->body.srvrqst.spistrlen == 0)
#else
    if(message->body.srvrqst.spistrlen == 0)
#endif
    {
        /*------------------------------------------------*/
        /* Check to to see if a this is a special SrvRqst */
        /*------------------------------------------------*/
        if(SLPCompareString(message->body.srvrqst.srvtypelen,
                            message->body.srvrqst.srvtype,
                            23,
                            "service:directory-agent") == 0)
        {
            errorcode = ProcessDASrvRqst(message, sendbuf, errorcode);
            return errorcode;
        }
        if(SLPCompareString(message->body.srvrqst.srvtypelen,
                            message->body.srvrqst.srvtype,
                            21,
                            "service:service-agent") == 0)
        {
            errorcode = ProcessSASrvRqst(message, sendbuf, errorcode);
            return errorcode;
        }
    
        /*------------------------------------*/
        /* Make sure that we handle the scope */
        /*------ -----------------------------*/
        if(SLPIntersectStringList(message->body.srvrqst.scopelistlen,
                                  message->body.srvrqst.scopelist,
                                  G_SlpdProperty.useScopesLen,
                                  G_SlpdProperty.useScopes) != 0)
        {
            /*-------------------------------*/
            /* Find services in the database */
            /*-------------------------------*/
            errorcode = SLPDDatabaseSrvRqstStart(message, &db);
        }
        else
        {
            errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;
        }
    }
    else
    {
        errorcode = SLP_ERROR_AUTHENTICATION_UNKNOWN;
    }

    RESPOND:
    /*----------------------------------------------------------------*/
    /* Do not send error codes or empty replies to multicast requests */
    /*----------------------------------------------------------------*/
    if(errorcode != 0 || db->urlcount == 0)
    {
        if(message->header.flags & SLP_FLAG_MCAST ||
           ISMCAST(message->peer.sin_addr))
        {
            result->end = result->start;
            goto FINISHED;  
        }
    }

    /*-------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvrply */
    /*-------------------------------------------------------------*/
    size = message->header.langtaglen + 18; /* 14 bytes for header     */
                                            /*  2 bytes for error code */
                                            /*  2 bytes for url count  */
    if(errorcode == 0)
    {
        for(i=0;i<db->urlcount;i++)
        {
            /* urlentry is the url from the db result */
            urlentry = db->urlarray[i];

            size += urlentry->urllen + 6; /*  1 byte for reserved  */
                                          /*  2 bytes for lifetime */
                                          /*  2 bytes for urllen   */
                                          /*  1 byte for authcount */
#ifdef ENABLE_AUTHENTICATION
            /* include the authblock that was asked for */
            if(G_SlpdProperty.enableSecurity &&
               message->body.srvrqst.spistrlen )
            {
                for(authcount=0;authcount < urlentry>authcount;authcount++)
                {
                    authblock = &(urlentry->authblock[authcount]);
                    if(SLPStringCompare(authblock->spistrlen,
                                        authblock->spistr,
                                        message->body.srvrqst.spistrlen,
                                        message->body.srvrqst.spistr) == 0)
                    {
                        size += authblock->length;
                        break;
                    }
                }
            }
#endif 
        }
    }

    /*------------------------------*/
    /* Reallocate the result buffer */
    /*------------------------------*/
    result = SLPBufferRealloc(result,size);
    if(result == 0)
    {
        errorcode = SLP_ERROR_INTERNAL_ERROR;
        goto FINISHED;
    }
    
    /*----------------*/
    /* Add the header */
    /*----------------*/
    /*version*/
    *(result->start)       = 2;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_SRVRPLY;
    /*length*/
    ToUINT24(result->start + 2, size);
    /*flags*/
    ToUINT16(result->start + 5,
             (size > SLP_MAX_DATAGRAM_SIZE ? SLP_FLAG_OVERFLOW : 0));
    /*ext offset*/
    ToUINT24(result->start + 7,0);
    /*xid*/
    ToUINT16(result->start + 10,message->header.xid);
    /*lang tag len*/
    ToUINT16(result->start + 12,message->header.langtaglen);
    /*lang tag*/
    memcpy(result->start + 14,
           message->header.langtag,
           message->header.langtaglen);

    /*-------------------------*/
    /* Add rest of the SrvRply */
    /*-------------------------*/
    result->curpos = result->start + 14 + message->header.langtaglen;
    /* error code*/
    ToUINT16(result->curpos, errorcode);
    result->curpos = result->curpos + 2;
    if(errorcode == 0)
    {
        /* urlentry count */
        ToUINT16(result->curpos, db->urlcount);
        result->curpos = result->curpos + 2;
        
        for(i=0;i<db->urlcount;i++)
        {
            /* urlentry is the url from the db result */
            urlentry = db->urlarray[i];
            
            /* Use an opaque copy if available */
            if(urlentry->opaque)
            {
                /* TODO: should we do this with authentication enabled? */
                /*       by doing so we return ALL authblocks of the    */
                /*       urlentry which may include ones not asked for  */
                memcpy(result->curpos,urlentry->opaque,urlentry->opaquelen);
                result->curpos = result->curpos + urlentry->opaquelen;
            }
            else
            {
                /* url-entry reserved */
                *result->curpos = 0;        
                result->curpos = result->curpos + 1;
                /* url-entry lifetime */
                ToUINT16(result->curpos,urlentry->lifetime);
                result->curpos = result->curpos + 2;
                /* url-entry urllen */
                ToUINT16(result->curpos,urlentry->urllen);
                result->curpos = result->curpos + 2;
                /* url-entry url */
                memcpy(result->curpos,urlentry->url,urlentry->urllen);
                result->curpos = result->curpos + urlentry->urllen;
                /* url-entry auths */
#ifdef ENABLE_AUTHENTICATION
                /* include an authblock if we should supply one */
                /* authblock is set above (line ~434)           */
                if(authblock)
                {
                    /* authcount == 1 */
                    *result->curpos = 1;
                    result->curpos = result->curpos + 1;
                    /* bsd */
                    ToUINT16(result->curpos,authblock->bsd);
                    result->curpos = result->curpos + 2;
                    /* authblock length */
                    ToUINT16(result->curpos,authblock->length);
                    result->curpos = resuslt->curpos + 2;
                    /* authblock timestamp */
                    ToUINT32(result->curpos,authblock->timestamp);
                    result->curpos = result->curpos + 4;
                    /* authblock spistr */
                    ToUINT16(result->curpos,authblock->spistrlen);
                    result->curpos = result->curpos + 2;
                    memcpy(result->curpos,authblock->spistr,authblock->spistrlen);
                    result->curpos = result->curpos + authblock->spistrlen;
                    /* authblock structured authenticator */
                    memcpy(result->curpos,
                           authblock->authstruct,
                           authblock->length - authblock->spistrlen - 10);
                }
                else
#endif
                {
                    /* authcount == 0 */
                    *result->curpos = 0;
                    result->curpos = result->curpos + 1;
                }
            }
        }
    }

    FINISHED:   
    if(db) SLPDDatabaseSrvRqstEnd(db);

    *sendbuf = result;

    return errorcode;
}


/*-------------------------------------------------------------------------*/
int ProcessSrvReg(SLPMessage message,
                  SLPBuffer recvbuf,
                  SLPBuffer* sendbuf,
                  int errorcode)
/*                                                                         */
/* Returns: non-zero if message should be silently dropped                 */
/*-------------------------------------------------------------------------*/
{
    SLPBuffer       result  = *sendbuf;

    /*--------------------------------------------------------------*/
    /* If errorcode is set, we can not be sure that message is good */
    /* Go directly to send response code  also do not process mcast */
    /* srvreg or srvdereg messages                                  */
    /*--------------------------------------------------------------*/
    if(errorcode || message->header.flags & SLP_FLAG_MCAST)
    {
        goto RESPOND;
    }

    /*------------------------------------*/
    /* Make sure that we handle the scope */
    /*------ -----------------------------*/
    if(SLPIntersectStringList(message->body.srvreg.scopelistlen,
                              message->body.srvreg.scopelist,
                              G_SlpdProperty.useScopesLen,
                              G_SlpdProperty.useScopes))
    {

#ifdef ENABLE_AUTHENTICATION
        /*-------------------------------*/
        /* Validate the authblocks       */
        /*-------------------------------*/
        errorcode = SLPAuthVerifyUrl(SLPSpiHandle hspi,
                                     0,
                                     &(message->body.srvreg.urlentry));
        if(errorcode == 0)
        {
            errorcode = SLPAuthVerifyString(SLPSpiHandle hspi,
                                            0,
                                            message->body.srvreg.attrlistlen,
                                            message->body.srvreg.attrlist,
                                            message->body.srvreg.authcount,
                                            message->body.srvreg.autharray);
        }
        if(errorcode == 0)
#endif
        {
            /*--------------------------------------------------------------*/
            /* Put the registration in the                                  */
            /*--------------------------------------------------------------*/
            /* TRICKY: Remember the recvbuf was duplicated back in          */
            /*         SLPDProcessMessage()                                 */
            
            if(ISLOCAL(message->peer.sin_addr))
            {
                message->body.srvreg.source= SLP_REG_SOURCE_LOCAL;
            }
            else
            {
                message->body.srvreg.source = SLP_REG_SOURCE_REMOTE;
            }

            errorcode = SLPDDatabaseReg(message, recvbuf);
        }
    }
    else
    {
        errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;
    }

    RESPOND:    
    /*--------------------------------------------------------------------*/
    /* don't send back reply anything multicast SrvReg (set result empty) */
    /*--------------------------------------------------------------------*/
    if(message->header.flags & SLP_FLAG_MCAST ||
       ISMCAST(message->peer.sin_addr))
    {
        result->end = result->start;
        goto FINISHED;
    }


    /*------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvack */
    /*------------------------------------------------------------*/
    result = SLPBufferRealloc(result,message->header.langtaglen + 16);
    if(result == 0)
    {
        errorcode = SLP_ERROR_INTERNAL_ERROR;
        goto FINISHED;
    }


    /*----------------*/
    /* Add the header */
    /*----------------*/
    /*version*/
    *(result->start)       = 2;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_SRVACK;
    /*length*/
    ToUINT24(result->start + 2,message->header.langtaglen + 16);
    /*flags*/
    ToUINT16(result->start + 5,0);
    /*ext offset*/
    ToUINT24(result->start + 7,0);
    /*xid*/
    ToUINT16(result->start + 10,message->header.xid);
    /*lang tag len*/
    ToUINT16(result->start + 12,message->header.langtaglen);
    /*lang tag*/
    memcpy(result->start + 14,
           message->header.langtag,
           message->header.langtaglen);

    /*-------------------*/
    /* Add the errorcode */
    /*-------------------*/
    ToUINT16(result->start + 14 + message->header.langtaglen, errorcode);

    FINISHED:
    *sendbuf = result;
    return errorcode;
}


/*-------------------------------------------------------------------------*/
int ProcessSrvDeReg(SLPMessage message,
                    SLPBuffer* sendbuf,
                    int errorcode)
/*                                                                         */
/* Returns: non-zero if message should be silently dropped                 */
/*-------------------------------------------------------------------------*/
{
    SLPBuffer result = *sendbuf;

    /*--------------------------------------------------------------*/
    /* If errorcode is set, we can not be sure that message is good */
    /* Go directly to send response code  also do not process mcast */
    /* srvreg or srvdereg messages                                  */
    /*--------------------------------------------------------------*/
    if(errorcode || message->header.flags & SLP_FLAG_MCAST)
    {
        goto RESPOND;
    }


    /*------------------------------------*/
    /* Make sure that we handle the scope */
    /*------------------------------------*/
    if(SLPIntersectStringList(message->body.srvdereg.scopelistlen,
                              message->body.srvdereg.scopelist,
                              G_SlpdProperty.useScopesLen,
                              G_SlpdProperty.useScopes))
    {
#ifdef ENABLE_AUTHENTICATION
        /*-------------------------------*/
        /* Validate the authblocks       */
        /*-------------------------------*/
        errorcode = SLPAuthVerifyUrl(SLPSpiHandle hspi,
                                     0,
                                     &(message->body.srvdereg.urlentry));
        if(errorcode == 0)
#endif
        { 
            /*--------------------------------------*/
            /* remove the service from the database */
            /*--------------------------------------*/
            errorcode = SLPDDatabaseDeReg(message);
        }
    }
    else
    {
        errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;
    }

    RESPOND:
    /*---------------------------------------------------------*/
    /* don't do anything multicast SrvDeReg (set result empty) */
    /*---------------------------------------------------------*/
    if(message->header.flags & SLP_FLAG_MCAST ||
       ISMCAST(message->peer.sin_addr))
    {

        result->end = result->start;
        goto FINISHED;
    }

    /*------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvack */
    /*------------------------------------------------------------*/
    result = SLPBufferRealloc(result,message->header.langtaglen + 16);
    if(result == 0)
    {
        errorcode = SLP_ERROR_INTERNAL_ERROR;
        goto FINISHED;
    }

    /*----------------*/
    /* Add the header */
    /*----------------*/
    /*version*/
    *(result->start)       = 2;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_SRVACK;
    /*length*/
    ToUINT24(result->start + 2,message->header.langtaglen + 16);
    /*flags*/
    ToUINT16(result->start + 5,0);
    /*ext offset*/
    ToUINT24(result->start + 7,0);
    /*xid*/
    ToUINT16(result->start + 10,message->header.xid);
    /*lang tag len*/
    ToUINT16(result->start + 12,message->header.langtaglen);
    /*lang tag*/
    memcpy(result->start + 14,
           message->header.langtag,
           message->header.langtaglen);

    /*-------------------*/
    /* Add the errorcode */
    /*-------------------*/
    ToUINT16(result->start + 14 + message->header.langtaglen, errorcode);

    FINISHED:
    *sendbuf = result;
    return errorcode;
}


/*-------------------------------------------------------------------------*/
int ProcessSrvAck(SLPMessage message,
                  SLPBuffer* sendbuf,
                  int errorcode)
/*-------------------------------------------------------------------------*/
{
    /* Ignore SrvAck.  Just return errorcode to caller */
    SLPBuffer result = *sendbuf;

    result->end = result->start;
    return message->body.srvack.errorcode;
}


/*-------------------------------------------------------------------------*/
int ProcessAttrRqst(SLPMessage message,
                    SLPBuffer* sendbuf,
                    int errorcode)
/*-------------------------------------------------------------------------*/
{
    SLPDDatabaseAttrRqstResult* db              = 0;
    int                         size            = 0;
    SLPBuffer                   result          = *sendbuf;
    
#ifdef ENABLE_AUTHENTICATION
    unsigned char*    attrauth       = 0;
    int               attrauthlen    = 0;
#endif

    /*--------------------------------------------------------------*/
    /* If errorcode is set, we can not be sure that message is good */
    /* Go directly to send response code                            */
    /*--------------------------------------------------------------*/
    if(errorcode)
    {
        goto RESPOND;
    }

    /*-------------------------------------------------*/
    /* Check for one of our IP addresses in the prlist */
    /*-------------------------------------------------*/
    if(SLPIntersectStringList(message->body.attrrqst.prlistlen,
                              message->body.attrrqst.prlist,
                              G_SlpdProperty.interfacesLen,
                              G_SlpdProperty.interfaces))
    {
        /* Silently ignore */
        result->end = result->start;
        goto FINISHED;
    }


    /*------------------------------------*/
    /* Make sure that we handle the scope */
    /*------ -----------------------------*/
    if(SLPIntersectStringList(message->body.attrrqst.scopelistlen,
                              message->body.attrrqst.scopelist,
                              G_SlpdProperty.useScopesLen,
                              G_SlpdProperty.useScopes))
    {
        /*------------------------------------*/
        /* Make sure that we handle the SPI   */
        /*------------------------------------*/
#ifdef ENABLE_AUTHENTICATION
        if(G_SlpdProperty.enableSecurity && 
           SLPSpiCanSign(SLPSpiHandle hspi,
                         message->body.attrrqst.spistrlen,
                         message->body.attrrqst.spistr) == 0)
            {
                result = SLP_ERROR_AUTHENTICATION_UNKNOWN;
                goto RESPOND;
            }
        }
        else if(message->body.attrrqst.spistrlen == 0)
#else
        if(message->body.attrrqst.spistrlen == 0)
#endif
        {
            /*---------------------------------*/
            /* Find attributes in the database */
            /*---------------------------------*/
            errorcode = SLPDDatabaseAttrRqstStart(message,&db);
        }
        else
        {
            errorcode = SLP_ERROR_AUTHENTICATION_UNKNOWN;
        }
    }
    else
    {
        errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;
    }


    RESPOND:
    /*----------------------------------------------------------------*/
    /* Do not send error codes or empty replies to multicast requests */
    /*----------------------------------------------------------------*/
    if(errorcode != 0 || db->attrlistlen == 0)
    {
        if(message->header.flags & SLP_FLAG_MCAST ||
           ISMCAST(message->peer.sin_addr))
        {
            result->end = result->start;
            goto FINISHED;  
        }
    }


    /*--------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole attrrply */
    /*--------------------------------------------------------------*/
    size = message->header.langtaglen + 20; /* 14 bytes for header     */
                                            /*  2 bytes for error code */
                                            /*  2 bytes for attr-list len */
                                            /*  2 bytes for the authcount */
    size += db->attrlistlen;
   
#ifdef ENABLE_AUTHENTICATION
    /*--------------------*/
    /* Generate authblock */
    /*--------------------*/
    if(doauth)
    {
        errorcode = SLPAuthSignString(message->body.attrrqst.spistrlen,
                                      message->body.attrrqst.spistr,
                                      attr.attrlistlen,
                                      attr.attrlist,
                                      &attrauthlen,
                                      &attrauth);
        size += attrauthlen;
    }
#endif                                    
    
    /*-------------------*/
    /* Alloc the  buffer */
    /*-------------------*/
    result = SLPBufferRealloc(result,size);
    if(result == 0)
    {
        errorcode = SLP_ERROR_INTERNAL_ERROR;
        goto FINISHED;
    }

    /*----------------*/
    /* Add the header */
    /*----------------*/
    /*version*/
    *(result->start)       = 2;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_ATTRRPLY;
    /*length*/
    ToUINT24(result->start + 2,size);
    /*flags*/
    ToUINT16(result->start + 5,
             (size > SLP_MAX_DATAGRAM_SIZE ? SLP_FLAG_OVERFLOW : 0));
    /*ext offset*/
    ToUINT24(result->start + 7,0);
    /*xid*/
    ToUINT16(result->start + 10,message->header.xid);
    /*lang tag len*/
    ToUINT16(result->start + 12,message->header.langtaglen);
    /*lang tag*/
    memcpy(result->start + 14,
           message->header.langtag,
           message->header.langtaglen);

    /*--------------------------*/
    /* Add rest of the AttrRqst */
    /*--------------------------*/
    result->curpos = result->start + 14 + message->header.langtaglen;
    /* error code*/
    ToUINT16(result->curpos, errorcode);
    result->curpos = result->curpos + 2;
    if(errorcode == 0)
    {
        /* attr-list len */
        ToUINT16(result->curpos, db->attrlistlen);
        result->curpos = result->curpos + 2;
        if (db->attrlistlen)
        {
            memcpy(result->curpos, db->attrlist, db->attrlistlen);
        }
        result->curpos = result->curpos + db->attrlistlen;
        /* authentication block */
    #ifdef ENABLE_AUTHENTICATION
        if(doauth && attrauth)
        {
            /* authcount */
            *(result->curpos) = 1;
            result->curpos = result->curpos + 1;
            /* authblock */
            memcpy(result->curpos,attrauth,attrauthlen);
            result->curpos = result->curpos + attrauthlen;
        }
        else
    #endif
        {
            /* authcount */
            *(result->curpos) = 0;
            result->curpos = result->curpos + 1;
        }
    }
    

    FINISHED:
    
#ifdef ENABLE_AUTHENTICATION    
    /* free the authblock if any */
    if(attrauth) free(attrauth);
#endif
    
    if(db) SLPDDatabaseAttrRqstEnd(db);
    
    *sendbuf = result;

    return errorcode;
}        


/*-------------------------------------------------------------------------*/
int ProcessDAAdvert(SLPMessage message,
                    SLPBuffer* sendbuf,
                    int errorcode)
/*-------------------------------------------------------------------------*/
{
    SLPDAEntry daentry;
    SLPBuffer result = *sendbuf;

    /*--------------------------------------------------------------*/
    /* If errorcode is set, we can not be sure that message is good */
    /* Go directly to send response code                            */
    /*--------------------------------------------------------------*/
    if(errorcode)
    {
        goto RESPOND;
    }

    /*-------------------------------*/
    /* Validate the authblocks       */
    /*-------------------------------*/
#ifdef ENABLE_AUTHENTICATION
    errorcode = SLPAuthVerifyDAAdvert(0,&(message->body.daadvert));
    if(errorcode == 0);
#endif
    {
        /* Only process if errorcode is not set */
        if(message->body.daadvert.errorcode == SLP_ERROR_OK)
        {
            /* TODO: Authentication stuff here */
    
    
            daentry.langtaglen = message->header.langtaglen;
            daentry.langtag = message->header.langtag;
            daentry.bootstamp = message->body.daadvert.bootstamp;
            daentry.urllen = message->body.daadvert.urllen;
            daentry.url = message->body.daadvert.url;
            daentry.scopelistlen = message->body.daadvert.scopelistlen;
            daentry.scopelist = message->body.daadvert.scopelist;
            daentry.attrlistlen = message->body.daadvert.attrlistlen;
            daentry.attrlist = message->body.daadvert.attrlist;
            daentry.spilistlen = message->body.daadvert.spilistlen;
            daentry.spilist = message->body.daadvert.spilist;
            SLPDKnownDAAdd(&(message->peer.sin_addr),&daentry);
        }
    }

    RESPOND:
    /* DAAdverts should never be replied to.  Set result buffer to empty*/
    result->end = result->start;


    *sendbuf = result;
    return errorcode;
}


/*-------------------------------------------------------------------------*/
int ProcessSrvTypeRqst(SLPMessage message,
                       SLPBuffer* sendbuf,
                       int errorcode)
/*-------------------------------------------------------------------------*/
{
    int                             size    = 0;
    SLPDDatabaseSrvTypeRqstResult*  db      = 0;
    SLPBuffer                       result  = *sendbuf;


    /*-------------------------------------------------*/
    /* Check for one of our IP addresses in the prlist */
    /*-------------------------------------------------*/
    if(SLPIntersectStringList(message->body.srvtyperqst.prlistlen,
                              message->body.srvtyperqst.prlist,
                              G_SlpdProperty.interfacesLen,
                              G_SlpdProperty.interfaces))
    {
        /* Silently ignore */
        result->end = result->start;
        goto FINISHED;
    }

    /*------------------------------------*/
    /* Make sure that we handle the scope */
    /*------ -----------------------------*/
    if(SLPIntersectStringList(message->body.srvtyperqst.scopelistlen,
                              message->body.srvtyperqst.scopelist,
                              G_SlpdProperty.useScopesLen,
                              G_SlpdProperty.useScopes) != 0)
    {
        /*------------------------------------*/
        /* Find service types in the database */
        /*------------------------------------*/
        errorcode = SLPDDatabaseSrvTypeRqstStart(message, &db);
    }
    else
    {
        errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;
    }

    /*----------------------------------------------------------------*/
    /* Do not send error codes or empty replies to multicast requests */
    /*----------------------------------------------------------------*/
    if(errorcode != 0 || db->srvtypelistlen == 0)
    {
        if(message->header.flags & SLP_FLAG_MCAST ||
           ISMCAST(message->peer.sin_addr))
        {
            result->end = result->start;
            goto FINISHED;  
        }
    }

    /*-----------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvtyperply */
    /*-----------------------------------------------------------------*/
    size = message->header.langtaglen + 18; /* 14 bytes for header     */
                                            /*  2 bytes for error code */
                                            /*  2 bytes for srvtype
                                                list length  */
    size += db->srvtypelistlen;
    

    /*------------------------------*/
    /* Reallocate the result buffer */
    /*------------------------------*/
    result = SLPBufferRealloc(result,size);
    if(result == 0)
    {
        errorcode = SLP_ERROR_INTERNAL_ERROR;
        goto FINISHED;
    }   

    /*----------------*/
    /* Add the header */
    /*----------------*/
    /*version*/
    *(result->start)       = 2;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_SRVTYPERPLY;
    /*length*/
    ToUINT24(result->start + 2,size);
    /*flags*/
    ToUINT16(result->start + 5,
             (size > SLP_MAX_DATAGRAM_SIZE ? SLP_FLAG_OVERFLOW : 0));
    /*ext offset*/
    ToUINT24(result->start + 7,0);
    /*xid*/
    ToUINT16(result->start + 10,message->header.xid);
    /*lang tag len*/
    ToUINT16(result->start + 12,message->header.langtaglen);
    /*lang tag*/
    memcpy(result->start + 14,
           message->header.langtag,
           message->header.langtaglen);

    /*-----------------------------*/
    /* Add rest of the SrvTypeRply */
    /*-----------------------------*/
    result->curpos = result->start + 14 + message->header.langtaglen;

    /* error code*/
    ToUINT16(result->curpos, errorcode);
    result->curpos += 2;

    if(errorcode == 0)
    {
        /* length of srvtype-list */
        ToUINT16(result->curpos, db->srvtypelistlen);
        result->curpos += 2;     
        memcpy(result->curpos, 
               db->srvtypelist,
               db->srvtypelistlen);
        result->curpos += db->srvtypelistlen;
    }
    

    FINISHED:   
    if(db) SLPDDatabaseSrvTypeRqstEnd(db);

    return errorcode;
}

/*-------------------------------------------------------------------------*/
int ProcessSAAdvert(SLPMessage message,
                    SLPBuffer* sendbuf,
                    int errorcode)
/*-------------------------------------------------------------------------*/
{
    /* Ignore all SAADVERTS */
    (*sendbuf)->end = (*sendbuf)->start;
    return errorcode;
}


/*=========================================================================*/
int SLPDProcessMessage(struct sockaddr_in* peerinfo,
                       SLPBuffer recvbuf,
                       SLPBuffer* sendbuf)
/* Processes the recvbuf and places the results in sendbuf                 */
/*                                                                         */
/* recvfd   - the socket the message was received on                       */
/*                                                                         */
/* recvbuf  - message to process                                           */
/*                                                                         */
/* sendbuf  - results of the processed message                             */
/*                                                                         */
/* Returns  - zero on success SLP_ERROR_PARSE_ERROR or                     */
/*            SLP_ERROR_INTERNAL_ERROR on ENOMEM.                          */
/*=========================================================================*/
{
    SLPHeader   header;
    SLPMessage  message     = 0;
    int         errorcode   = 0;
    
    /* Parse just the message header the reset the buffer "curpos" pointer */
    errorcode = SLPMessageParseHeader(recvbuf,&header);
    recvbuf->curpos = recvbuf->start;
#if defined(ENABLE_SLPv1)   
    if(errorcode == SLP_ERROR_VER_NOT_SUPPORTED &&
       header.version == 1)
    {
        errorcode = SLPDv1ProcessMessage(peerinfo, 
                                         recvbuf, 
                                         sendbuf);
    }
    else
#endif
    {
        /* TRICKY: Duplicate SRVREG recvbufs *before* parsing them   */
        /*         it because we are going to keep them in the       */
        if(header.functionid == SLP_FUNCT_SRVREG)
        {
            recvbuf = SLPBufferDup(recvbuf);
            if(recvbuf == NULL)
            {
                return SLP_ERROR_INTERNAL_ERROR;
            }
        }
    
        /* Allocate the message descriptor */
        message = SLPMessageAlloc();
        if(message)
        {
            /* Parse the message and fill out the message descriptor */
            errorcode = SLPMessageParseBuffer(peerinfo,recvbuf, message);
            if(errorcode == 0)
            {    
                SLPDLogMessage("Incoming",message);
                {
                    /* Process messages based on type */
                    switch(message->header.functionid)
                    {
                    case SLP_FUNCT_SRVRQST:
                        errorcode = ProcessSrvRqst(message,sendbuf,errorcode);
                        break;
                
                    case SLP_FUNCT_SRVREG:
                        errorcode = ProcessSrvReg(message,recvbuf,sendbuf,errorcode);
                        if(errorcode == 0)
                        {
                            SLPDKnownDAEcho(&(message->peer),message, recvbuf);
                        }
                        break;
                
                    case SLP_FUNCT_SRVDEREG:
                        errorcode = ProcessSrvDeReg(message,sendbuf,errorcode);
                        if(errorcode == 0)
                        {
                            SLPDKnownDAEcho(&(message->peer),message, recvbuf);
                        }
                        break;
                
                    case SLP_FUNCT_SRVACK:
                        errorcode = ProcessSrvAck(message,sendbuf, errorcode);        
                        break;
                
                    case SLP_FUNCT_ATTRRQST:
                        errorcode = ProcessAttrRqst(message,sendbuf, errorcode);
                        break;
                
                    case SLP_FUNCT_DAADVERT:
                        errorcode = ProcessDAAdvert(message, sendbuf, errorcode);
                        /* If necessary log that we received a DAAdvert */
                        SLPDLogDAAdvertisement("IN", message);
                        break;
                
                    case SLP_FUNCT_SRVTYPERQST:
                        errorcode = ProcessSrvTypeRqst(message, sendbuf, errorcode);
                        break;
                
                    case SLP_FUNCT_SAADVERT:
                        errorcode = ProcessSAAdvert(message, sendbuf, errorcode);
                        break;
                
                    default:
                        /* Should never happen... but we're paranoid */
                        errorcode = SLP_ERROR_PARSE_ERROR;
                        break;
                    }
                    
                    /* Log traceMsg of message will be sent */
                    /* that will be sent                    */
                    /*SLPDLogMessage("Outgoing",peerinfo,*sendbuf);*/
                }   
            }
    
            /* TRICKY: Do not free the message descriptor for SRVREGs */
            /*         because we are keeping them in the database    */
            if(errorcode == 0 && header.functionid == SLP_FUNCT_SRVREG)
            {
                /* Don't free the message descriptor cause it referenced */
                /* by the the database                                   */
            }
            else
            {
                SLPMessageFree(message);
            }
        }
        else
        {
            /* out of memory */
            errorcode = SLP_ERROR_INTERNAL_ERROR;
        }
    }
    
    /* Log reception of important errors */
    switch(errorcode)
    {
    case SLP_ERROR_DA_BUSY_NOW:
        SLPDLog("DA_BUSY from %s\n",
                inet_ntoa(peerinfo->sin_addr));
        break;
    case SLP_ERROR_INTERNAL_ERROR:
        SLPDLog("INTERNAL_ERROR from %s\n",
                inet_ntoa(peerinfo->sin_addr));
        break;
    case SLP_ERROR_PARSE_ERROR:
        SLPDLog("PARSE_ERROR from %s\n",
                inet_ntoa(peerinfo->sin_addr));
        break;
    case SLP_ERROR_VER_NOT_SUPPORTED:
        SLPDLog("VER_NOT_SUPPORTED from %s\n",
                inet_ntoa(peerinfo->sin_addr));
        break;                    
    }
    
    return errorcode;
}                
