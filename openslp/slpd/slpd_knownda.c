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
#ifdef ENABLE_SLPv2_SECURITY
    #include "slpd_spi.h"
#endif

/*=========================================================================*/
/* common code includes                                                    */
/*=========================================================================*/
#include "slp_xmalloc.h"
#include "slp_v1message.h"
#include "slp_utf8.h"
#include "slp_compare.h"
#include "slp_xid.h"
#include "slp_dhcp.h"
#include "slp_parse.h"
#include "slp_net.h"
#ifdef ENABLE_SLPv2_SECURITY
    #include "slp_auth.h"
    #include "slp_spi.h"
#endif

#include <limits.h>

/*=========================================================================*/
SLPDatabase G_SlpdKnownDAs;
/* The database of DAAdverts from DAs known to slpd.                       */
/*=========================================================================*/


/*=========================================================================*/
int G_KnownDATimeSinceLastRefresh = 0;
/*=========================================================================*/

/*-------------------------------------------------------------------------*/
int MakeActiveDiscoveryRqst(int ismcast, SLPBuffer* buffer)
/* Pack a buffer with service:directory-agent SrvRqst                      *
 *                                                                         *
 * Caller must free buffer                                                 *
 *-------------------------------------------------------------------------*/
{
    size_t          size;
    void*           eh;
    SLPMessage      msg;
    char*           prlist      = 0;
    size_t          prlistlen   = 0;
    int             errorcode   = 0;
    SLPBuffer       tmp         = 0;
    SLPBuffer       result      = *buffer;

    /*-------------------------------------------------*/
    /* Generate a DA service request buffer to be sent */
    /*-------------------------------------------------*/
    /* determine the size of the fixed portion of the SRVRQST     */
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
    if ( prlist == 0 )
    {
        /* out of memory */
        errorcode = SLP_ERROR_INTERNAL_ERROR;
        goto FINISHED;
    }

    *prlist = 0;
    /* Don't send active discoveries to DAs we already know about */
    eh = SLPDKnownDAEnumStart();
    if ( eh )
    {
        while ( 1 )
        {
            if ( SLPDKnownDAEnum(eh, &msg, &tmp) == 0 )
            {
                break;
            }
            strcat(prlist,inet_ntoa(msg->peer.sin_addr));
            strcat(prlist,",");
            prlistlen = strlen(prlist);
        }

        SLPDKnownDAEnumEnd(eh);
    }

    /* Allocate the send buffer */
    size += G_SlpdProperty.localeLen + prlistlen;
    result = SLPBufferRealloc(result, size);
    if ( result == 0 )
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
    /* 23 is the length of SLP_DA_SERVICE_TYPE */
    memcpy(result->curpos,SLP_DA_SERVICE_TYPE,23);
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

    if ( prlist )
    {
        xfree(prlist);
    }

    return 0;
}


/*-------------------------------------------------------------------------*/
void SLPDKnownDARegisterAll(SLPMessage daadvert, int immortalonly)
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
    if ( SLPDDatabaseIsEmpty() )
    {
        return;
    }

    /*--------------------------------------*/
    /* Never do a Register All to ourselves */
    /*--------------------------------------*/
    if ( SLPCompareString(G_SlpdProperty.myUrlLen,
                          G_SlpdProperty.myUrl,
                          daadvert->body.daadvert.urllen,
                          daadvert->body.daadvert.url) == 0 )
    {
        return;
    }

    handle = SLPDDatabaseEnumStart();
    if ( handle == 0 )
    {
        return;
    }

    /*----------------------------------------------*/
    /* Establish a new connection with the known DA */
    /*----------------------------------------------*/
    sock = SLPDOutgoingConnect(&(daadvert->peer.sin_addr));
    if ( sock )
    {
        while ( 1 )
        {
            msg = SLPDDatabaseEnum(handle, &msg, &buf);
            if ( msg == NULL ) break;
            srvreg = &(msg->body.srvreg);

            /*-----------------------------------------------*/
            /* If so instructed, skip mortal registrations   */
            /*-----------------------------------------------*/
            if ( immortalonly && 
                 srvreg->urlentry.lifetime < SLP_LIFETIME_MAXIMUM ) 
            {
                continue;
            }
                
            /*---------------------------------------------------------*/
            /* Only register local (or static) registrations of scopes */
            /* supported by peer DA                                    */
            /*---------------------------------------------------------*/
            if ( ( srvreg->source == SLP_REG_SOURCE_LOCAL || 
                   srvreg->source == SLP_REG_SOURCE_STATIC ) &&
                 SLPIntersectStringList(srvreg->scopelistlen,
                                        srvreg->scopelist,
                                        srvreg->scopelistlen,
                                        srvreg->scopelist) )
            {
                sendbuf = SLPBufferDup(buf);
                if ( sendbuf )
                {
                    /*--------------------------------------------------*/
                    /* link newly constructed buffer to socket sendlist */
                    /*--------------------------------------------------*/
                    SLPListLinkTail(&(sock->sendlist),(SLPListItem*)sendbuf);
                    if ( sock->state == STREAM_CONNECT_IDLE )
                    {
                        sock->state = STREAM_WRITE_FIRST;
                    }
                }
            }
        }
    }

    SLPDDatabaseEnumEnd(handle);
}  


