/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_knownda.c                                             */
/*                                                                         */
/* Abstract:    Keeps track of known DAs                                   */
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

#include "slpd.h"

/*=========================================================================*/
/* slpd includes                                                           */
/*=========================================================================*/
#include "slpd_knownda.h"
#include "slpd_property.h"
#include "slpd_database.h"
#include "slpd_socket.h"
#include "slpd_outgoing.h"
#include "slpd_log.h"


/*=========================================================================*/
/* common code includes                                                    */
/*=========================================================================*/
#include "../common/slp_xmalloc.h"
#include "../common/slp_v1message.h"
#include "../common/slp_utf8.h"
#include "../common/slp_compare.h"
#include "../common/slp_xid.h"


/*=========================================================================*/
SLPList G_KnownDAList = {0,0,0};                                         
/* The list of DAs known to slpd.                                          */
/*=========================================================================*/

/*=========================================================================*/
int G_KnownDATimeSinceLastRefresh = 0;
/*=========================================================================*/

/*-------------------------------------------------------------------------*/
int MakeActiveDiscoveryRqst(int ismcast, SLPBuffer* buffer)
/* Pack a buffer with service:directory-agent SrvRqst                      */
/*-------------------------------------------------------------------------*/
{
    size_t          size;
    SLPDAEntry*     daentry;
    SLPBuffer       result      = 0;
    char*           prlist      = 0;
    size_t          prlistlen   = 0;
    int             errorcode   = 0;

    /*-------------------------------------------------*/
    /* Generate a DA service request buffer to be sent */
    /*-------------------------------------------------*/
    /* determine the size of the fixed portion of the SRVRQST         */
    size  = 47; /* 14 bytes for the header                        */
                /*  2 bytes for the prlistlen                     */
                /*  2 bytes for the srvtype length                */
                /* 23 bytes for "service:directory-agent" srvtype */
                /*  2 bytes for scopelistlen                      */
                /*  2 bytes for predicatelen                      */
                /*  2 bytes for sprstrlen                         */

    /* figure out what our Prlist will be by going through our list of  */
    /* known DAs                                                        */
    prlistlen = 0;
    prlist = xmalloc(SLP_MAX_DATAGRAM_SIZE);
    if(prlist == 0)
    {
        /* out of memory */
        errorcode = SLP_ERROR_INTERNAL_ERROR;
        goto FINISHED;
    }

    *prlist = 0;
    daentry = (SLPDAEntry*)G_KnownDAList.head;
    while(daentry && prlistlen < SLP_MAX_DATAGRAM_SIZE)
    {
        strcat(prlist,inet_ntoa(daentry->daaddr));
        daentry = (SLPDAEntry*)daentry->listitem.next;
        if(daentry)
        {
            strcat(prlist,",");
        }
        prlistlen = strlen(prlist);
    }                                  

    /* Allocate the send buffer */
    size += G_SlpdProperty.localeLen + prlistlen;
    result = SLPBufferRealloc(*buffer,size);
    if(result == 0)
    {
        /* out of memory */
        errorcode = SLP_ERROR_INTERNAL_ERROR;
        goto FINISHED;
    }

    /*------------------------------------------------------------*/
    /* Build a buffer containing the fixed portion of the SRVRQST */
    /*------------------------------------------------------------*/
    /*version*/
    *(result->start)       = 2;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_SRVRQST;
    /*length*/
    ToUINT24(result->start + 2, size);
    /*flags*/
    ToUINT16(result->start + 5,  (ismcast ? SLP_FLAG_MCAST : 0));
    /*ext offset*/
    ToUINT24(result->start + 7,0);
    /*xid*/
    ToUINT16(result->start + 10, SLPXidGenerate());  /* TODO: generate a real XID */
    /*lang tag len*/
    ToUINT16(result->start + 12, G_SlpdProperty.localeLen);
    /*lang tag*/
    memcpy(result->start + 14,
           G_SlpdProperty.locale,
           G_SlpdProperty.localeLen);
    result->curpos = result->start + G_SlpdProperty.localeLen + 14;
    /* Prlist */
    ToUINT16(result->curpos,prlistlen);
    result->curpos = result->curpos + 2;
    memcpy(result->curpos,prlist,prlistlen);
    result->curpos = result->curpos + prlistlen;
    /* service type */
    ToUINT16(result->curpos,23);                                         
    result->curpos = result->curpos + 2;
    memcpy(result->curpos,"service:directory-agent",23);
    result->curpos = result->curpos + 23;
    /* scope list zero length */
    ToUINT16(result->curpos,0);
    result->curpos = result->curpos + 2;
    /* predicate zero length */
    ToUINT16(result->curpos,0);
    result->curpos = result->curpos + 2;
    /* spi list zero length */
    ToUINT16(result->curpos,0);
    result->curpos = result->curpos + 2;

    *buffer = result;

    FINISHED:

    if(prlist)
    {
        xfree(prlist);
    }

    return 0;
}



