/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_v1process.c                                           */
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
#include "../common/slp_v1message.h"
#include "../common/slp_utf8.h"
#include "../common/slp_da.h"
#include "../common/slp_compare.h"



/*-------------------------------------------------------------------------*/
int v1ProcessDASrvRqst(struct sockaddr_in* peeraddr,
                       SLPMessage message,
                       SLPBuffer* sendbuf,
                       int errorcode)
/*-------------------------------------------------------------------------*/
{
    SLPDAEntry      daentry;

    /* set up local data */
    memset(&daentry,0,sizeof(daentry));

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

    /* don't return errorcodes to multicast messages */
    if(errorcode == 0)
    {
        if(message->header.flags & SLP_FLAG_MCAST ||
           ISMCAST(peeraddr->sin_addr))
        {
            (*sendbuf)->end = (*sendbuf)->start;
            return errorcode;
        }
    }

    errorcode = SLPDv1KnownDAEntryToDAAdvert(errorcode,
                                             message->header.encoding,
                                             message->header.xid,
                                             &daentry,
                                             sendbuf);
    return errorcode;
}


/*-------------------------------------------------------------------------*/
int v1ProcessSrvRqst(struct sockaddr_in* peeraddr,
                     SLPMessage message,
                     SLPBuffer* sendbuf,
                     int errorcode)
/*-------------------------------------------------------------------------*/
{
    int                         i;
    int                         urllen;
    int                         size        = 0;
    SLPDDatabaseSrvRqstResult*  db          = 0;
    SLPBuffer                   result      = *sendbuf;

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
                              G_SlpdProperty.interfaces))
    {
        result->end = result->start;
        goto FINISHED;
    }

    /*------------------------------------------------*/
    /* Check to to see if a this is a special SrvRqst */
    /*------------------------------------------------*/
    if(SLPCompareString(message->body.srvrqst.srvtypelen,
                        message->body.srvrqst.srvtype,
                        15,
                        "directory-agent") == 0)
    {
        errorcode = v1ProcessDASrvRqst(peeraddr, message, sendbuf, errorcode);
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


    RESPOND:
    /*----------------------------------------------------------------*/
    /* Do not send error codes or empty replies to multicast requests */
    /*----------------------------------------------------------------*/
    if(message->header.flags & SLP_FLAG_MCAST)
    {
        if(errorcode != 0 || db->urlcount == 0)
        {
            result->end = result->start;
            goto FINISHED;  
        }
    }

    /*-------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvrply */
    /*-------------------------------------------------------------*/
    size = 16; /* 12 bytes for header,  2 bytes for error code,   2
                  bytes for url count */
    if(errorcode == 0)
    {
        for(i=0;i<db->urlcount;i++)
        {
            urllen = INT_MAX;
            errorcode = SLPv1ToEncoding(0, 
                                        &urllen,
                                        message->header.encoding,  
                                        db->urlarray[i]->url,
                                        db->urlarray[i]->urllen);
            if(errorcode)
                break;
            size += urllen + 4; /* 2 bytes for lifetime,
                   2 bytes for urllen */
        } 
        result = SLPBufferRealloc(result,size);
        if(result == 0)
        {
            errorcode = SLP_ERROR_INTERNAL_ERROR;
        }
    }

    /*----------------*/
    /* Add the header */
    /*----------------*/
    /*version*/
    *(result->start)       = 1;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_SRVRPLY;
    /*length*/
    ToUINT16(result->start + 2, size);
    /*flags - TODO set the flags correctly */
    *(result->start + 4) = message->header.flags |
                           (size > SLP_MAX_DATAGRAM_SIZE ? SLPv1_FLAG_OVERFLOW : 0);  
    /*dialect*/
    *(result->start + 5) = 0;
    /*language code*/
    memcpy(result->start + 6, message->header.langtag, 2);
    ToUINT16(result->start + 8, message->header.encoding);
    /*xid*/
    ToUINT16(result->start + 10, message->header.xid);

    /*-------------------------*/
    /* Add rest of the SrvRply */
    /*-------------------------*/
    result->curpos = result->start + 12;
    /* error code*/
    ToUINT16(result->curpos, errorcode);
    result->curpos = result->curpos + 2;
    if(errorcode)
    {
        /* urlentry count */
        ToUINT16(result->curpos, db->urlcount);
        result->curpos = result->curpos + 2;
        for(i=0;i<db->urlcount;i++)
        {
            /* url-entry lifetime */
            ToUINT16(result->curpos, db->urlarray[i]->lifetime);
            result->curpos = result->curpos + 2;
            /* url-entry url and urllen */
            urllen = size;      
            errorcode = SLPv1ToEncoding(result->curpos + 2, 
                                        &urllen,
                                        message->header.encoding,  
                                        db->urlarray[i]->url,
                                        db->urlarray[i]->urllen);
            ToUINT16(result->curpos, urllen);
            result->curpos = result->curpos + 2 + urllen;
        }
    }

    FINISHED:   
    
    SLPDDatabaseSrvRqstEnd(db);

    *sendbuf = result;
    
    return errorcode;
}