/*-------------------------------------------------------------------------*/
int MakeSrvderegFromSrvReg(SLPMessage msg, 
                           SLPBuffer inbuf,
                           SLPBuffer* outbuf)
/* Pack a buffer with a SrvDereg message using information from an existing
 * SrvReg message
 * 
 * Caller must free outbuf
 *-------------------------------------------------------------------------*/
{
    int                 size;
    SLPBuffer           sendbuf;
    SLPSrvReg*          srvreg;
    
    srvreg = &(msg->body.srvreg);

    /*-------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvdereg*/
    /*-------------------------------------------------------------*/
    size = msg->header.langtaglen + 18; /* 14 bytes for header     */
                                           /*  2 bytes for scopelen */
                                           /*  see below for URLEntry */
                                           /*  2 bytes for taglist len */
    if ( srvreg->urlentry.opaque )
    {
        size += srvreg->urlentry.opaquelen;
    }
    else
    {
        size += 6; /* +6 for the static portion of the url-entry */
        size += srvreg->urlentry.urllen;
    }
    size += srvreg->scopelistlen;
    /* taglistlen is always 0 */

    *outbuf = sendbuf = SLPBufferAlloc(size);
    if (*outbuf == NULL)
    {
        return SLP_ERROR_INTERNAL_ERROR;
    }

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
#ifdef ENABLE_SLPv1
    if ( srvreg->urlentry.opaque == 0 )
    {
        /* url-entry reserved */
        *sendbuf->curpos = 0;        
        sendbuf->curpos += 1;
        /* url-entry lifetime */
        ToUINT16(sendbuf->curpos,srvreg->urlentry.lifetime);
        sendbuf->curpos = sendbuf->curpos + 2;
        /* url-entry urllen */
        ToUINT16(sendbuf->curpos,srvreg->urlentry.urllen);
        sendbuf->curpos += 2;
        /* url-entry url */
        memcpy(sendbuf->curpos,
               srvreg->urlentry.url,
               srvreg->urlentry.urllen);
        sendbuf->curpos += srvreg->urlentry.urllen;                        
        /* url-entry authcount */
        *sendbuf->curpos = 0;        
        sendbuf->curpos += 1;
    }
    else
#endif /* ENABLE_SLPv1 */
    {
        memcpy(sendbuf->curpos,
               srvreg->urlentry.opaque,
               srvreg->urlentry.opaquelen);
        sendbuf->curpos += srvreg->urlentry.opaquelen;
    }

    /* taglist (always 0) */
    ToUINT16(sendbuf->curpos,0);
    sendbuf->curpos += 1;

    return 0;
}