/*=========================================================================*/
int SLPDKnownDAEntryToDAAdvert(int errorcode,
                               unsigned int xid,
                               const SLPDAEntry* daentry,
                               SLPBuffer* sendbuf)
/* Pack a buffer with a DAAdvert using information from a SLPDAentry       */
/*                                                                         */
/* errorcode (IN) the errorcode for the DAAdvert                           */
/*                                                                         */
/* xid (IN) the xid to for the DAAdvert                                    */
/*                                                                         */
/* daentry (IN) pointer to the daentry that contains the rest of the info  */
/*              to make the DAAdvert                                       */
/*                                                                         */
/* sendbuf (OUT) pointer to the SLPBuffer that will be packed with a       */
/*               DAAdvert                                                  */
/*                                                                         */
/* returns: zero on success, non-zero on error                             */
/*=========================================================================*/
{
    int       size;
    SLPBuffer result = *sendbuf;

    /*-------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvrply */
    /*-------------------------------------------------------------*/
    size = daentry->langtaglen + 29; /* 14 bytes for header     */
                                     /*  2 errorcode  */
                                     /*  4 bytes for timestamp */
                                     /*  2 bytes for url len */
                                     /*  2 bytes for scope list len */
                                     /*  2 bytes for attr list len */
                                     /*  2 bytes for spi str len */
                                     /*  1 byte for authblock count */
    size += daentry->urllen;
    size += daentry->scopelistlen;
    size += daentry->attrlistlen;
    size += daentry->spilistlen;

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
    *(result->start + 1)   = SLP_FUNCT_DAADVERT;
    /*length*/
    ToUINT24(result->start + 2, size);
    /*flags*/
    ToUINT16(result->start + 5,
             (size > SLP_MAX_DATAGRAM_SIZE ? SLP_FLAG_OVERFLOW : 0));
    /*ext offset*/
    ToUINT24(result->start + 7,0);
    /*xid*/
    ToUINT16(result->start + 10,xid);
    /*lang tag len*/
    ToUINT16(result->start + 12,daentry->langtaglen);
    /*lang tag*/
    memcpy(result->start + 14,daentry->langtag,daentry->langtaglen);
    result->curpos = result->start + 14 + daentry->langtaglen;

    /*--------------------------*/
    /* Add rest of the DAAdvert */
    /*--------------------------*/
    /* error code */
    ToUINT16(result->curpos,errorcode);
    result->curpos = result->curpos + 2;
    /* timestamp */
    ToUINT32(result->curpos,daentry->bootstamp);
    result->curpos = result->curpos + 4;                       
    /* url len */
    ToUINT16(result->curpos, daentry->urllen);
    result->curpos = result->curpos + 2;
    /* url */
    memcpy(result->curpos,daentry->url,daentry->urllen);
    result->curpos = result->curpos + daentry->urllen;
    /* scope list len */
    ToUINT16(result->curpos, daentry->scopelistlen);
    result->curpos = result->curpos + 2;
    /* scope list */
    memcpy(result->curpos,daentry->scopelist,daentry->scopelistlen);
    result->curpos = result->curpos + daentry->scopelistlen;
    /* attr list len */
    ToUINT16(result->curpos, daentry->attrlistlen);
    result->curpos = result->curpos + 2;
    /* attr list */
    memcpy(result->start,daentry->attrlist,daentry->attrlistlen);
    result->curpos = result->curpos + daentry->attrlistlen;
    /* SPI List */
    ToUINT16(result->curpos,daentry->spilistlen);
    result->curpos = result->curpos + 2;
    memcpy(result->start,daentry->spilist,daentry->spilistlen);
    result->curpos = result->curpos + daentry->spilistlen;
    /* authblock count */
    *(result->curpos) = 0;
    result->curpos = result->curpos + 1;

    FINISHED:
    *sendbuf = result;

    return errorcode;
}


#if defined(ENABLE_SLPv1)
/*=========================================================================*/
int SLPDv1KnownDAEntryToDAAdvert(int errorcode,
                                 int encoding,
                                 unsigned int xid,
                                 const SLPDAEntry* daentry,
                                 SLPBuffer* sendbuf)
