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

#include "slpd.h"

/*=========================================================================*/
SLPList G_KnownDAList = {0,0,0};                                         
/* The list of DAs known to slpd.                                          */
/*=========================================================================*/

/*-------------------------------------------------------------------------*/
int MakeActiveDiscoveryRqst(int ismcast, SLPBuffer* buffer)
/* Pack a buffer with service:directory-agent SrvRqst                      */
/*-------------------------------------------------------------------------*/
{
    size_t          size;
    SLPDAEntry*     daentry;
    SLPBuffer       result;
    char*           prlist     = 0;
    size_t          prlistlen  = 0;
    int             errorcode = 0;

    /*-------------------------------------------------*/
    /* Generate a DA service request buffer to be sent */
    /*-------------------------------------------------*/
    /* determine the size of the fixed portion of the SRVRQST         */
    size  = 47;  /* 14 bytes for the header                        */
                 /*  2 bytes for the prlistlen                     */
                 /*  2 bytes for the srvtype length                */ 
                 /* 23 bytes for "service:directory-agent" srvtype */
                 /*  2 bytes for scopelistlen                      */
                 /*  2 bytes for predicatelen                      */
                 /*  2 bytes for sprstrlen                         */
    
    /* figure out what our Prlist will be by going through our list of  */
    /* known DAs                                                        */
    prlistlen = 0;
    prlist = malloc(SLP_MAX_DATAGRAM_SIZE);
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
    ToUINT16(result->start + 5,  ismcast ? SLP_FLAG_MCAST : 0);
    /*ext offset*/
    ToUINT24(result->start + 7,0);
    /*xid*/
    ToUINT16(result->start + 10, 0);  /* TODO: generate a real XID */
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
        free(prlist);
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
             size > SLP_MAX_DATAGRAM_SIZE ? SLP_FLAG_OVERFLOW : 0);
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


