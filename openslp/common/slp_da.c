/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP                                                    */
/*                                                                         */
/* File:        slp_da.c                                                   */
/*                                                                         */
/* Abstract:    Functions to keep track of DAs                             */
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

#include <slp_da.h>


/*-------------------------------------------------------------------------*/
SLPBuffer BuildDASrvRqst(int mcast,
                         int scopelistlen, 
                         const char* scopelist,
                         int prlistlen,
                         const char* prlist)
/*-------------------------------------------------------------------------*/
{
    SLPBuffer   buf;
    int         size;

    /*--------------------------------------------------------------------*/
    /* allocate a buffer big enough to handle the directory-agent srvrqst */
    /*--------------------------------------------------------------------*/
    size = 49;  /* 16 bytes for header (including langtag) */
                /*  2 bytes for prlistlen  */
                /*  2 bytes for srvtype len field */
                /* 23 bytes for "service:directory-agent" */
                /*  2 bytes for scopelist len field */
                /*  2 bytes for predicate len field */
                /*  2 bytes for spistr len*/ 
    size += scopelistlen + prlistlen;

    buf = SLPBufferAlloc(size);
    if(buf == 0)
    {
        return 0;
    }
    
    /*----------------*/
    /* Add the header */
    /*----------------*/
    /*version*/
    *(buf->start)       = 2;
    /*function id*/
    *(buf->start + 1)   = SLP_FUNCT_SRVRQST;
    /*length*/
    ToUINT24(buf->start + 2, size);
    /*flags*/
    ToUINT16(buf->start + 5, mcast ? SLP_FLAG_MCAST : 0);
    /*ext offset*/
    ToUINT24(buf->start + 7, 0);
    /*xid*/
    ToUINT16(buf->start + 10, SLPXidGenerate());
    /*lang tag len*/
    ToUINT16(buf->start + 12, 2);
    buf->curpos = buf->start + 14;
    /*lang tag*/
    ToUINT16( buf->curpos + 14, 0x656e); /* "en" */
    buf->curpos = buf->curpos + 2 ;

    /*-------------------------*/
    /* Add rest of the SrvRqst */
    /*-------------------------*/
    /* prlist */
    ToUINT16(buf->curpos, prlistlen);
    buf->curpos = buf->curpos + 2;
    memcpy(buf->curpos, prlist, prlistlen);
    buf->curpos = buf->curpos + prlistlen;
    /* service type */
    ToUINT16(buf->curpos, 23);
    buf->curpos = buf->curpos + 2;
    memcpy(buf->curpos, "service:directory-agent", 23);
    buf->curpos = buf->curpos + 23;
    /* scope list */
    ToUINT16(buf->curpos, scopelistlen);
    buf->curpos = buf->curpos + 2;
    memcpy(buf->curpos, scopelist, scopelistlen);
    buf->curpos = buf->curpos + scopelistlen;
    /* predicate */
    ToUINT16(buf->curpos, 0);
    buf->curpos = buf->curpos + 2;
    /* spi list len */
    ToUINT16(buf->curpos,0);
    buf->curpos = buf->curpos + 2;

    return buf;
}

/*-------------------------------------------------------------------------*/
int DiscoverDAsByMulticast(SLPDAEntry** dalisthead,
                           int scopelistlen,
                           const char* scopelist,
                           unsigned long flags,
                           const char* daaddresses,
                           struct timeval* timeout)
/*-------------------------------------------------------------------------*/
{
    return 0;
}


/*-------------------------------------------------------------------------*/
int DiscoverDAsByName(SLPDAEntry** dalisthead,
                      int scopelistlen,
                      const char* scopelist,
                      unsigned long flags,
                      const char* daaddresses,
                      struct timeval* timeout)
/*-------------------------------------------------------------------------*/
{
    struct sockaddr_in  peeraddr;
    SLPBuffer           buf         = 0;
    SLPMessage          msg         = 0;
    SLPDAEntry*         entry       = 0;
    struct hostent*     dahostent   = 0;
    char*               begin       = 0;
    char*               end         = 0;
    int                 sock        = 0;
    int                 finished    = 0;
    int                 result      = 0;

    memset(&peeraddr,0,sizeof(struct sockaddr_in));
    peeraddr.sin_family = AF_INET;
    peeraddr.sin_port = htons(SLP_RESERVED_PORT);

    buf = BuildDASrvRqst(0,scopelistlen,scopelist,0,0);
    if(buf == 0)
    {
        return 0;
    }

    
    /*------------------------------------------------------------------*/
    /* resolve DA host names connect to DA, send SrvRqst, recv DAAdvert */
    /*------------------------------------------------------------------*/
    result = 0;
    begin = (char*)daaddresses;
    end = begin;
    finished = 0;
    while( finished == 0 )
    {
        while(*end && *end != ',') end ++;
        if(*end == 0) finished = 1;
        while(*end <=0x2f) 
        {
            *end = 0;
            end--;
        }
         
        dahostent = gethostbyname(begin);
        if(dahostent)
        {
            peeraddr.sin_addr.s_addr = *(unsigned long*)(dahostent->h_addr_list[0]);
        }
        else
        {
            peeraddr.sin_addr.s_addr = inet_addr(begin);
        }

        sock = SLPNetworkConnectStream(&peeraddr,timeout);
        if(sock >= 0)
        {
            if(SLPNetworkSendMessage(sock, buf, &peeraddr, timeout) == 0)
            {
                if(SLPNetworkRecvMessage(sock, buf, &peeraddr, timeout) == 0)
                {
                    msg = SLPMessageAlloc();
                    if(msg == 0)
                    {
                        return 0;
                    }
                    if(SLPMessageParseBuffer(buf, msg) == 0)
                    {
                        if(msg->body.daadvert.errorcode == 0)
                        {
                            entry = SLPDAEntryCreate(&peeraddr.sin_addr,&(msg->body.daadvert));
                            if(entry)
                            {
                                ListLink((PListItem*)dalisthead,(PListItem)entry);
                                result ++;
                                if(flags & 0x00000001)
                                {
                                    break;
                                }
                            }
                        }
                    }
                    
                    SLPMessageFree(msg);
                }
            }
            close(sock);
        }  
    }

    SLPBufferFree(buf);
    
    return result;
}