/* Pack a buffer with a v1 DAAdvert using information from a SLPDAentry    */
/*                                                                         */
/* errorcode (IN) the errorcode for the DAAdvert                           */
/*                                                                         */
/* encoding (IN) the SLPv1 language encoding for the DAAdvert              */
/*                                                                         */
/* xid (IN) the xid to for the DAAdvert                                    */
/*                                                                         */
/* daentry (IN) pointer to the daentry that contains the rest of the info  */
/*              to make the DAAdvert                                       */
/*                                                                         */
/* sendbuf (OUT) pointer to the SLPBuffer that will be packed with a       */
/*               DAAdvert                                                  */
/*                                                                         */
/* returns: zero on success, non-zero on error                             */
/*=========================================================================*/
{
    int       size = 0;
    int       urllen = INT_MAX;
    int       scopelistlen = INT_MAX;
    SLPBuffer result = *sendbuf;

    /*-------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvrply */
    /*-------------------------------------------------------------*/
    size = 18; /* 12 bytes for header     */
               /*  2 errorcode  */
               /*  2 bytes for url len */
               /*  2 bytes for scope list len */

    if(!errorcode)
    {
        errorcode = SLPv1ToEncoding(0, &urllen, encoding,
                                    daentry->url,
                                    daentry->urllen);
        if(!errorcode)
        {
            size += urllen;
            errorcode = SLPv1ToEncoding(0, &scopelistlen,
                                        encoding,
                                        daentry->scopelist,
                                        daentry->scopelistlen);
            if(!errorcode)
            {
                size += scopelistlen;
            }
        }
    }
    else
    {
        /* don't add these */
        urllen = scopelistlen = 0;
    }

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
    *(result->start)       = 1;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_DAADVERT;
    /*length*/
    ToUINT16(result->start + 2, size);
    /*flags - TODO we have to handle monoling and all that crap */
    ToUINT16(result->start + 4,
             (size > SLP_MAX_DATAGRAM_SIZE ? SLPv1_FLAG_OVERFLOW : 0));
    /*dialect*/
    *(result->start + 5) = 0;
    /*language code*/
    if(daentry->langtag)
    {
        memcpy(result->start + 6, daentry->langtag, 2);
    }
    ToUINT16(result->start + 8, encoding);
    /*xid*/
    ToUINT16(result->start + 10,xid);

    result->curpos = result->start + 12;

    /*--------------------------*/
    /* Add rest of the DAAdvert */
    /*--------------------------*/
    /* error code */
    ToUINT16(result->curpos,errorcode);
    result->curpos = result->curpos + 2;
    ToUINT16(result->curpos, urllen);
    result->curpos = result->curpos + 2;
    /* url */
    SLPv1ToEncoding(result->curpos, &urllen, encoding, 
                    daentry->url,
                    daentry->urllen);
    result->curpos = result->curpos + urllen;
    /* scope list len */
    ToUINT16(result->curpos, daentry->scopelistlen);
    result->curpos = result->curpos + 2;
    /* scope list */
    SLPv1ToEncoding(result->curpos, &scopelistlen,
                    encoding,  
                    daentry->scopelist,
                    daentry->scopelistlen);
    result->curpos = result->curpos + scopelistlen;

    FINISHED:
    *sendbuf = result;

    return errorcode;
}
#endif