/*-------------------------------------------------------------------------*/
void SLPDKnownDADeregisterAll(SLPMessage daadvert)
/* de-registers all services with specified DA                             */
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
    if ( SLPDDatabaseIsEmpty() )
    {
        return;
    }
    handle = SLPDDatabaseEnumStart();
    if ( handle == 0 )
    {
        return;
    }

    /* Establish a new connection with the known DA */
    sock = SLPDOutgoingConnect(&(daadvert->peer.sin_addr));
    if ( sock )
    {
        while ( 1 )
        {
            msg = SLPDDatabaseEnum(handle, &msg, &buf);
            if ( msg == NULL ) break;
            srvreg = &(msg->body.srvreg);

            /*-------------------------------------------------*/
            /* Deregister all local (and static) registrations */
            /*-------------------------------------------------*/
            if ( srvreg->source == SLP_REG_SOURCE_LOCAL || 
                 srvreg->source == SLP_REG_SOURCE_STATIC )
            {
                if(MakeSrvderegFromSrvReg(msg,buf,&sendbuf) == 0)
                {
                    /*--------------------------------------------------*/
                    /* link newly constructed buffer to socket sendlist */
                    /*--------------------------------------------------*/
                    SLPListLinkTail(&(sock->sendlist),(SLPListItem*)sendbuf);
                    if ( sock->state == STREAM_CONNECT_IDLE )
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
int SLPDKnownDAFromDHCP()
/* Queries DHCP for configured DA's.													*/
/*                                                                         */
/* returns  zero on success, Non-zero on failure                           */
/*=========================================================================*/
{
	SLPBuffer			buf;
	DHCPContext			ctx;
	SLPDSocket*			sock;
	struct in_addr		daaddr;
	unsigned char *	alp;
	unsigned char		dhcpOpts[] = {TAG_SLP_SCOPE, TAG_SLP_DA};

	*ctx.scopelist = 0;
	ctx.addrlistlen = 0;

	DHCPGetOptionInfo(dhcpOpts, sizeof(dhcpOpts), DHCPParseSLPTags, &ctx);

	alp = ctx.addrlist;
	while(ctx.addrlistlen >= 4)
	{
		memcpy(&daaddr.s_addr, alp, 4);
		if (daaddr.s_addr)
		{
			/*--------------------------------------------------------
				Get an outgoing socket to the DA and set it up to make
				the service:directoryagent request
			  --------------------------------------------------------*/
			sock = SLPDOutgoingConnect(&daaddr);
			if (sock)
			{
			   buf = 0;
			   if (MakeActiveDiscoveryRqst(0,&buf) == 0)
			   {
					if (sock->state == STREAM_CONNECT_IDLE)
						sock->state = STREAM_WRITE_FIRST;
					SLPListLinkTail(&(sock->sendlist),(SLPListItem*)buf);
					if (sock->state == STREAM_CONNECT_IDLE)
						sock->state = STREAM_WRITE_FIRST;
			   }
			}
		}
		ctx.addrlistlen -= 4;
		alp += 4;
	}
	return 0;
}

/*=========================================================================*/
int SLPKnownDAFromProperties()
/* Queries static configuration for DA's.												*/
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
	SLPBuffer           buf;

	if (G_SlpdProperty.DAAddresses && *G_SlpdProperty.DAAddresses)
	{
		temp = slider1 = xstrdup(G_SlpdProperty.DAAddresses);
		if (temp)
		{
			tempend = temp + strlen(temp);
			while (slider1 < tempend)
			{
				while (*slider1 && *slider1 == ' ') slider1++;
				slider2 = slider1;
				while (*slider2 && *slider2 != ',') slider2++;
				*slider2++ = 0;

				daaddr.s_addr = 0;
				if(inet_aton(slider1, &daaddr) == 0)
				{
					he = gethostbyname(slider1);
					if (he)
						daaddr.s_addr = *((unsigned int*)(he->h_addr_list[0]));
				}
			    
				if(daaddr.s_addr)
				{
					/*--------------------------------------------------------*/
					/* Get an outgoing socket to the DA and set it up to make */
					/* the service:directoryagent request                     */
					/*--------------------------------------------------------*/
					sock = SLPDOutgoingConnect(&daaddr);
					if (sock)
					{
						buf = 0;
						if (MakeActiveDiscoveryRqst(0,&buf) == 0)
						{
							if (sock->state == STREAM_CONNECT_IDLE)
								sock->state = STREAM_WRITE_FIRST;
							SLPListLinkTail(&(sock->sendlist),(SLPListItem*)buf);
							if (sock->state == STREAM_CONNECT_IDLE)
								sock->state = STREAM_WRITE_FIRST;
						}
					}
				}
				slider1 = slider2;
			}
			xfree(temp);
		}
	}
	return 0;
}

/*=========================================================================*/
int SLPDKnownDAInit()
/* Initializes the KnownDA list.  Removes all entries and adds entries     */
/* that are statically configured.  Adds entries configured through DHCP.  */
/*                                                                         */
/* returns  zero on success, Non-zero on failure                           */
/*=========================================================================*/
{
    /*--------------------------------------*/
    /* Set initialize the DAAdvert database */
    /*--------------------------------------*/
    SLPDatabaseInit(&G_SlpdKnownDAs);

    /*-----------------------------------------------------------------*/
    /* Added statically configured DAs to the Known DA List by sending */
    /* active DA discovery requests directly to them                   */
    /*-----------------------------------------------------------------*/
    SLPKnownDAFromProperties();

    /*-----------------------------------------------------------------*/
    /* Discover DHCP DA's and add them to the active discovery list.	  */
    /*-----------------------------------------------------------------*/
    SLPDKnownDAFromDHCP();

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
    SLPDatabaseHandle   dh;
    SLPDatabaseEntry*   entry;
    dh = SLPDatabaseOpen(&G_SlpdKnownDAs);
    if ( dh )
    {
        /*------------------------------------*/
        /* Unregister all local registrations */
        /*------------------------------------*/
        while ( 1 )
        {
            entry = SLPDatabaseEnum(dh);
            if ( entry == NULL ) break;

            SLPDKnownDADeregisterAll(entry->msg);
        }
        SLPDatabaseClose(dh);
    }

    SLPDatabaseDeinit(&G_SlpdKnownDAs);

    return 0;
} 


/*=========================================================================*/
int SLPDKnownDAAdd(SLPMessage msg, SLPBuffer buf)
/* Adds a DA to the known DA list if it is new, removes it if DA is going  */
/* down or adjusts entry if DA changed.                                    */
/*                                                                         */
/* msg     (IN) DAAdvert Message descriptor                                */
/*                                                                         */
/* buf     (IN) The DAAdvert message buffer                                */
/*                                                                         */
/* returns  Zero on success, Non-zero on error                             */
/*=========================================================================*/
{
    SLPDatabaseEntry*   entry;
    SLPDAAdvert*        entrydaadvert;
    SLPDAAdvert*        daadvert;
    struct in_addr      daaddr;
    SLPParsedSrvUrl*    parsedurl       = NULL;
    int                 result          = 0;
    SLPDatabaseHandle   dh              = NULL;

    dh = SLPDatabaseOpen(&G_SlpdKnownDAs);
    if ( dh == NULL )
    {
        result = SLP_ERROR_INTERNAL_ERROR;
        goto CLEANUP;
    }

    /* daadvert is the DAAdvert message being added */
    daadvert = &(msg->body.daadvert);

    
    /* --------------------------------------------------------
     * Make sure that the peer address in the DAAdvert matches
     * the host in the DA service URL.
     *---------------------------------------------------------
     */
    if (SLPParseSrvUrl(daadvert->urllen,
                       daadvert->url,
                       &parsedurl))
    {
        /* could not parse the DA service url */
        result = SLP_ERROR_PARSE_ERROR;
        goto CLEANUP;
    }
    if (SLPNetResolveHostToAddr(parsedurl->host,&daaddr))
    {
        /* Unable to resolve the host in the DA advert to an address */
        xfree(parsedurl);
        result = SLP_ERROR_PARSE_ERROR;
        goto CLEANUP;
    }
    /* free the parsed url created in call to SLPParseSrvUrl() */
    xfree(parsedurl);
    /* set the peer address in the DAAdvert message so that it matches
     * the address the DA service URL resolves to 
     */
    msg->peer.sin_addr = daaddr;


    /*-----------------------------------------------------*/
    /* Check to see if there is already an identical entry */
    /*-----------------------------------------------------*/
    while ( 1 )
    {
        entry = SLPDatabaseEnum(dh);
        if ( entry == NULL ) break;

        /* entrydaadvert is the DAAdvert message from the database */
        entrydaadvert = &(entry->msg->body.daadvert);

        /* Assume DAs are identical if their URLs match */
        if ( SLPCompareString(entrydaadvert->urllen,
                              entrydaadvert->url,
                              daadvert->urllen,
                              daadvert->url) == 0 )
        {

#ifdef ENABLE_SLPv2_SECURITY                
            if ( G_SlpdProperty.checkSourceAddr &&
                 memcmp(&(entry->msg->peer.sin_addr),
                        &(msg->peer.sin_addr),
                        sizeof(struct in_addr)) )
            {
                SLPDatabaseClose(dh);
                result = SLP_ERROR_AUTHENTICATION_FAILED;
                goto CLEANUP;
            }

            /* make sure an unauthenticated DAAdvert can't replace */
            /* an authenticated one                                */
            if ( entrydaadvert->authcount &&
                 entrydaadvert->authcount != daadvert->authcount )
            {
                SLPDatabaseClose(dh);
                result = SLP_ERROR_AUTHENTICATION_FAILED;
                goto CLEANUP;
            } 
            
#endif
            
            if ( daadvert->bootstamp != 0 &&
                 daadvert->bootstamp <= entrydaadvert->bootstamp )
            {
                /* Advertising DA must have went down then came back up */
                SLPDKnownDARegisterAll(msg,0);
            }

            /* Remove the entry that is the same as the advertised entry */
            /* so that we can put the new advertised entry back in       */
            SLPDatabaseRemove(dh,entry);
            break;
        }
    }

    /* Make sure the DA is not dying */
    if (daadvert->bootstamp != 0)
    {
        if ( entry == 0 )
        {
            /* create a new database entry using the DAAdvert message */
            entry = SLPDatabaseEntryCreate(msg,buf);
            if (entry)
            {
                SLPDatabaseAdd(dh, entry);

                /* register all the services we know about with this new DA */
                SLPDKnownDARegisterAll(msg,0);

                /* log the addition of a new DA */
                SLPDLogDAAdvertisement("Addition",entry);
            }
            else
            {
                /* Could not create a new entry */
                result = SLP_ERROR_INTERNAL_ERROR;
                goto CLEANUP;
            }
        }
        else
        {
            /* The advertising DA is not new to us, but the old entry   */
            /* has been deleted from our database so that the new entry */
            /* with its up to date time stamp can be put back in.       */
            /* create a new database entry using the DAAdvert message */
            entry = SLPDatabaseEntryCreate(msg,buf);
            if (entry)
            {
                SLPDatabaseAdd(dh, entry);
            }
            else
            {
                /* Could not create a new entry */
                result = SLP_ERROR_INTERNAL_ERROR;
                goto CLEANUP;
            }
        }

        SLPDatabaseClose(dh);

        return result;
    }
    else
    {
        /* DA is dying */
        if (entry)
        {
            /* Dying DA was found in our KnownDA database. Log that it
             * was removed.
             */
            SLPDLogDAAdvertisement("Removed",entry);
        }
    }

    CLEANUP:
    /* If we are here, we need to cleanup the message descriptor and the  */
    /* message buffer because they were not added to the database and not */
    /* cleaning them up would result in a memory leak                     */
    /* We also need to make sure the Database handle is closed.           */
    SLPMessageFree(msg);
    SLPBufferFree(buf);
    if (dh) SLPDatabaseClose(dh);

    return result;
}

/*=========================================================================*/
void SLPDKnownDARemove(struct in_addr* addr)
/* Removes known DAs that sent DAAdverts from the specified in_addr        */
/*=========================================================================*/
{
    SLPDatabaseHandle   dh;
    SLPDatabaseEntry*   entry;

    dh = SLPDatabaseOpen(&G_SlpdKnownDAs);
    if ( dh )
    {
        /*-----------------------------------------------------*/
        /* Check to see if there is already an identical entry */
        /*-----------------------------------------------------*/
        while ( 1 )
        {
            entry = SLPDatabaseEnum(dh);
            if ( entry == NULL ) break;

            /* Assume DAs are identical if their peer match */
            if ( memcmp(addr,&(entry->msg->peer.sin_addr),sizeof(*addr)) == 0 )
            {
                SLPDatabaseRemove(dh,entry);
                SLPDLogDAAdvertisement("Removal",entry);
                break;            
            }
        }

        SLPDatabaseClose(dh);
    }
}


/*=========================================================================*/
void* SLPDKnownDAEnumStart()
/* Start an enumeration of all Known DAs                                   */
/*                                                                         */
/* Returns: An enumeration handle that is passed to subsequent calls to    */
/*          SLPDKnownDAEnum().  Returns NULL on failure.  Returned         */
/*          enumeration handle (if not NULL) must be passed to             */
/*          SLPDKnownDAEnumEnd() when you are done with it.                */
/*=========================================================================*/
{
    return SLPDatabaseOpen(&G_SlpdKnownDAs);   
}


/*=========================================================================*/
SLPMessage SLPDKnownDAEnum(void* eh, SLPMessage* msg, SLPBuffer* buf)
/* Enumerate through all Known DAs                                         */
/*                                                                         */
/* eh (IN) pointer to opaque data that is used to maintain                 */
/*         enumerate entries.  Pass in a pointer to NULL to start          */
/*         enumeration.                                                    */
/*                                                                         */
/* msg (OUT) pointer to the DAAdvert message descriptor                    */
/*                                                                         */
/* buf (OUT) pointer to the DAAdvert message buffer                        */
/*                                                                         */
/* returns: Pointer to enumerated entry or NULL if end of enumeration      */
/*=========================================================================*/
{
    SLPDatabaseEntry*   entry;
    entry = SLPDatabaseEnum((SLPDatabaseHandle) eh);
    if ( entry )
    {
        *msg = entry->msg;
        *buf = entry->buf;
    }
    else
    {
        *msg = 0;
        *buf = 0;
    }

    return *msg;
}


/*=========================================================================*/
void SLPDKnownDAEnumEnd(void* eh)
/* End an enumeration started by SLPDKnownDAEnumStart()                    */
/*                                                                         */
/* Parameters:  eh (IN) The enumeration handle returned by                 */
/*              SLPDKnownDAEnumStart()                                     */
/*=========================================================================*/
{
    if ( eh )
    {
        SLPDatabaseClose((SLPDatabaseHandle)eh);
    }
}


/*=========================================================================*/
int SLPDKnownDAGenerateMyDAAdvert(int errorcode,
                                  int deadda,
                                  int xid,
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

#ifdef ENABLE_SLPv2_SECURITY
    int                 daadvertauthlen  = 0;
    unsigned char*      daadvertauth     = 0;
    int                 spistrlen   = 0;
    char*               spistr      = 0;  

    G_SlpdProperty.DATimestamp += 1;

    if ( G_SlpdProperty.securityEnabled )
    {
        SLPSpiGetDefaultSPI(G_SlpdSpiHandle,
                            SLPSPI_KEY_TYPE_PRIVATE,
                            &spistrlen,
                            &spistr);

        SLPAuthSignDAAdvert(G_SlpdSpiHandle,
                            spistrlen,
                            spistr,
                            G_SlpdProperty.DATimestamp,
                            G_SlpdProperty.myUrlLen,
                            G_SlpdProperty.myUrl,
                            0,
                            0,
                            G_SlpdProperty.useScopesLen,
                            G_SlpdProperty.useScopes,
                            spistrlen,
                            spistr,
                            &daadvertauthlen,
                            &daadvertauth);
    }
#else
    G_SlpdProperty.DATimestamp += 1;
#endif





    /*-------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvrply */
    /*-------------------------------------------------------------*/
    size =  G_SlpdProperty.localeLen + 29; /* 14 bytes for header     */
                                           /*  2 errorcode  */
                                           /*  4 bytes for timestamp */
                                           /*  2 bytes for url len */
                                           /*  2 bytes for scope list len */
                                           /*  2 bytes for attr list len */
                                           /*  2 bytes for spi str len */
                                           /*  1 byte for authblock count */
    size += G_SlpdProperty.myUrlLen;
    size += G_SlpdProperty.useScopesLen;
#ifdef ENABLE_SLPv2_SECURITY
    size += spistrlen;
    size += daadvertauthlen; 
#endif

    result = SLPBufferRealloc(result, size);
    if ( result == 0 )
    {
        /* Out of memory, what should we do here! */
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
    ToUINT16(result->start + 12, G_SlpdProperty.localeLen);
    /*lang tag*/
    memcpy(result->start + 14,
           G_SlpdProperty.locale,
           G_SlpdProperty.localeLen);
    result->curpos = result->start + 14 + G_SlpdProperty.localeLen;

    /*--------------------------*/
    /* Add rest of the DAAdvert */
    /*--------------------------*/
    /* error code */
    ToUINT16(result->curpos,errorcode);
    result->curpos = result->curpos + 2;
    if ( errorcode == 0 )
    {
        /* timestamp */
        if ( deadda )
        {
            ToUINT32(result->curpos,0);
        }
        else
        {
            ToUINT32(result->curpos,G_SlpdProperty.DATimestamp);
        }
        result->curpos = result->curpos + 4;                       
        /* url len */
        ToUINT16(result->curpos, G_SlpdProperty.myUrlLen);
        result->curpos = result->curpos + 2;
        /* url */
        memcpy(result->curpos,
               G_SlpdProperty.myUrl,
               G_SlpdProperty.myUrlLen);
        result->curpos = result->curpos + G_SlpdProperty.myUrlLen;
        /* scope list len */
        ToUINT16(result->curpos, G_SlpdProperty.useScopesLen);
        result->curpos = result->curpos + 2;
        /* scope list */
        memcpy(result->curpos,
               G_SlpdProperty.useScopes,
               G_SlpdProperty.useScopesLen);
        result->curpos = result->curpos + G_SlpdProperty.useScopesLen;
        /* attr list len */
        ToUINT16(result->curpos, 0);
        result->curpos = result->curpos + 2;
        /* attr list */
        /* memcpy(result->start, ???, 0);                          */
        /* result->curpos = result->curpos + daentry->attrlistlen; */
        /* SPI List */
#ifdef ENABLE_SLPv2_SECURITY
        ToUINT16(result->curpos,spistrlen);
        result->curpos = result->curpos + 2;
        memcpy(result->curpos,spistr,spistrlen);
        result->curpos = result->curpos + spistrlen;
#else
        ToUINT16(result->curpos,0);
        result->curpos = result->curpos + 2;
#endif



        /* authblock count */
#ifdef ENABLE_SLPv2_SECURITY
        if ( daadvertauth )
        {
            /* authcount */
            *(result->curpos) = 1;
            result->curpos = result->curpos + 1;
            /* authblock */
            memcpy(result->curpos,daadvertauth,daadvertauthlen);
            result->curpos = result->curpos + daadvertauthlen;
        }
        else
#endif
        {
            *(result->curpos) = 0;
            result->curpos = result->curpos + 1;
        }
    }

    FINISHED:
#ifdef ENABLE_SLPv2_SECURITY
    if ( daadvertauth ) xfree(daadvertauth);
    if ( spistr ) xfree(spistr);
#endif
    *sendbuf = result;

    return errorcode;
}


#if defined(ENABLE_SLPv1)
/*=========================================================================*/
int SLPDKnownDAGenerateMyV1DAAdvert(int errorcode,
                                    int encoding,
                                    unsigned int xid,
                                    SLPBuffer* sendbuf)
/* Pack a buffer with a v1 DAAdvert using information from a SLPDAentry    */
/*                                                                         */
/* errorcode (IN) the errorcode for the DAAdvert                           */
/*                                                                         */
/* encoding (IN) the SLPv1 language encoding for the DAAdvert              */
/*                                                                         */
/* xid (IN) the xid to for the DAAdvert                                    */
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

    if ( !errorcode )
    {
        errorcode = SLPv1ToEncoding(0, 
                                    &urllen, 
                                    encoding,
                                    G_SlpdProperty.myUrl,
                                    G_SlpdProperty.myUrlLen);
        if ( !errorcode )
        {
            size += urllen;
#ifndef FAKE_UNSCOPED_DA
            errorcode = SLPv1ToEncoding(0, &scopelistlen,
                                        encoding,
                                        G_SlpdProperty.useScopes,
                                        G_SlpdProperty.useScopesLen);
#else
            scopelistlen = 0;   /* pretend that we're unscoped */
#endif
            if ( !errorcode )
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

    result = SLPBufferRealloc(result, size);
    if ( result == 0 )
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
    if ( G_SlpdProperty.locale )
    {
        memcpy(result->start + 6, G_SlpdProperty.locale, 2);
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
    SLPv1ToEncoding(result->curpos, 
                    &urllen, 
                    encoding, 
                    G_SlpdProperty.myUrl,
                    G_SlpdProperty.myUrlLen);
    result->curpos = result->curpos + urllen;
    /* scope list len */
    ToUINT16(result->curpos, scopelistlen);
    result->curpos = result->curpos + 2;
    /* scope list */
#ifndef FAKE_UNSCOPED_DA
    SLPv1ToEncoding(result->curpos, 
                    &scopelistlen,
                    encoding,  
                    G_SlpdProperty.useScopes,
                    G_SlpdProperty.useScopesLen);
#endif

    result->curpos = result->curpos + scopelistlen;

    FINISHED:
    *sendbuf = result;

    return errorcode;
}
#endif



/*=========================================================================*/
void SLPDKnownDAEcho(SLPMessage msg, SLPBuffer buf)
/* Echo a srvreg message to a known DA                                     */
/*									                                       */
/* msg (IN) the SrvReg message descriptor                                  */
/*                                                                         */
/* buf (IN) the SrvReg message buffer to echo                              */
/*                                                                         */
/* Returns:  none                                                          */
/*=========================================================================*/
{
    SLPBuffer           dup;
    SLPDatabaseHandle   dh;
    SLPDatabaseEntry*   entry;
    SLPDAAdvert*        entrydaadvert;
    SLPDSocket*         sock;
    const char*         msgscope;
    int                 msgscopelen;

    /* Do not echo registrations if we are a DA unless they were made  */
    /* local through the API!                                          */
    if ( G_SlpdProperty.isDA && !ISLOCAL(msg->peer.sin_addr) )
    {
        return;
    }

    if ( msg->header.functionid == SLP_FUNCT_SRVREG )
    {
        msgscope = msg->body.srvreg.scopelist;
        msgscopelen = msg->body.srvreg.scopelistlen;
    }
    else if ( msg->header.functionid == SLP_FUNCT_SRVDEREG )
    {
        msgscope = msg->body.srvdereg.scopelist;
        msgscopelen = msg->body.srvdereg.scopelistlen;
    }
    else
    {
        /* We only echo SRVREG and SRVDEREG */
        return;
    }

    dh = SLPDatabaseOpen(&G_SlpdKnownDAs);
    if ( dh )
    {

        /*-----------------------------------------------------*/
        /* Check to see if there is already an identical entry */
        /*-----------------------------------------------------*/
        while ( 1 )
        {
            entry = SLPDatabaseEnum(dh);
            if ( entry == NULL ) break;

            /* entrydaadvert is the DAAdvert message from the database */
            entrydaadvert = &(entry->msg->body.daadvert);

            /* Send to all DAs that have matching scope */
            if ( SLPIntersectStringList(msgscopelen,
                                        msgscope,
                                        entrydaadvert->scopelistlen,
                                        entrydaadvert->scopelist) )
            {
                /* Do not echo to ourselves if we are a DA*/
                if ( G_SlpdProperty.isDA  &&
                     SLPCompareString(G_SlpdProperty.myUrlLen,
                                      G_SlpdProperty.myUrl,
                                      entrydaadvert->urllen,
                                      entrydaadvert->url) == 0 )
                {
                    /* don't do anything because it makes no sense to echo */
                    /* to myself                                           */
                }
                else
                {
                    /*------------------------------------------*/
                    /* Load the socket with the message to send */
                    /*------------------------------------------*/
                    sock = SLPDOutgoingConnect(&(entry->msg->peer.sin_addr));
                    if ( sock )
                    {
                        dup = SLPBufferDup(buf);
                        if ( dup )
                        {
                            SLPListLinkTail(&(sock->sendlist),(SLPListItem*)dup);
                            if ( sock->state == STREAM_CONNECT_IDLE )
                            {
                                sock->state = STREAM_WRITE_FIRST;
                            }
                        }
                        else
                        {
                            sock->state = SOCKET_CLOSE;
                        }
                    }
                }
            }
        }
        SLPDatabaseClose(dh);
    }
}


/*=========================================================================*/
void SLPDKnownDAActiveDiscovery(int seconds)
/* Add a socket to the outgoing list to do active DA discovery SrvRqst     */
/*									                                       */
/* seconds (IN) number of seconds that expired since last call             */
/*                                                                         */
/* Returns:  none                                                          */
/*=========================================================================*/
{
    struct in_addr  peeraddr;
    SLPDSocket*     sock;

    /* Check to see if we should perform active DA detection */
    if ( G_SlpdProperty.DAActiveDiscoveryInterval == 0 )
    {
        return;
    }
    /* When activeDiscoveryXmits is < 0 then we should not xmit any more */
    if (G_SlpdProperty.activeDiscoveryXmits < 0)
    {
        return ;
    }

    if ( G_SlpdProperty.nextActiveDiscovery <= 0 )
    {
        if ( G_SlpdProperty.activeDiscoveryXmits == 0)
        {
            if (G_SlpdProperty.DAActiveDiscoveryInterval == 1)
            {/* ensures xmit on first call */
                /* don't xmit any more */
                G_SlpdProperty.activeDiscoveryXmits = -1;
            }
            else
            {
                G_SlpdProperty.nextActiveDiscovery = G_SlpdProperty.DAActiveDiscoveryInterval;
                G_SlpdProperty.activeDiscoveryXmits = 3;
            }
        }

        G_SlpdProperty.activeDiscoveryXmits --;

        /*--------------------------------------------------*/
        /* Create new DATAGRAM socket with appropriate peer */
        /*--------------------------------------------------*/
        if ( G_SlpdProperty.isBroadcastOnly == 0 )
        {
            peeraddr.s_addr = htonl(SLP_MCAST_ADDRESS);
            sock = SLPDSocketCreateDatagram(&peeraddr,DATAGRAM_MULTICAST);
        }
        else
        {
            peeraddr.s_addr = htonl(SLP_BCAST_ADDRESS);
            sock = SLPDSocketCreateDatagram(&peeraddr,DATAGRAM_BROADCAST);
        }

        if ( sock )
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
    SLPDSocket*     sock;
#ifdef ENABLE_SLPv1
    SLPDSocket*     v1sock;
#endif


    /* SAs don't send passive DAAdverts */
    if ( G_SlpdProperty.isDA == 0)
    {
        return;
    }

    /* Check to see if we should perform passive DA detection */
    if ( G_SlpdProperty.passiveDADetection == 0 )
    {
        return;
    }

    if ( G_SlpdProperty.nextPassiveDAAdvert <= 0 || dadead )
    {
        G_SlpdProperty.nextPassiveDAAdvert = G_SlpdProperty.DAHeartBeat;

        /*--------------------------------------------------*/
        /* Create new DATAGRAM socket with appropriate peer */
        /*--------------------------------------------------*/
        if ( G_SlpdProperty.isBroadcastOnly == 0 )
        {
            peeraddr.s_addr = htonl(SLP_MCAST_ADDRESS);
            sock = SLPDSocketCreateDatagram(&peeraddr,DATAGRAM_MULTICAST);

#ifdef ENABLE_SLPv1
            if ( !dadead )
            {
                peeraddr.s_addr = htonl(SLPv1_DA_MCAST_ADDRESS);
                v1sock = SLPDSocketCreateDatagram(&peeraddr,
                                                  DATAGRAM_MULTICAST);
            }
            else
            {
                v1sock = NULL;
            }
#endif  
        }
        else
        {
            peeraddr.s_addr = htonl(SLP_BCAST_ADDRESS);
            sock = SLPDSocketCreateDatagram(&peeraddr,DATAGRAM_BROADCAST);

#ifdef ENABLE_SLPv1
            if ( !dadead )
            {
                v1sock = SLPDSocketCreateDatagram(&peeraddr,DATAGRAM_BROADCAST);
            }
            else
            {
                v1sock = NULL;
            }
#endif
        }

        /* Generate the DAAdvert and link it to the write list */
        if ( sock )
        {
            if (SLPDKnownDAGenerateMyDAAdvert(0,dadead,0,&(sock->sendbuf)) == 0)
            {
                SLPDOutgoingDatagramWrite(sock);
            }
            else
            {
                SLPDSocketFree(sock);
            }
        }

#ifdef ENABLE_SLPv1
        if ( v1sock )
        {
            /* SLPv1 does not support shutdown messages */

            /* Generate the DAAdvert and write it */
            if (SLPDKnownDAGenerateMyV1DAAdvert(0,
                                                SLP_CHAR_UTF8,
                                                SLPXidGenerate(),
                                                &(v1sock->sendbuf)) == 0)
            {
                SLPDOutgoingDatagramWrite(v1sock);
            }
            else
            {
                SLPDSocketFree(v1sock);
            }
        }
#endif
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
    SLPDatabaseHandle   dh;
    SLPDatabaseEntry*   entry;
    SLPDAAdvert*        entrydaadvert;

    G_KnownDATimeSinceLastRefresh += seconds;

    if ( G_KnownDATimeSinceLastRefresh >= SLP_LIFETIME_MAXIMUM - seconds )
    {
        /* Refresh all SLP_LIFETIME_MAXIMUM registrations */
        dh = SLPDatabaseOpen(&G_SlpdKnownDAs);
        if(dh)
        {

            /*-----------------------------------------------------*/
            /* Check to see if there is already an identical entry */
            /*-----------------------------------------------------*/
            while ( 1 )
            {
                entry = SLPDatabaseEnum(dh);
                if ( entry == NULL ) break;

                /* entrydaadvert is the DAAdvert message from the database */
                entrydaadvert = &(entry->msg->body.daadvert);

                /* Assume DAs are identical if their URLs match */
                if ( SLPCompareString(entrydaadvert->urllen,
                                      entrydaadvert->url,
                                      G_SlpdProperty.myUrlLen,
                                      G_SlpdProperty.myUrl) )
                {
                    SLPDKnownDARegisterAll(entry->msg,1);
                }
            }   

            SLPDatabaseClose(dh);
        }

        G_KnownDATimeSinceLastRefresh = 0;
    }
}


/*=========================================================================*/
void SLPDKnownDADeRegisterWithAllDas(SLPMessage msg, SLPBuffer buf)
/* Deregister the registration described by the specified message          */
/*                                                                         */
/* msg (IN) A message descriptor for a SrvReg or SrvDereg message to       */
/*          deregister                                                     */
/*                                                                         */
/* buf (IN) Message buffer associated with msg                             */
/*                                                                         */
/* Returns: None                                                           */
/*=========================================================================*/
{
    SLPBuffer  sendbuf;
    
    if(msg->header.functionid == SLP_FUNCT_SRVREG)
    {
        if(MakeSrvderegFromSrvReg(msg,buf, &sendbuf) == 0)
        {
            SLPDKnownDAEcho(msg,sendbuf);
            SLPBufferFree(sendbuf); 
        }
    }
    else if (msg->header.functionid == SLP_FUNCT_SRVDEREG)
    {
        /* Simply echo the message through as is */
        SLPDKnownDAEcho(msg,buf);
    }
}


/*=========================================================================*/
void SLPDKnownDARegisterWithAllDas(SLPMessage msg, SLPBuffer buf)
/* Register the registration described by the specified message with all   */
/* known DAs                                                               */
/*                                                                         */
/* msg (IN) A message descriptor for a SrvReg or SrvDereg message to       */
/*          register                                                       */
/*                                                                         */
/* buf (IN) Message buffer associated with msg                             */
/*                                                                         */
/* Returns: None                                                           */
/*=========================================================================*/
{
    if (msg->header.functionid == SLP_FUNCT_SRVREG)
    {
        /* Simply echo the message through as is */
        SLPDKnownDAEcho(msg,buf);
    }
}


#ifdef DEBUG
/*=========================================================================*/
void SLPDKnownDADump()
/*=========================================================================*/
{
    SLPMessage      msg;
    SLPBuffer       buf;
    void* eh;

    eh = SLPDKnownDAEnumStart();
    if ( eh )
    {
        SLPDLog("========================================================================\n");
        SLPDLog("Dumping KnownDAs \n");
        SLPDLog("========================================================================\n");
        while ( SLPDKnownDAEnum(eh, &msg, &buf) )
        {
            SLPDLogMessageInternals(msg);
            SLPDLog("\n");
        }

        SLPDKnownDAEnumEnd(eh);
    }
}
#endif