/*-------------------------------------------------------------------------*/
int v1ProcessSrvReg(struct sockaddr_in* peeraddr,
                    SLPMessage message,
                    SLPBuffer recvbuf,
                    SLPBuffer* sendbuf,
                    int errorcode)
/*                                                                         */
/* Returns: non-zero if message should be silently dropped                 */
/*-------------------------------------------------------------------------*/
{
    SLPBuffer       result = *sendbuf;

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
        /*---------------------------------*/
        /* put the service in the database */
        /*---------------------------------*/
        if(ISLOCAL(message->peer.sin_addr))
        {
            message->body.srvreg.source= SLP_REG_SOURCE_LOCAL;
        }
        else
        {
            message->body.srvreg.source = SLP_REG_SOURCE_REMOTE;
        }

        errorcode = SLPDDatabaseReg(message,recvbuf);
    }
    else
    {
        errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;
    }

    RESPOND:    
    /*--------------------------------------------------------------------*/
    /* don't send back reply anything multicast SrvReg (set result empty) */
    /*--------------------------------------------------------------------*/
    if(message->header.flags & SLP_FLAG_MCAST)
    {
        result->end = result->start;
        goto FINISHED;
    }


    /*------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvack */
    /*------------------------------------------------------------*/
    result = SLPBufferRealloc(result, 14);
    if(result == 0)
    {
        errorcode = SLP_ERROR_INTERNAL_ERROR;
        goto FINISHED;
    }

    /*----------------*/
    /* Add the header */
    /*----------------*/
    /*version*/
    *(result->start)       = 1;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_SRVACK;
    /*length*/
    ToUINT16(result->start + 2, 14);
    /*flags - TODO set the flags correctly */
    *(result->start + 4) = 0;
    /*dialect*/
    *(result->start + 5) = 0;
    /*language code*/
    memcpy(result->start + 6, message->header.langtag, 2);
    ToUINT16(result->start + 8, message->header.encoding);
    /*xid*/
    ToUINT16(result->start + 10, message->header.xid);

    /*-------------------*/
    /* Add the errorcode */
    /*-------------------*/
    ToUINT16(result->start + 12, errorcode);

    FINISHED:
    *sendbuf = result;
    return errorcode;
}


/*-------------------------------------------------------------------------*/
int v1ProcessSrvDeReg(struct sockaddr_in* peeraddr,
                      SLPMessage message,
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
        /*--------------------------------------*/
        /* remove the service from the database */
        /*--------------------------------------*/
        errorcode = SLPDDatabaseDeReg(message);
    }
    else
    {
        errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;
    }

    RESPOND:
    /*---------------------------------------------------------*/
    /* don't do anything multicast SrvDeReg (set result empty) */
    /*---------------------------------------------------------*/
    if(message->header.flags & SLP_FLAG_MCAST)
    {

        result->end = result->start;
        goto FINISHED;
    }

    /*------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvack */
    /*------------------------------------------------------------*/
    result = SLPBufferRealloc(result, 14);
    if(result == 0)
    {
        errorcode = SLP_ERROR_INTERNAL_ERROR;
        goto FINISHED;
    }

    /*----------------*/
    /* Add the header */
    /*----------------*/
    /*version*/
    *(result->start)       = 1;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_SRVACK;
    /*length*/
    ToUINT16(result->start + 2, 14);
    /*flags - TODO set the flags correctly */
    *(result->start + 4) = 0;
    /*dialect*/
    *(result->start + 5) = 0;
    /*language code*/
    memcpy(result->start + 6, message->header.langtag, 2);
    ToUINT16(result->start + 8, message->header.encoding);
    /*xid*/
    ToUINT16(result->start + 10, message->header.xid);

    /*-------------------*/
    /* Add the errorcode */
    /*-------------------*/
    ToUINT16(result->start + 12, errorcode);

    FINISHED:
    *sendbuf = result;
    return errorcode;
}

/*-------------------------------------------------------------------------*/
int v1ProcessAttrRqst(struct sockaddr_in* peeraddr,
                      SLPMessage message,
                      SLPBuffer* sendbuf,
                      int errorcode)