/*-------------------------------------------------------------------------*/
void SLPDKnownDARegisterAll(SLPDAEntry* daentry, int immortalonly)
/* registers all services with specified DA                                */
/*-------------------------------------------------------------------------*/
{
    SLPBuffer           buf;
    SLPMessage          msg;
    SLPSrvReg*          srvreg;
    SLPDSocket*         sock;
    SLPBuffer           sendbuf     = 0;
    void*               handle      = 0;

    /*---------------------------------------------------------------*/
    /* Check to see if the database is empty and open an enumeration */
    /* handle if it is not empty                                     */
    /*---------------------------------------------------------------*/
    if(SLPDDatabaseIsEmpty())
    {
        return;
    }
    handle = SLPDDatabaseEnumStart();
    if(handle == 0)
    {
        return;
    }

    /*----------------------------------------------*/
    /* Establish a new connection with the known DA */
    /*----------------------------------------------*/
    sock = SLPDOutgoingConnect(&daentry->daaddr);
    if(sock)
    {
        while(1)
        {
            msg = SLPDDatabaseEnum(handle, &msg, &buf);
            if(msg == NULL) break;
            srvreg = &(msg->body.srvreg);

            /*-----------------------------------------------*/
            /* If so instructed, skip immortal registrations */
            /*-----------------------------------------------*/
            if(!(immortalonly && 
                 srvreg->urlentry.lifetime < SLP_LIFETIME_MAXIMUM))
            {
                /*---------------------------------------------------------*/
                /* Only pass on local (or static) registrations of scopes  */
                /* supported by peer DA                     			   */
                /*---------------------------------------------------------*/
                if((srvreg->source == SLP_REG_SOURCE_LOCAL || 
                   srvreg->source == SLP_REG_SOURCE_STATIC) &&
                   SLPIntersectStringList(srvreg->scopelistlen,
                                          srvreg->scopelist,
                                          srvreg->scopelistlen,
                                          srvreg->scopelist) )
                {
                    sendbuf = SLPBufferDup(buf);
                    if(sendbuf)
                    {
                        /*--------------------------------------------------*/
                        /* link newly constructed buffer to socket sendlist */
                        /*--------------------------------------------------*/
                        SLPListLinkTail(&(sock->sendlist),(SLPListItem*)sendbuf);
                        if(sock->state == STREAM_CONNECT_IDLE)
                        {
                            sock->state = STREAM_WRITE_FIRST;
                        }
                    }
                }
            }
        }
    }

    SLPDDatabaseEnumEnd(handle);
}  


/*-------------------------------------------------------------------------*/
void SLPDKnownDADeregisterAll(SLPDAEntry* daentry)
/* de-registers all services with specified DA                             */
/*-------------------------------------------------------------------------*/
{
    SLPBuffer           buf;
    SLPMessage          msg;
    SLPSrvReg*          srvreg;
    SLPDSocket*         sock;
    size_t              size;
    SLPBuffer           sendbuf     = 0;
    void*               handle      = 0;

    /*---------------------------------------------------------------*/
    /* Check to see if the database is empty and open an enumeration */
    /* handle if it is not empty                                     */
    /*---------------------------------------------------------------*/
    if(SLPDDatabaseIsEmpty())
    {
        return;
    }
    handle = SLPDDatabaseEnumStart();
    if(handle == 0)
    {
        return;
    }

    /* Establish a new connection with the known DA */
    sock = SLPDOutgoingConnect(&daentry->daaddr);
    if(sock)
    {
        while(1)
        {
            msg = SLPDDatabaseEnum(handle, &msg, &buf);
            if(msg == NULL) break;
            srvreg = &(msg->body.srvreg);

            /*-------------------------------------------------*/
            /* Deregister all local (and static) registrations */
            /*-------------------------------------------------*/
            if(srvreg->source == SLP_REG_SOURCE_LOCAL || 
               srvreg->source == SLP_REG_SOURCE_STATIC )
            {
                /*-------------------------------------------------------------*/
                /* ensure the buffer is big enough to handle the whole srvrply */
                /*-------------------------------------------------------------*/
                size = msg->header.langtaglen + 24; /* 14 bytes for header     */
                                                    /*  2 bytes for scopelen */
                                                    /*  6 for static portions of urlentry  */
                                                    /*  2 bytes for taglist len */

                size += srvreg->urlentry.urllen;
                size += srvreg->scopelistlen;
                /* taglistlen is always 0 */

                sendbuf = SLPBufferAlloc(size);
                if(sendbuf)
                {
                    /*----------------------*/
                    /* Construct a SrvDereg */
                    /*----------------------*/
                    /*version*/
                    *(sendbuf->start)       = 2;
                    /*function id*/
                    *(sendbuf->start + 1)   = SLP_FUNCT_SRVDEREG;
                    /*length*/
                    ToUINT24(sendbuf->start + 2, size);
                    /*flags*/
                    ToUINT16(sendbuf->start + 5,
                             (size > SLP_MAX_DATAGRAM_SIZE ? SLP_FLAG_OVERFLOW : 0));
                    /*ext offset*/
                    ToUINT24(sendbuf->start + 7,0);
                    /*xid*/
                    ToUINT16(sendbuf->start + 10,SLPXidGenerate());
                    /*lang tag len*/
                    ToUINT16(sendbuf->start + 12,msg->header.langtaglen);
                    /*lang tag*/
                    memcpy(sendbuf->start + 14,
                           msg->header.langtag,
                           msg->header.langtaglen);
                    sendbuf->curpos = sendbuf->start + 14 + msg->header.langtaglen;

                    /* scope list */
                    ToUINT16(sendbuf->curpos, srvreg->scopelistlen);
                    sendbuf->curpos = sendbuf->curpos + 2;
                    memcpy(sendbuf->curpos,srvreg->scopelist,srvreg->scopelistlen);
                    sendbuf->curpos = sendbuf->curpos + srvreg->scopelistlen;
                    /* the urlentry */
                    if(srvreg->urlentry.opaque)
                    {
                        memcpy(sendbuf->curpos,
                               srvreg->urlentry.opaque,
                               srvreg->urlentry.opaquelen);
                        sendbuf->curpos = sendbuf->curpos + srvreg->urlentry.opaquelen;
                    }
                    else
                    {
                        /* url-entry reserved */
                        *sendbuf->curpos = 0;        
                        sendbuf->curpos = sendbuf->curpos + 1;
                        /* url-entry lifetime */
                        ToUINT16(sendbuf->curpos,srvreg->urlentry.lifetime);
                        sendbuf->curpos = sendbuf->curpos + 2;
                        /* url-entry urllen */
                        ToUINT16(sendbuf->curpos,srvreg->urlentry.urllen);
                        sendbuf->curpos = sendbuf->curpos + 2;
                        /* url-entry url */
                        memcpy(sendbuf->curpos,
                               srvreg->urlentry.url,
                               srvreg->urlentry.urllen);
                        sendbuf->curpos = sendbuf->curpos + srvreg->urlentry.urllen;
                        /* url-entry authcount */
                        *sendbuf->curpos = 0;        
                        sendbuf->curpos = sendbuf->curpos + 1;
                    }
                    /* taglist (always 0) */
                    ToUINT16(sendbuf->curpos,0);
                    
                    /*--------------------------------------------------*/
                    /* link newly constructed buffer to socket sendlist */
                    /*--------------------------------------------------*/
                    SLPListLinkTail(&(sock->sendlist),(SLPListItem*)sendbuf);
                    if(sock->state == STREAM_CONNECT_IDLE)
                    {
                        sock->state = STREAM_WRITE_FIRST;
                    }
                }
            }
        }
    }

    SLPDDatabaseEnumEnd(handle);
}