/*=========================================================================*/
void SLPDAEntryFree(SLPDAEntry* entry)
/* Frees a SLPDAEntry                                                      */
/*                                                                         */
/* entry    (IN) pointer to entry to free                                  */
/*                                                                         */
/* returns  none                                                           */
/*=========================================================================*/
{
    if(entry->daadvert.url) free((void*)entry->daadvert.url);    
    if(entry->daadvert.scopelist) free((void*)entry->daadvert.scopelist);
    if(entry->daadvert.attrlist) free((void*)entry->daadvert.attrlist);
    if(entry->daadvert.spilist) free((void*)entry->daadvert.spilist);
    free((void*)entry);
}


/*=========================================================================*/
SLPDAEntry* SLPDAEntryCreate(struct in_addr* addr, SLPDAAdvert* daadvert)
/* Creates a SLPDAEntry                                                    */
/*                                                                         */
/* addr     (IN) pointer to in_addr of the DA                              */
/*                                                                         */
/* daadvert (IN) pointer to a daadvert of the DA                           */
/*                                                                         */
/* returns  Pointer to new SLPDAEntry.                                     */
/*=========================================================================*/
{
    SLPDAEntry* result;
     
    result = (SLPDAEntry*)malloc(sizeof(SLPDAEntry));
    if(result == 0)
    {
        return 0;
    }
    
    result->daaddr = *addr;
    memcpy(&(result->daadvert),daadvert,sizeof(SLPDAAdvert));
    result->daadvert.url = memdup(result->daadvert.url,
                                  result->daadvert.urllen);
    result->daadvert.scopelist = memdup(result->daadvert.scopelist,
                                        result->daadvert.scopelistlen);
    result->daadvert.attrlist = memdup(result->daadvert.attrlist,
                                       result->daadvert.attrlistlen);
    result->daadvert.spilist = memdup(result->daadvert.spilist,
                                      result->daadvert.spilistlen);
    result->daadvert.authcount = 0;
    result->daadvert.autharray = 0;
    /* IGNORE DUPLICATION OF AUTHBLOCK BECAUSE IT WON'T BE USED*/

    if(result->daadvert.url ||
       result->daadvert.scopelist ||
       result->daadvert.attrlist ||
       result->daadvert.spilist == 0)
    {
        SLPDAEntryFree(result);
        return 0;
    }
    
    return result;
}


/*=========================================================================*/
int SLPDiscoverDAs(SLPDAEntry** dalisthead,
                   int scopelistlen,
                   const char* scopelist,
                   unsigned long flags,
                   const char* daaddresses,
                   struct timeval* timeout)
/* Discover all known DAs.  Use the net.slp.DAAddresses property, DHCP,    */
/* IPC with slpd, and finally active DA discovery via multicast.  The      */
/* list is NOT emptied before the newly discovered SLPDAEntrys are added.  */
/* Duplicate are discarded.                                                */
/*                                                                         */
/* dalisthead   (IN/OUT) pointer to the head of the list to add to         */
/*                                                                         */
/* scopelistlen (IN) length in bytes of scopelist                          */
/*                                                                         */
/* scopelist    (IN) find DAs of the specified scope.  NULL means find     */
/*                   DAs in all scopes                                     */
/*                                                                         */
/* flags        (IN) Discovery flags:                                      */
/*                   0x000000001 = First DA sufficient                     */
/*                   0x000000002 = First discovery method sufficient       */
/*                                                                         */
/* daaddresses  (IN) value of net.slp.DAAddresses                          */
/*                                                                         */
/* timeout      (IN) Maximum time to wait for discovery                    */
/*                                                                         */
/* Returns:     number of DAs found. -1 on error.                          */
/*                                                                         */
/* WARNING:     This function may block for a *long* time                  */
/*=========================================================================*/
{
    int                 result  = 0;

    /*-------------------*/
    /* Check DAAddresses */
    /*-------------------*/
    result += DiscoverDAsByName(dalisthead,
                                scopelistlen,
                                scopelist,
                                flags,
                                daaddresses,
                                timeout); 
    if(result && flags & 0x00000002)
    {
        return result;
    }

    /*------------*/
    /* Check DHCP */
    /*------------*/
    /* TODO: put code to get DHCP options here when available */
    if(result && flags & 0x00000002)
    {
        return result;
    }


    /*---------------*/
    /* Use Multicast */
    /*---------------*/
    result += DiscoverDAsByMulticast(dalisthead,
                                     scopelistlen,
                                     scopelist,
                                     flags,
                                     daaddresses,
                                     timeout);
        
    return result;
}