/*-------------------------------------------------------------------------*/
{
    SLPDDatabaseAttrRqstResult* db              = 0;
    int                         attrlen     = 0;
    int                         size        = 0;
    SLPBuffer                   result      = *sendbuf;

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
        /*---------------------------------*/
        /* Find attributes in the database */
        /*---------------------------------*/
        errorcode = SLPDDatabaseAttrRqstStart(message,&db);
    }
    else
    {
        errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;
    }

    RESPOND:
    /*----------------------------------------------------------------*/
    /* Do not send error codes or empty replies to multicast requests */
    /*----------------------------------------------------------------*/
    if(message->header.flags & SLP_FLAG_MCAST)
    {
        if(errorcode != 0 || db->attrlistlen == 0)
        {
            result->end = result->start;
            goto FINISHED;  
        }
    }

    /*--------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole attrrply */
    /*--------------------------------------------------------------*/
    size = 16; /* 12 bytes for header, 2 bytes for error code, 2 bytes
                for attr-list len */
    if(errorcode == 0)
    {
        
        attrlen = INT_MAX;
        errorcode = SLPv1ToEncoding(0, &attrlen,
                                    message->header.encoding,  
                                    db->attrlist,
                                    db->attrlistlen);
        size += attrlen;
    }
    
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
    *(result->start)       = 1;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_ATTRRPLY;
    /*length*/
    ToUINT16(result->start + 2, size);
    /*flags - TODO set the flags correctly */
    *(result->start + 4) = message->header.flags |
                           (size > SLP_MAX_DATAGRAM_SIZE ? SLPv1_FLAG_OVERFLOW : 0);  
    /*dialect*/
    *(result->start + 5) = 0;
    /*language code*/
    memcpy(result->start + 6, message->header.langtag, 2);
    ToUINT16(result->start + 8, message->header.encoding);
    /*xid*/
    ToUINT16(result->start + 10, message->header.xid);

    /*--------------------------*/
    /* Add rest of the AttrRply */
    /*--------------------------*/
    result->curpos = result->start + 12;
    /* error code*/
    ToUINT16(result->curpos, errorcode);
    result->curpos = result->curpos + 2; 
    if(errorcode == 0)
    {
        /* attr-list len */
        ToUINT16(result->curpos, attrlen);
        result->curpos = result->curpos + 2;
        attrlen = size;
        SLPv1ToEncoding(result->curpos, &attrlen,
                        message->header.encoding,
                        db->attrlist,
                        db->attrlistlen);
        result->curpos = result->curpos + attrlen; 
    }
    
    FINISHED:
    *sendbuf = result;
    if(db) SLPDDatabaseAttrRqstEnd(db);

    return errorcode;
}        


/*-------------------------------------------------------------------------*/
int v1ProcessSrvTypeRqst(struct sockaddr_in* peeraddr,
                         SLPMessage message,
                         SLPBuffer* sendbuf,
                         int errorcode)
/*-------------------------------------------------------------------------*/
{
    char*                           type;
    char*                           end;
    char*                           slider;
    int                             i;
    int                             typelen;
    int                             numsrvtypes = 0;
    int                             size        = 0;
    SLPDDatabaseSrvTypeRqstResult*  db          = 0;
    SLPBuffer                       result      = *sendbuf;


    /*-------------------------------------------------*/
    /* Check for one of our IP addresses in the prlist */
    /*-------------------------------------------------*/
    if(SLPIntersectStringList(message->body.srvtyperqst.prlistlen,
                              message->body.srvtyperqst.prlist,
                              G_SlpdProperty.interfacesLen,
                              G_SlpdProperty.interfaces))
    {
        result->end = result->start;
        goto FINISHED;
    }

    /*------------------------------------*/
    /* Make sure that we handle the scope */
    /*------------------------------------*/
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
    if(message->header.flags & SLP_FLAG_MCAST)
    {
        if(errorcode != 0 || db->srvtypelistlen == 0)
        {
            result->end = result->start;
            goto FINISHED;  
        }
    }


    /*-----------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvtyperply */
    /*-----------------------------------------------------------------*/
    size = 16; /* 12 bytes for header, 2 bytes for error code, 2 bytes
                  for num of service types */
    if(errorcode == 0)
    {
        if(db->srvtypelistlen) 
        {
            /* there has to be at least one service type*/
            numsrvtypes = 1;

            /* count the rest of the service types */
            type = db->srvtypelist;
            for(i=0; i< db->srvtypelistlen; i++)
            {
                if(type[i] == ',')
                {
                    numsrvtypes += 1;
                }
            }
            
            /* figure out how much memory is required for srvtype strings */
            typelen = INT_MAX;
            errorcode = SLPv1ToEncoding(0,
                                        &typelen,
                                        message->header.encoding,  
                                        db->srvtypelist,
                                        db->srvtypelistlen);
            
            /* TRICKY: we add in the numofsrvtypes + 1 to make room for the */
            /* type length.  We can do this because the ',' of the comma    */
            /* delimited list is one byte.                                  */
            size = size + typelen + numsrvtypes + 1;
        }
        else
        {
            numsrvtypes = 0;
        }
    }
    
    /*-----------------*/
    /* Allocate memory */
    /*-----------------*/
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
    *(result->start)       = 1;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_SRVTYPERPLY;
    /*length*/
    ToUINT16(result->start + 2, size);
    /*flags - TODO set the flags correctly */
    *(result->start + 4) = message->header.flags |
                           (size > SLP_MAX_DATAGRAM_SIZE ? SLPv1_FLAG_OVERFLOW : 0);  
    /*dialect*/
    *(result->start + 5) = 0;
    /*language code*/
    memcpy(result->start + 6, message->header.langtag, 2);
    ToUINT16(result->start + 8, message->header.encoding);
    /*xid*/
    ToUINT16(result->start + 10, message->header.xid);

    /*-----------------------------*/
    /* Add rest of the SrvTypeRply */
    /*-----------------------------*/
    result->curpos = result->start + 12;
    /* error code*/
    ToUINT16(result->curpos, errorcode);
    result->curpos += 2;
    if(errorcode == 0)
    {
        /* num of service types */
        ToUINT16(result->curpos, numsrvtypes);
        result->curpos += 2;
        
        /* service type strings */
        type = db->srvtypelist;
        slider = db->srvtypelist;
        end = &(type[db->srvtypelistlen]);
        for(i=0;i<numsrvtypes; i++)
        {
            while(slider < end && *slider != ',') slider++;
            
            typelen = size;
            /* put in the encoded service type */
            SLPv1ToEncoding(result->curpos + 2, 
                            &typelen,
                            message->header.encoding,
                            type,
                            slider - type);
            /* slip in the typelen */
            ToUINT16(result->curpos, typelen);
            result->curpos += 2;
            result->curpos += typelen;

            slider ++; /* skip comma */
            type = slider;
        }

        /* TODO - make sure we don't return generic types */
    }

    FINISHED:   
    if(db) SLPDDatabaseSrvTypeRqstEnd(db);

    *sendbuf = result;

    return errorcode;
}