/*=========================================================================*/
int SLPDKnownDAInit()
/* Initializes the KnownDA list.  Removes all entries and adds entries     */
/* that are statically configured.                                         */
/*                                                                         */
/* returns  zero on success, Non-zero on failure                           */
/*=========================================================================*/
{
    char*               temp;
    char*               tempend;
    char*               slider1;
    char*               slider2;
    struct hostent*     he;
    struct in_addr      daaddr;
    SLPDSocket*         sock;
    SLPBuffer           buf     = 0;

    /*---------------------------------------*/
    /* Skip static DA parsing if we are a DA */
    /*---------------------------------------*/
    if(G_SlpdProperty.isDA)
    {
        /* TODO: some day we may put something here for DA to DA communication */

        /* Perform first passive DAAdvert */
        SLPDKnownDAPassiveDAAdvert(0,0);
        return 0;
    }

    /*------------------------------------------------------*/
    /* Added statically configured DAs to the Known DA List */
    /*------------------------------------------------------*/
    if(G_SlpdProperty.DAAddresses && *G_SlpdProperty.DAAddresses)
    {
        temp = slider1 = slider2 = xstrdup(G_SlpdProperty.DAAddresses);
        if(temp)
        {
            tempend = temp + strlen(temp);
            while(slider1 != tempend)
            {
                while(*slider2 && *slider2 != ',') slider2++;
                *slider2 = 0;

                he = gethostbyname(slider1);
                if(he)
                {
                    daaddr.s_addr = *((unsigned int*)(he->h_addr_list[0]));

                    /*--------------------------------------------------------*/
                    /* Get an outgoing socket to the DA and set it up to make */
                    /* the service:directoryagent request                     */
                    /*--------------------------------------------------------*/
                    sock = SLPDOutgoingConnect(&daaddr);
                    if(sock)
                    {
                        if(MakeActiveDiscoveryRqst(0,&buf) == 0)
                        {
                            if(sock->state == STREAM_CONNECT_IDLE)
                            {
                                sock->state = STREAM_WRITE_FIRST;
                            }
                            SLPListLinkTail(&(sock->sendlist),(SLPListItem*)buf);
                            if(sock->state == STREAM_CONNECT_IDLE)
                            {
                                sock->state = STREAM_WRITE_FIRST;
                            }
                        }
                    }
                }

                slider1 = slider2;
                slider2++;
            }

            xfree(temp);
        }
    }

    /*----------------------------------------*/
    /* Lastly, Perform first active discovery */
    /*----------------------------------------*/
    SLPDKnownDAActiveDiscovery(0);


    return 0;
}