/*-------------------------------------------------------------------------*/
void SLPDKnownDARegisterAll(SLPDAEntry* daentry)
/* Forks a child process to register all services with specified DA        */
/*-------------------------------------------------------------------------*/
{
    SLPDDatabaseEntry*  dbentry;
    SLPDSocket*         sock;
    SLPBuffer           buf;
    size_t              size;
    void*               handle      = 0;
    
    /* Establish a new connection with the known DA */
    sock = SLPDOutgoingConnect(&daentry->daaddr);
    if(sock)
    {
        while( SLPDDatabaseEnum(&handle,&dbentry) == 0)
        {
            if(SLPIntersectStringList(daentry->scopelistlen,
                                      daentry->scopelist,
                                      dbentry->scopelistlen,
                                      dbentry->scopelist) )
            {
                /*-------------------------------------------------------------*/
                /* ensure the buffer is big enough to handle the whole srvrply */
                /*-------------------------------------------------------------*/
                size = dbentry->langtaglen + 27; /* 14 bytes for header     */
                                                 /*  6 for static portions of urlentry  */
                                                 /*  2 bytes for srvtypelen */
                                                 /*  2 bytes for scopelen */
                                                 /*  2 bytes for attr list len */
                                                 /*  1 byte for authblock count */
                size += dbentry->urllen;
                size += dbentry->srvtypelen;
                size += dbentry->scopelistlen;
                size += dbentry->attrlistlen;
                
                /* TODO: room for authstuff */
                
                buf = SLPBufferAlloc(size);
                if(buf)
                {                               
                    /*--------------------*/
                    /* Construct a SrvReg */
                    /*--------------------*/
                    /*version*/
                    *(buf->start)       = 2;
                    /*function id*/
                    *(buf->start + 1)   = SLP_FUNCT_SRVREG;
                    /*length*/
                    ToUINT24(buf->start + 2, size);
                    /*flags*/
                    ToUINT16(buf->start + 5,
                             size > SLP_MAX_DATAGRAM_SIZE ? SLP_FLAG_OVERFLOW : 0);
                    /*ext offset*/
                    ToUINT24(buf->start + 7,0);
                    /*xid*/
                    ToUINT16(buf->start + 10,rand());
                    /*lang tag len*/
                    ToUINT16(buf->start + 12,dbentry->langtaglen);
                    /*lang tag*/
                    memcpy(buf->start + 14,
                           dbentry->langtag,
                           dbentry->langtaglen);
                    buf->curpos = buf->start + 14 + dbentry->langtaglen;
                
                    /* url-entry reserved */
                    *buf->curpos = 0;        
                    buf->curpos = buf->curpos + 1;
                    /* url-entry lifetime */
                    ToUINT16(buf->curpos,dbentry->lifetime);
                    buf->curpos = buf->curpos + 2;
                    /* url-entry urllen */
                    ToUINT16(buf->curpos,dbentry->urllen);
                    buf->curpos = buf->curpos + 2;
                    /* url-entry url */
                    memcpy(buf->curpos,dbentry->url,dbentry->urllen);
                    buf->curpos = buf->curpos + dbentry->urllen;
                    /* url-entry authcount */
                    *buf->curpos = 0;        
                    buf->curpos = buf->curpos + 1;
                    /* srvtype */
                    ToUINT16(buf->curpos,dbentry->srvtypelen);
                    buf->curpos = buf->curpos + 2;
                    memcpy(buf->curpos,dbentry->srvtype,dbentry->srvtypelen);
                    buf->curpos = buf->curpos + dbentry->srvtypelen;
                    /* scope list */
                    ToUINT16(buf->curpos, dbentry->scopelistlen);
                    buf->curpos = buf->curpos + 2;
                    memcpy(buf->curpos,dbentry->scopelist,dbentry->scopelistlen);
                    buf->curpos = buf->curpos + dbentry->scopelistlen;
                    /* attr list */
                    ToUINT16(buf->curpos, dbentry->attrlistlen);
                    buf->curpos = buf->curpos + 2;
                    memcpy(buf->curpos,dbentry->attrlist,dbentry->attrlistlen);
                    buf->curpos = buf->curpos + dbentry->attrlistlen;;
                    /* authblock count */
                    *(buf->curpos) = 0;
                    buf->curpos = buf->curpos + 1;
    
                    /*--------------------------------------------------*/
                    /* link newly constructed buffer to socket sendlist */
                    /*--------------------------------------------------*/
                    SLPListLinkTail(&(sock->sendlist),(SLPListItem*)buf);
                    if (sock->state == STREAM_CONNECT_IDLE)
                    {
                        sock->state = STREAM_WRITE_FIRST;
                    }
                }
            }
        }
    }
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
    SLPBuffer           buf;
    SLPDSocket*         sock;

    /*---------------------------------------*/
    /* Skip static DA parsing if we are a DA */
    /*---------------------------------------*/
    if(G_SlpdProperty.isDA)
    {
        /* TODO: some day we may put something here for DA to DA communication */
        
        /* Perform first passive DAAdvert */
        SLPDKnownDAPassiveDAAdvert(0);
        return 0;
    }
    
    /*------------------------------------------------------*/
    /* Added statically configured DAs to the Known DA List */
    /*------------------------------------------------------*/
    if (G_SlpdProperty.DAAddresses && *G_SlpdProperty.DAAddresses)
    {
        temp = slider1 = slider2 = strdup(G_SlpdProperty.DAAddresses);
        if(temp)
        {
            tempend = temp + strlen(temp);
            while (slider1 != tempend)
            {
                while (*slider2 && *slider2 != ',') slider2++;
                *slider2 = 0;
    
                he = gethostbyname(slider1);
                if (he)
                {
                    daaddr.s_addr = *((unsigned long*)(he->h_addr_list[0]));
                    
                    /*--------------------------------------------------------*/
                    /* Get an outgoing socket to the DA and set it up to make */
                    /* the service:directoryagent request                     */
                    /*--------------------------------------------------------*/
                    sock = SLPDOutgoingConnect(&daaddr);
                    if(sock)
                    {
                        if(MakeActiveDiscoveryRqst(0,&buf) == 0)
                        {
                            if (sock->state == STREAM_CONNECT_IDLE)
                            {
                                sock->state = STREAM_WRITE_FIRST;
                            }
                            SLPListLinkTail(&(sock->sendlist),(SLPListItem*)buf);
                        }
                    }
                }
    
                slider1 = slider2;
                slider2++;
            }
    
            free(temp);
        }
    }
    
    /*----------------------------------------*/
    /* Lastly, Perform first active discovery */
    /*----------------------------------------*/
    SLPDKnownDAActiveDiscovery(0);
    
    
    return 0;
}