/*=========================================================================*/
int SLPDv1ProcessMessage(struct sockaddr_in* peeraddr,
                         SLPBuffer recvbuf,
                         SLPBuffer* sendbuf)
/* Processes the SLPv1 message and places the results in sendbuf           */
/*                                                                         */
/* recvfd   - the socket the message was received on                       */
/*                                                                         */
/* recvbuf  - message to process                                           */
/*                                                                         */
/* sendbuf  - results of the processed message                             */
/*                                                                         */
/* message  - the parsed message from recvbuf                              */
/*                                                                         */
/* errorcode - error code from parsing the SLPv1 packet                    */
/*                                                                         */
/* Returns  - zero on success SLP_ERROR_PARSE_ERROR or                     */
/*            SLP_ERROR_INTERNAL_ERROR on ENOMEM.                          */
/*=========================================================================*/
{
    SLPHeader   header;
    SLPMessage  message;
    int         errorcode   = 0;

    if(!G_SlpdProperty.isDA)
    {
        /* SLPv1 messages are handled only by DAs */
        errorcode = SLP_ERROR_VER_NOT_SUPPORTED;
    }

    /* Parse just the message header the reset the buffer "curpos" pointer */
    errorcode = SLPv1MessageParseHeader(recvbuf, &header);
    recvbuf->curpos = recvbuf->start;
    if(errorcode)
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
            errorcode = SLPv1MessageParseBuffer(peeraddr,recvbuf, message);
            
            SLPDLogMessage("Incoming",message);
            
            switch(message->header.functionid)
            {
            case SLP_FUNCT_SRVRQST:
                errorcode = v1ProcessSrvRqst(peeraddr, message, sendbuf, errorcode);
                break;
        
            case SLP_FUNCT_SRVREG:
                errorcode = v1ProcessSrvReg(peeraddr,
                                            message,
                                            recvbuf,
                                            sendbuf,
                                            errorcode);
                /* We are a DA so there is no need to register with known DAs */
                break;
        
            case SLP_FUNCT_SRVDEREG:
                errorcode = v1ProcessSrvDeReg(peeraddr, message, sendbuf, errorcode);
                /* We are a DA so there is no need to deregister with known DAs */
                break;
        
            case SLP_FUNCT_ATTRRQST:
                errorcode = v1ProcessAttrRqst(peeraddr, message, sendbuf, errorcode);
                break;
        
            case SLP_FUNCT_SRVTYPERQST:
                errorcode = v1ProcessSrvTypeRqst(peeraddr, message, sendbuf, errorcode);
                break;
        
            case SLP_FUNCT_DAADVERT:
                errorcode = 0;      /* we are a SLPv2 DA, ignore other DAs */
                (*sendbuf)->end = (*sendbuf)->start;
                break;
        
            default:
                /* Should never happen... but we're paranoid */
                errorcode = SLP_ERROR_PARSE_ERROR;
                break;
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

    return errorcode;
}                