/*=========================================================================*/
int SLPDKnownDADeinit()
/* Deinitializes the KnownDA list.  Removes all entries and deregisters    */
/* all services.                                                           */
/*                                                                         */
/* returns  zero on success, Non-zero on failure                           */
/*=========================================================================*/
{
    SLPDAEntry* entry;
    SLPDAEntry* del;

    entry = (SLPDAEntry*)G_KnownDAList.head;
    while(entry)
    {
        SLPDKnownDADeregisterAll(entry);

        del = entry;

        entry = (SLPDAEntry*)entry->listitem.next;

        SLPDKnownDARemove(del);

        /* broadcast that we are going down */
    }

    return 0;
} 


/*=========================================================================*/
int SLPDKnownDAEnum(void** handle,
                    SLPDAEntry** entry)
/* Enumerate through all entries of the database                           */
/*                                                                         */
/* handle (IN/OUT) pointer to opaque data that is used to maintain         */
/*                 enumerate entries.  Pass in a pointer to NULL to start  */
/*                 enumeration.                                            */
/*                                                                         */
/* entry (OUT) pointer to an entry structure pointer that will point to    */
/*             the next entry on valid return                              */
/*                                                                         */
/* returns: >0 if end of enumeration, 0 on success, <0 on error            */
/*=========================================================================*/
{
    if(*handle == 0)
    {
        *entry = (SLPDAEntry*)G_KnownDAList.head; 
    }
    else
    {
        *entry = (SLPDAEntry*)*handle;
        *entry = (SLPDAEntry*)(*entry)->listitem.next;
    }

    *handle = (void*)*entry;

    if(*handle == 0)
    {
        return 1;
    }

    return 0;
}


/*=========================================================================*/
SLPDAEntry* SLPDKnownDAAdd(struct in_addr* addr,
                           const SLPDAEntry* daentry)
/* Adds a DA to the known DA list if it is new, removes it if DA is going  */
/* down or adjusts entry if DA changed.                                    */
/*                                                                         */
/* addr     (IN) pointer to in_addr of the DA to add                       */
/*                                                                         */
/* pointer (IN) pointer to a daentry to add                                */
/*                                          SLP_LIFETIME_MAXIMUM                               */
/* returns  Pointer to the added or updated entry                          */
/*=========================================================================*/
{
    SLPDAEntry* entry;

    /* Iterate through the list looking for an identical entry */
    entry = (SLPDAEntry*)G_KnownDAList.head;
    while(entry)
    {
        /* for now assume entries are the same if in_addrs match */
        if(memcmp(&entry->daaddr,addr,sizeof(struct in_addr)) == 0)
        {
            /* Check to see if DA is going down */
            if(daentry->bootstamp == 0)
            {
                /* DA is going down. Remove it from our list */
                SLPDKnownDARemove(entry);
            }
            else if(entry->bootstamp >= daentry->bootstamp)
            {
                /* DA went down and came up. Record new entry and */
                /* Re-register all services                       */
                SLPDKnownDARemove(entry);
                entry = 0;
            }
            else
            {
                /* Just adjust the bootstamp */
                entry->bootstamp = daentry->bootstamp;
            }

            break;
        }

        entry = (SLPDAEntry*)entry->listitem.next;
    }

    if(entry == 0)
    {
        /* Create and link in a new entry */
        entry = SLPDAEntryCreate(addr, daentry);
        if(entry)
        {
            SLPListLinkHead(&G_KnownDAList,(SLPListItem*)entry);
            SLPDLogKnownDA("Added",entry);

            /* Register all services with the new DA unless the new DA is us*/
            if(SLPCompareString(entry->urllen,
                                entry->url,
                                G_SlpdProperty.myUrlLen,
                                G_SlpdProperty.myUrl))
            {
                SLPDKnownDARegisterAll(entry,0);
            }
        }
    }

    return entry;
}


/*=========================================================================*/
void SLPDKnownDARemove(SLPDAEntry* daentry)
/* Remove the specified entry from the list of KnownDAs                    */
/*                                                                         */
/* daentry (IN) the entry to remove.                                       */
/*                                                                         */
/* Warning! memory pointed to by daentry will be freed                     */
/*=========================================================================*/
{
    SLPDLogKnownDA("Removed",daentry);
    SLPDAEntryFree((SLPDAEntry*)SLPListUnlink(&G_KnownDAList,
                                              (SLPListItem*)daentry));
}