/*=========================================================================*/
SLPDAEntry* SLPDKnownDAFindRandomEntry(int scopelistlen,
                                       const char* scopelist)
/* Find a known DA that supports the specified scope list                  */
/*=========================================================================*/
{
    SLPDAEntry* entry;
    SLPDAEntry* lastentry;

    lastentry = 0;
    entry = (SLPDAEntry*)G_KnownDAList.head;
    while (entry)
    {
        /* check to see if a common scope is supported */
        if (SLPIntersectStringList(entry->scopelistlen,
                                   entry->scopelist,
                                   scopelistlen,
                                   scopelist))
        {
            lastentry = entry;
            if(rand() % 2)
            {
                break;
            }
        }

        entry = (SLPDAEntry*)entry->listitem.next;
    }

    if(entry == 0)
    {
        entry = lastentry;
    }

    return entry;
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
/*                                                                         */
/* returns  Pointer to the added or updated entry                          */
/*=========================================================================*/
{
    SLPDAEntry* entry;

    /* Iterate through the list looking for an identical entry */
    entry = (SLPDAEntry*)G_KnownDAList.head;
    while (entry)
    {
        /* for now assume entries are the same if in_addrs match */
        if (memcmp(&entry->daaddr,addr,sizeof(struct in_addr)) == 0)
        {
            /* Found an existing entry */
            if(entry->bootstamp == 0)
            {
                /* DA is going down. Remove it from our list */
                SLPDKnownDARemove(entry);
            }
            else if(entry->bootstamp > daentry->bootstamp)
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

            /* Register all services with the new DA */
            SLPDKnownDARegisterAll(entry);
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
    SLPBuffer   dup;
    const char* msgscope;
    int         msgscopelen;
    
    /* TODO: make sure that we do not echo to ourselves */
    
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
    while (daentry)
    {
        /*-----------------------------------------------------*/
        /* Do not echo to agents that do not support the scope */
        /*-----------------------------------------------------*/
        if(SLPIntersectStringList(daentry->scopelistlen,
                                  daentry->scopelist,
                                  msgscopelen,
                                  msgscope) )
        {
            /*------------------------------------------*/
            /* Load the socket with the message to send */
            /*------------------------------------------*/
            sock = SLPDOutgoingConnect(&daentry->daaddr);
            if(sock)
            {
                dup = SLPBufferDup(buf);
                if (dup)
                {
                    SLPListLinkTail(&(sock->sendlist),(SLPListItem*)dup);
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
void SLPDKnownDAPassiveDAAdvert(int seconds)
/* Send passive daadvert messages if properly configured and running as    */
/* a DA                                                                    */
/*	                                                                       */
/* seconds (IN) number seconds that elapsed since the last call to this    */
/*              function                                                   */
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

    if(G_SlpdProperty.nextPassiveDAAdvert <= 0)
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
            
            if(SLPDKnownDAEntryToDAAdvert(0,
                                          0,
                                          &daentry,
                                          &(sock->sendbuf)) == 0)
            {
                SLPDOutgoingDatagramWrite(sock);
            }
        }
    }
    else
    {
        G_SlpdProperty.nextPassiveDAAdvert = G_SlpdProperty.nextPassiveDAAdvert - seconds;
    }
}