/*=========================================================================*/
void SLPDKnownDAEcho(struct sockaddr_in* peeraddr,
                     SLPMessage msg,
                     SLPBuffer buf)
/* Echo a srvreg message to a known DA                                     */
/*									                                       */
/* peerinfo (IN) the peer that the registration came from                  */    
/*                                                                         */ 
/* msg (IN) the translated message to echo                                 */
/*                                                                         */
/* buf (IN) the message buffer to echo                                     */
/*                                                                         */
/* Returns:  none                                                          */
/*=========================================================================*/
{
    SLPDAEntry* daentry;
    SLPDSocket* sock;
    const char* msgscope;
    int         msgscopelen;
    SLPBuffer   dup = 0;

    /* Do not echo registrations if we are a DA unless they were made  */
    /* local through the API!                                          */
    if(G_SlpdProperty.isDA && !ISLOCAL(peeraddr->sin_addr))
    {
        return;
    }

    if(msg->header.functionid == SLP_FUNCT_SRVREG)
    {
        msgscope = msg->body.srvreg.scopelist;
        msgscopelen = msg->body.srvreg.scopelistlen;
    }
    else if(msg->header.functionid == SLP_FUNCT_SRVDEREG)
    {
        msgscope = msg->body.srvdereg.scopelist;
        msgscopelen = msg->body.srvdereg.scopelistlen;
    }
    else
    {
        /* We only echo SRVREG and SRVDEREG */
        return;
    }

    daentry = (SLPDAEntry*)G_KnownDAList.head;
    while(daentry)
    {
        /*-----------------------------------------------------*/
        /* Do not echo to agents that do not support the scope */
        /*-----------------------------------------------------*/
        if(SLPIntersectStringList(msgscopelen,
                                  msgscope,
                                  daentry->scopelistlen,
                                  daentry->scopelist))
        {
            /*------------------------------------------*/
            /* Load the socket with the message to send */
            /*------------------------------------------*/
            sock = SLPDOutgoingConnect(&daentry->daaddr);
            if(sock)
            {
                dup = SLPBufferDup(buf);
                if(dup)
                {
                    SLPListLinkTail(&(sock->sendlist),(SLPListItem*)dup);
                    if(sock->state == STREAM_CONNECT_IDLE)
                    {
                        sock->state = STREAM_WRITE_FIRST;
                    }
                }
            }
        }

        daentry = (SLPDAEntry*)daentry->listitem.next;
    }
}


/*=========================================================================*/
void SLPDKnownDAActiveDiscovery(int seconds)
/* Add a socket to the outgoing list to do active DA discovery SrvRqst     */
/*									                                       */
/* Returns:  none                                                          */
/*=========================================================================*/
{
    struct in_addr  peeraddr;
    SLPDSocket*     sock;

    /* Check to see if we should perform active DA detection */
    if(G_SlpdProperty.activeDADetection == 0)
    {
        return;
    }

    if(G_SlpdProperty.nextActiveDiscovery <= 0)
    {
        if(G_SlpdProperty.activeDiscoveryXmits <= 0)
        {
            G_SlpdProperty.nextActiveDiscovery = SLP_CONFIG_DA_BEAT;
            G_SlpdProperty.activeDiscoveryXmits = 3;
        }

        G_SlpdProperty.activeDiscoveryXmits --;

        /*--------------------------------------------------*/
        /* Create new DATAGRAM socket with appropriate peer */
        /*--------------------------------------------------*/
        if(G_SlpdProperty.isBroadcastOnly == 0)
        {
            peeraddr.s_addr = htonl(SLP_MCAST_ADDRESS);
            sock = SLPDSocketCreateDatagram(&peeraddr,DATAGRAM_MULTICAST);
        }
        else
        {
            peeraddr.s_addr = htonl(SLP_BCAST_ADDRESS);
            sock = SLPDSocketCreateDatagram(&peeraddr,DATAGRAM_BROADCAST);
        }

        if(sock)
        {
            /*----------------------------------------------------------*/
            /* Make the srvrqst and add the socket to the outgoing list */
            /*----------------------------------------------------------*/
            MakeActiveDiscoveryRqst(1,&(sock->sendbuf));
            SLPDOutgoingDatagramWrite(sock); 
        }
    }
    else
    {
        G_SlpdProperty.nextActiveDiscovery = G_SlpdProperty.nextActiveDiscovery - seconds;
    }
}


/*=========================================================================*/
void SLPDKnownDAPassiveDAAdvert(int seconds, int dadead)
/* Send passive daadvert messages if properly configured and running as    */
/* a DA                                                                    */
/*	                                                                       */
/* seconds (IN) number seconds that elapsed since the last call to this    */
/*              function                                                   */
/*                                                                         */
/* dadead  (IN) nonzero if the DA is dead and a bootstamp of 0 should be   */
/*              sent                                                       */
/*                                                                         */
/* Returns:  none                                                          */
/*=========================================================================*/
{
    struct in_addr  peeraddr;
    SLPDAEntry      daentry;
    SLPDSocket*     sock;

    /* Check to see if we should perform passive DA detection */
    if(G_SlpdProperty.passiveDADetection == 0)
    {
        return;
    }

    if(G_SlpdProperty.nextPassiveDAAdvert <= 0 || dadead)
    {
        G_SlpdProperty.nextPassiveDAAdvert = SLP_CONFIG_DA_BEAT;

        /*--------------------------------------------------*/
        /* Create new DATAGRAM socket with appropriate peer */
        /*--------------------------------------------------*/
        if(G_SlpdProperty.isBroadcastOnly == 0)
        {
            peeraddr.s_addr = htonl(SLP_MCAST_ADDRESS);
            sock = SLPDSocketCreateDatagram(&peeraddr,DATAGRAM_MULTICAST);
        }
        else
        {
            peeraddr.s_addr = htonl(SLP_BCAST_ADDRESS);
            sock = SLPDSocketCreateDatagram(&peeraddr,DATAGRAM_BROADCAST);
        }

        if(sock)
        {
            /*-----------------------------------------------------------*/
            /* Make the daadvert and add the socket to the outgoing list */
            /*-----------------------------------------------------------*/
            if(dadead)
            {
                daentry.bootstamp = 0;
            }
            else
            {
                G_SlpdProperty.DATimestamp += 1;
                daentry.bootstamp = G_SlpdProperty.DATimestamp;
            }

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

            if(SLPDKnownDAEntryToDAAdvert(0,
                                          0,
                                          &daentry,
                                          &(sock->sendbuf)) == 0)
            {
                SLPDOutgoingDatagramWrite(sock);
            }
            else
            {
                SLPDSocketFree(sock);
            }
#if defined(ENABLE_SLPv1)
            if(!dadead)    /* SLPv1 does not support shutdown messages */
            {
                SLPDSocket *v1sock;
                /*--------------------------------------------------*/
                /* Create new DATAGRAM socket with appropriate peer */
                /*--------------------------------------------------*/
                if(G_SlpdProperty.isBroadcastOnly == 0)
                {
                    peeraddr.s_addr = htonl(SLPv1_DA_MCAST_ADDRESS);
                    v1sock = SLPDSocketCreateDatagram(&peeraddr,
                                                      DATAGRAM_MULTICAST);
                }
                else
                {
                    peeraddr.s_addr = htonl(SLP_BCAST_ADDRESS);
                    v1sock = SLPDSocketCreateDatagram(&peeraddr,
                                                      DATAGRAM_BROADCAST);
                }
                if(v1sock)
                {
                    if(SLPDv1KnownDAEntryToDAAdvert(0,
                                                    SLP_CHAR_UTF8,
                                                    0,
                                                    &daentry,
                                                    &(v1sock->sendbuf)) == 0)
                    {
                        SLPDOutgoingDatagramWrite(v1sock);
                    }
                }
            }
#endif
        }
    }
    else
    {
        G_SlpdProperty.nextPassiveDAAdvert = G_SlpdProperty.nextPassiveDAAdvert - seconds;
    }
}

/*=========================================================================*/
void SLPDKnownDAImmortalRefresh(int seconds)
/* Refresh all SLP_LIFETIME_MAXIMUM services                               */
/* 																		   */
/* seconds (IN) time in seconds since last call                            */
/*=========================================================================*/
{
    SLPDAEntry *entry;

    G_KnownDATimeSinceLastRefresh += seconds;

    if(G_KnownDATimeSinceLastRefresh >= SLP_LIFETIME_MAXIMUM - seconds)
    {
        /* Refresh all SLP_LIFETIME_MAXIMUM registrations */
        entry = (SLPDAEntry*)G_KnownDAList.head;
        while(entry)
        {
            /* Register all services with the all DAs except us*/
            if(SLPCompareString(entry->urllen,
                                entry->url,
                                G_SlpdProperty.myUrlLen,
                                G_SlpdProperty.myUrl))
            {
                SLPDKnownDARegisterAll(entry,1);
            }   

            entry = (SLPDAEntry*)entry->listitem.next;
        }

        G_KnownDATimeSinceLastRefresh = 0;
    }
}
