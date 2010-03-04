/*-------------------------------------------------------------------------
 * Copyright (C) 2000 Caldera Systems, Inc
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    Neither the name of Caldera Systems nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CALDERA
 * SYSTEMS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *-------------------------------------------------------------------------*/

/** Keeps track of known DA's.
 *
 * @file       slpd_knownda.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#include "slpd_knownda.h"
#include "slpd_property.h"
#include "slpd_database.h"
#include "slpd_socket.h"
#include "slpd_outgoing.h"
#include "slpd_log.h"
#include "slpd.h"
#include "slpd_incoming.h"  /*For the global incoming socket map.  Instead of creating a new 
                              socket for every multicast and broadcast, we'll simply send 
                              on the existing sockets, using their network interfaces*/

#ifdef ENABLE_SLPv2_SECURITY
# include "slpd_spi.h"
#endif

#include "slp_xmalloc.h"
#include "slp_v1message.h"
#include "slp_utf8.h"
#include "slp_compare.h"
#include "slp_xid.h"
#include "slp_dhcp.h"
#include "slp_parse.h"
#include "slp_net.h"

#ifdef ENABLE_SLPv2_SECURITY
# include "slp_auth.h"
# include "slp_spi.h"
#endif

/*=========================================================================*/
SLPDatabase G_SlpdKnownDAs;
/* The database of DAAdverts from DAs known to slpd.                       */
/*=========================================================================*/

/*=========================================================================*/
int G_KnownDATimeSinceLastRefresh = 0;
/*=========================================================================*/

/*=========================================================================*/
char* G_ifaceurls = 0;
size_t G_ifaceurlsLen = 0;
/* Used to filter out our own DA urls */
/*=========================================================================*/

/*-------------------------------------------------------------------------*/
int MakeActiveDiscoveryRqst(int ismcast, SLPBuffer * buffer)
/* Pack a buffer with service:directory-agent SrvRqst                      *
 *                                                                         *
 * Caller must free buffer                                                 *
 *-------------------------------------------------------------------------*/
{
   size_t size;
   void * eh;
   SLPMessage * msg;
   char * prlist = 0;
   size_t prlistlen = 0;
   int errorcode = 0;
   SLPBuffer tmp = 0;
   SLPBuffer result = *buffer;
   char addr_str[INET6_ADDRSTRLEN];

   /*-------------------------------------------------*/
   /* Generate a DA service request buffer to be sent */
   /*-------------------------------------------------*/
   /* determine the size of the fixed portion of the SRVRQST     */
   size = 47; /* 14 bytes for the header                        */
   /*  2 bytes for the prlistlen                     */
   /*  2 bytes for the srvtype length                */
   /* 23 bytes for "service:directory-agent" srvtype */
   /*  2 bytes for scopelistlen                      */
   /*  2 bytes for predicatelen                      */
   /*  2 bytes for sprstrlen                         */

   /* figure out what our Prlist will be by going through our list of  */
   /* known DAs                                                        */
   prlistlen = 0;
   prlist = xmalloc(SLP_MAX_DATAGRAM_SIZE - size);
   if (prlist == 0)
   {
      /* out of memory */
      errorcode = SLP_ERROR_INTERNAL_ERROR;
      goto FINISHED;
   }

   *prlist = 0;
   /* Don't send active discoveries to DAs we already know about */
   eh = SLPDKnownDAEnumStart();
   if (eh)
   {
      while (1)
      {
			size_t addrstrlen;

         if (SLPDKnownDAEnum(eh, &msg, &tmp) == 0)
            break;
         /*Make sure we have room for the potential pr addr*/
         if(SLP_MAX_DATAGRAM_SIZE - size < prlistlen + sizeof(addr_str) + 1) /*1 for the comma*/
            break;

         *addr_str = 0;
         SLPNetSockAddrStorageToString(&(msg->peer), addr_str,
                     sizeof(addr_str));
         addrstrlen = strlen(addr_str);
         if (addrstrlen > 0 && SLPContainsStringList(prlistlen,
                     prlist, addrstrlen, addr_str) == 0)
         {
            if (prlistlen != 0)
               strcat(prlist, ",");
            strcat(prlist, addr_str);
            prlistlen = strlen(prlist);
         }
      }

      SLPDKnownDAEnumEnd(eh);
   }

   /* Allocate the send buffer */
   size += G_SlpdProperty.localeLen + prlistlen;
   result = SLPBufferRealloc(result, size);
   if (result == 0)
   {
      /* out of memory */
      errorcode = SLP_ERROR_INTERNAL_ERROR;
      goto FINISHED;
   }

   /*------------------------------------------------------------*/
   /* Build a buffer containing the fixed portion of the SRVRQST */
   /*------------------------------------------------------------*/

   /* version */
   *result->curpos++ = 2;

   /* function id */
   *result->curpos++ = SLP_FUNCT_SRVRQST;

   /* length */
   PutUINT24(&result->curpos, size);

   /* flags */
   PutUINT16(&result->curpos, (ismcast ? SLP_FLAG_MCAST : 0));

   /* ext offset */
   PutUINT24(&result->curpos, 0);

   /* xid */
   PutUINT16(&result->curpos, SLPXidGenerate());

   /* lang tag len */
   PutUINT16(&result->curpos, G_SlpdProperty.localeLen);

   /* lang tag */
   memcpy(result->curpos, G_SlpdProperty.locale, G_SlpdProperty.localeLen);
   result->curpos += G_SlpdProperty.localeLen;

   /* Prlist */
   PutUINT16(&result->curpos, prlistlen);
   memcpy(result->curpos, prlist, prlistlen);
   result->curpos += prlistlen;

   /* service type */
   PutUINT16(&result->curpos, 23);                                         

   /* 23 is the length of SLP_DA_SERVICE_TYPE */
   memcpy(result->curpos, SLP_DA_SERVICE_TYPE, 23);
   result->curpos += 23;

   /* scope list zero length */
   PutUINT16(&result->curpos, 0);

   /* predicate zero length */
   PutUINT16(&result->curpos, 0);

   /* spi list zero length */
   PutUINT16(&result->curpos, 0);

   *buffer = result;

FINISHED:

   if (prlist)
      xfree(prlist);

   return 0;
}


/*-------------------------------------------------------------------------*/
void SLPDKnownDARegisterAll(SLPMessage * daadvert, int immortalonly)
/* registers all services with specified DA                                */
/*-------------------------------------------------------------------------*/
{
   SLPBuffer buf;
   SLPMessage * msg;
   SLPSrvReg * srvreg;
   SLPDSocket * sock;
   SLPBuffer sendbuf = 0;
   void * handle = 0;

   /*---------------------------------------------------------------*/
   /* Check to see if the database is empty and open an enumeration */
   /* handle if it is not empty                                     */
   /*---------------------------------------------------------------*/
   if (SLPDDatabaseIsEmpty())
      return;

   /*--------------------------------------*/
   /* Never do a Register All to ourselves */
   /*--------------------------------------*/
   if(SLPIntersectStringList(G_ifaceurlsLen, G_ifaceurls, daadvert->body.daadvert.urllen,
                             daadvert->body.daadvert.url) > 0)
      return;

   handle = SLPDDatabaseEnumStart();
   if (handle == 0)
      return;

   /*----------------------------------------------*/
   /* Establish a new connection with the known DA */
   /*----------------------------------------------*/
   sock = SLPDOutgoingConnect(0, &(daadvert->peer));
   if (sock)
      while (1)
      {
         msg = SLPDDatabaseEnum(handle, &msg, &buf);
         if (msg == NULL)
            break;
         srvreg = &(msg->body.srvreg);

         /*-----------------------------------------------*/
         /* If so instructed, skip mortal registrations   */
         /*-----------------------------------------------*/
         if (immortalonly && srvreg->urlentry.lifetime < SLP_LIFETIME_MAXIMUM)
            continue;

         /*---------------------------------------------------------*/
         /* Only register local (or static) registrations of scopes */
         /* supported by peer DA                                    */
         /*---------------------------------------------------------*/
         if ((srvreg->source == SLP_REG_SOURCE_LOCAL
               || srvreg->source == SLP_REG_SOURCE_STATIC)
               && SLPIntersectStringList(srvreg->scopelistlen,
                                         srvreg->scopelist,
                                         daadvert->body.daadvert.scopelistlen,
                                         daadvert->body.daadvert.scopelist))
         {
            sendbuf = SLPBufferDup(buf);
            if (sendbuf)
            {
               /*--------------------------------------------------------------*/
               /* link newly constructed buffer to socket resendlist, and send */
               /*--------------------------------------------------------------*/
               SLPListLinkTail(&(sock->sendlist), (SLPListItem *) sendbuf);
               SLPDOutgoingDatagramWrite(sock, sendbuf);
            }
         }
      }

   SLPDDatabaseEnumEnd(handle);
}  


/*-------------------------------------------------------------------------*/
/* Pack a buffer with a SrvDereg message using information from an existing
 * SrvReg message
 * 
 * Caller must free outbuf
 *-------------------------------------------------------------------------*/
int MakeSrvderegFromSrvReg(SLPMessage * msg, SLPBuffer inbuf, SLPBuffer * outbuf)
{
   size_t size;
   SLPBuffer sendbuf;
   SLPSrvReg * srvreg;

   (void)inbuf;

   srvreg = &(msg->body.srvreg);

   /*-------------------------------------------------------------*/
   /* ensure the buffer is big enough to handle the whole srvdereg*/
   /*-------------------------------------------------------------*/
   size = msg->header.langtaglen + 18; /* 14 bytes for header     */
   /*  2 bytes for scopelen */
   /*  see below for URLEntry */
   /*  2 bytes for taglist len */
   if (srvreg->urlentry.opaque)
      size += srvreg->urlentry.opaquelen;
   else
   {
      size += 6; /* +6 for the static portion of the url-entry */
      size += srvreg->urlentry.urllen;
   }
   size += srvreg->scopelistlen;
   /* taglistlen is always 0 */

   *outbuf = sendbuf = SLPBufferAlloc(size);
   if (*outbuf == NULL)
      return SLP_ERROR_INTERNAL_ERROR;

   /*----------------------*/
   /* Construct a SrvDereg */
   /*----------------------*/

   /* version */
   *sendbuf->curpos++ = 2;

   /* function id */
   *sendbuf->curpos++ = SLP_FUNCT_SRVDEREG;

   /* length */
   PutUINT24(&sendbuf->curpos, size);

   /* flags */
   PutUINT16(&sendbuf->curpos, (size > SLP_MAX_DATAGRAM_SIZE? 
         SLP_FLAG_OVERFLOW: 0));

   /* ext offset */
   PutUINT24(&sendbuf->curpos, 0);

   /* xid */
   PutUINT16(&sendbuf->curpos, SLPXidGenerate());

   /* lang tag len */
   PutUINT16(&sendbuf->curpos, msg->header.langtaglen);

   /* lang tag */
   memcpy(sendbuf->curpos, msg->header.langtag, msg->header.langtaglen);
   sendbuf->curpos += msg->header.langtaglen;

   /* scope list */
   PutUINT16(&sendbuf->curpos, srvreg->scopelistlen);
   memcpy(sendbuf->curpos, srvreg->scopelist, srvreg->scopelistlen);
   sendbuf->curpos += srvreg->scopelistlen;

   /* the URL entry */
#ifdef ENABLE_SLPv1
   if (srvreg->urlentry.opaque == 0)
   {
      /* url-entry reserved */
      *sendbuf->curpos++ = 0;        

      /* url-entry lifetime */
      PutUINT16(&sendbuf->curpos, srvreg->urlentry.lifetime);

      /* url-entry urllen */
      PutUINT16(&sendbuf->curpos, srvreg->urlentry.urllen);

      /* url-entry url */
      memcpy(sendbuf->curpos, srvreg->urlentry.url, srvreg->urlentry.urllen);
      sendbuf->curpos += srvreg->urlentry.urllen;

      /* url-entry authcount */
      *sendbuf->curpos++ = 0;
   }
   else
#endif /* ENABLE_SLPv1 */
   {
      memcpy(sendbuf->curpos, srvreg->urlentry.opaque,
            srvreg->urlentry.opaquelen);
      sendbuf->curpos += srvreg->urlentry.opaquelen;
   }

   /* taglist (always 0) */
   PutUINT16(&sendbuf->curpos, 0);

   return 0;
}


/*-------------------------------------------------------------------------*/
void SLPDKnownDADeregisterAll(SLPMessage * daadvert)
/* de-registers all services with specified DA                             */
/*-------------------------------------------------------------------------*/
{
   SLPBuffer buf;
   SLPMessage * msg;
   SLPSrvReg * srvreg;
   SLPDSocket * sock;
   SLPBuffer sendbuf = 0;
   void * handle = 0;

   /*---------------------------------------------------------------*/
   /* Check to see if the database is empty and open an enumeration */
   /* handle if it is not empty                                     */
   /*---------------------------------------------------------------*/
   if (SLPDDatabaseIsEmpty())
      return;
   handle = SLPDDatabaseEnumStart();
   if (handle == 0)
      return;

   /* Establish a new connection with the known DA */
   sock = SLPDOutgoingConnect(0, &(daadvert->peer));
   if (sock)
      while (1)
      {
         msg = SLPDDatabaseEnum(handle, &msg, &buf);
         if (msg == NULL)
            break;
         srvreg = &(msg->body.srvreg);

        /*-----------------------------------------------------------*/
        /* Only Deregister local (or static) registrations of scopes */
        /* supported by peer DA                                      */
        /*-----------------------------------------------------------*/
        if ( ( srvreg->source == SLP_REG_SOURCE_LOCAL ||
               srvreg->source == SLP_REG_SOURCE_STATIC ) &&
            SLPIntersectStringList(srvreg->scopelistlen,
                                   srvreg->scopelist,
                                   daadvert->body.daadvert.scopelistlen,
                                   daadvert->body.daadvert.scopelist) )
         {
            if (MakeSrvderegFromSrvReg(msg, buf, &sendbuf) == 0)
            {
               /*--------------------------------------------------------------*/
               /* link newly constructed buffer to socket resendlist, and send */
               /*--------------------------------------------------------------*/
               SLPListLinkTail(&(sock->sendlist), (SLPListItem *) sendbuf);
               SLPDOutgoingDatagramWrite(sock, sendbuf);
            }
         }
      }

   SLPDDatabaseEnumEnd(handle);
}


/*=========================================================================*/
int SLPDKnownDAFromDHCP()
/* Queries DHCP for configured DA's.  IPv4 is the only supported address   */
/* family presently.                                                       */
/* returns  zero on success, Non-zero on failure                           */
/*=========================================================================*/
{
   SLPBuffer buf;
   DHCPContext ctx;
   SLPDSocket * sock;
   struct sockaddr_storage daaddr;
   unsigned char * alp;
   unsigned char             dhcpOpts[] =
   {
      TAG_SLP_SCOPE, TAG_SLP_DA
   };

   *ctx.scopelist = 0;
   ctx.addrlistlen = 0;

   DHCPGetOptionInfo(dhcpOpts, sizeof(dhcpOpts), DHCPParseSLPTags, &ctx);

   alp = ctx.addrlist;
   while (ctx.addrlistlen >= 4)
   {
      memset(&daaddr, 0, sizeof(struct sockaddr_in)); /*Some platforms require sin_zero be 0*/
      daaddr.ss_family = AF_INET;
      memcpy(&(((struct sockaddr_in *) &daaddr)->sin_addr.s_addr), alp, 4);
      if (&(((struct sockaddr_in *) &daaddr)->sin_addr.s_addr))
      {
         /*--------------------------------------------------------
               Get an outgoing socket to the DA and set it up to make
               the service:directoryagent request
              --------------------------------------------------------*/
         sock = SLPDOutgoingConnect(0, &daaddr);
         if (sock)
         {
            buf = 0;
            if (MakeActiveDiscoveryRqst(0, &buf) == 0)
            {
               SLPListLinkTail(&(sock->sendlist), (SLPListItem *) buf);
               SLPDOutgoingDatagramWrite(sock, buf);
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
/* Queries static configuration for DA's.                                  */
/*                                                                         */
/* returns  zero on success, Non-zero on failure                           */
/*=========================================================================*/
{
   char * temp;
   char * tempend;
   char * slider1;
   char * slider2;
   struct addrinfo * ai;
   struct addrinfo * ai_ref;
   struct sockaddr_storage daaddr;
   int daaddr_isset;
   SLPDSocket * sock;
   SLPBuffer buf;

   if (G_SlpdProperty.DAAddresses && *G_SlpdProperty.DAAddresses)
   {
      temp = slider1 = xstrdup(G_SlpdProperty.DAAddresses);
      if (temp)
      {
         tempend = temp + strlen(temp);
         while (slider1 < tempend)
         {
            while (*slider1 && *slider1 == ' ')
               slider1++;
            slider2 = slider1;
            while (*slider2 && *slider2 != ',')
               slider2++;
            *slider2++ = 0;

            daaddr_isset = 0;
            if (inet_pton(AF_INET6, slider1,
                     &((struct sockaddr_in6 *) &daaddr)->sin6_addr) > 0)
            {
               daaddr.ss_family = AF_INET6;
               ((struct sockaddr_in6 *) &daaddr)->sin6_family = AF_INET6;
               daaddr_isset = 1;
            }
            else if (inet_pton(AF_INET, slider1,
                           &((struct sockaddr_in *) &daaddr)->sin_addr) > 0)
            {
               daaddr.ss_family = AF_INET;
               ((struct sockaddr_in *) &daaddr)->sin_family = AF_INET;
               daaddr_isset = 1;
            }
            else if (getaddrinfo(slider1, NULL, NULL, &ai) == 0)
            {
               ai_ref = ai;
               while (ai_ref != NULL)
               {
                  if (SLPNetIsIPV6() && ai_ref->ai_addr->sa_family == AF_INET6)
                  {
                     memcpy(&daaddr, &ai_ref->ai_addr,
                           sizeof(struct sockaddr_in6));
                     daaddr_isset = 1;
                     break;
                     /* we prefer IPv6 when configured, so we'll take the first IPv6 address and break */
                  }
                  else if (SLPNetIsIPV4()
                        && !daaddr_isset
                        && ai_ref->ai_addr->sa_family == AF_INET)
                  {
                     memcpy(&daaddr, &ai_ref->ai_addr,
                           sizeof(struct sockaddr_in));
                     daaddr_isset = 1;
                     /* we'll continue searching for an IPv6 address, but we'll use the first IPv4 address if none are found */
                  }
               }

               freeaddrinfo(ai);
            }

            if (daaddr_isset)
            {
               /*--------------------------------------------------------*/
               /* Get an outgoing socket to the DA and set it up to make */
               /* the service:directoryagent request                     */
               /*--------------------------------------------------------*/
               sock = SLPDOutgoingConnect(0, &daaddr);
               if (sock)
               {
                  buf = 0;
                  if (MakeActiveDiscoveryRqst(0, &buf) == 0)
                  {
                     SLPListLinkTail(&(sock->sendlist), (SLPListItem *) buf);
                     SLPDOutgoingDatagramWrite(sock, buf);
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
void SLPDKnownDAGenerateIfaceURLs()
/* Initializes G_ifaceurls and G_ifaceurlsLen from GSlpdProperties.ifaceInfo */
/* Much of this is pulled from SLPDKnownDAGenerateMyDAAdvert               */
/*=========================================================================*/
{
   int i;
   char localaddr_str[INET6_ADDRSTRLEN + 2];
   struct sockaddr_storage* localaddr;
   size_t strmax = G_SlpdProperty.ifaceInfo.iface_count * (G_SlpdProperty.urlPrefixLen + INET6_ADDRSTRLEN + 2 + 1);  /*The +1 is for the ','*/

   if(strmax)
   {
      ++strmax;  /*For the terminating NULL*/
      G_ifaceurls = xmalloc(strmax);
      if(G_ifaceurls)
      {
         G_ifaceurls[0] = '\0';
         for(i = 0; i < G_SlpdProperty.ifaceInfo.iface_count; ++i)
         {
            /*build the ipaddr string*/
            localaddr_str[0] = '\0';
            localaddr = G_SlpdProperty.ifaceInfo.iface_addr + i;
            if (localaddr->ss_family == AF_INET)
               inet_ntop(localaddr->ss_family,
                     &((struct sockaddr_in *) localaddr)->sin_addr, localaddr_str,
                     sizeof(localaddr_str));
            else if (localaddr->ss_family == AF_INET6)
            {
               strcpy(localaddr_str, "[");
               inet_ntop(localaddr->ss_family,
                     &((struct sockaddr_in6 *) localaddr)->sin6_addr,
                     &localaddr_str[1], sizeof(localaddr_str) - 1);
               strcat(localaddr_str, "]");
            }

            /*Add the url to G_ifaceurls*/
            strncat(G_ifaceurls, G_SlpdProperty.urlPrefix, G_SlpdProperty.urlPrefixLen);
            strcat(G_ifaceurls, localaddr_str);
            strcat(G_ifaceurls, ",");
         }
         G_ifaceurlsLen = strlen(G_ifaceurls);
      }
   }
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
   /* Discover DHCP DA's and add them to the active discovery list.    */
   /*-----------------------------------------------------------------*/
   SLPDKnownDAFromDHCP();

   /*------------------------------------------------------------------*/
   /* Generate the list of our URLs                                    */
   /*------------------------------------------------------------------*/
   SLPDKnownDAGenerateIfaceURLs();

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
   SLPDatabaseHandle dh;
   SLPDatabaseEntry * entry;
   dh = SLPDatabaseOpen(&G_SlpdKnownDAs);
   if (dh)
   {
      /*------------------------------------*/
      /* Unregister all local registrations */
      /*------------------------------------*/
      while (1)
      {
         entry = SLPDatabaseEnum(dh);
         if (entry == NULL)
            break;

         SLPDKnownDADeregisterAll(entry->msg);
      }
      SLPDatabaseClose(dh);
   }

   SLPDatabaseDeinit(&G_SlpdKnownDAs);

   if(G_ifaceurls)
      xfree(G_ifaceurls);

   return 0;
} 


/*=========================================================================*/
int SLPDKnownDAAdd(SLPMessage * msg, SLPBuffer buf)
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
   SLPDatabaseEntry * entry;
   SLPDAAdvert * entrydaadvert;
   SLPDAAdvert * daadvert;
   struct sockaddr_storage daaddr;
   SLPParsedSrvUrl * parsedurl = NULL;
   int result = 0;
   SLPDatabaseHandle dh = NULL;
   uint32_t saved_scope = 0;

   dh = SLPDatabaseOpen(&G_SlpdKnownDAs);
   if (dh == NULL)
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

   if(SLPNetIsIPV6())
   {
      /*We need to save the scope of the address, since a multi-nic machine
        with multiple link-local addresses has no other way of differentiating route*/
      saved_scope = ((struct sockaddr_in6*)&msg->peer)->sin6_scope_id;
   }

   if (SLPParseSrvUrl(daadvert->urllen, daadvert->url, &parsedurl))
   {
      /* could not parse the DA service url */
      result = SLP_ERROR_PARSE_ERROR;
      goto CLEANUP;
   }
   if (SLPNetResolveHostToAddr(parsedurl->host, &daaddr))
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
   msg->peer = daaddr;

   if(SLPNetIsIPV6())
   {
      /*reset the scope*/
      ((struct sockaddr_in6*)&msg->peer)->sin6_scope_id = saved_scope;
   }



   /*-----------------------------------------------------*/
   /* Check to see if there is already an identical entry */
   /*-----------------------------------------------------*/
   while (1)
   {
      entry = SLPDatabaseEnum(dh);
      if (entry == NULL)
         break;

      /* entrydaadvert is the DAAdvert message from the database */
      entrydaadvert = &(entry->msg->body.daadvert);

      /* Assume DAs are identical if their URLs match */
      if (SLPCompareString(entrydaadvert->urllen, entrydaadvert->url,
               daadvert->urllen, daadvert->url) == 0)
      {
#ifdef ENABLE_SLPv2_SECURITY
         if (G_SlpdProperty.checkSourceAddr)
         {
            if ((entry->msg->peer.ss_family == AF_INET
                  && msg->peer.ss_family == AF_INET
                  && memcmp(&(((struct sockaddr_in *)&entry->msg->peer)->sin_addr),
                           &(((struct sockaddr_in *)&msg->peer)->sin_addr),
                           sizeof(struct in_addr)))
                  || (entry->msg->peer.ss_family == AF_INET6
                  && msg->peer.ss_family == AF_INET6
                  && memcmp(&(((struct sockaddr_in6 *)&entry->msg->peer)->sin6_addr),
                           &(((struct sockaddr_in6 *)&msg->peer)->sin6_addr),
                           sizeof(struct in6_addr))))
            {
               SLPDatabaseClose(dh);
               result = SLP_ERROR_AUTHENTICATION_FAILED;
               goto CLEANUP;
            }
         }

         /* make sure an unauthenticated DAAdvert can't replace */
         /* an authenticated one                                */
         if (entrydaadvert->authcount
               && entrydaadvert->authcount != daadvert->authcount)
         {
            SLPDatabaseClose(dh);
            result = SLP_ERROR_AUTHENTICATION_FAILED;
            goto CLEANUP;
         } 
#endif
         if (daadvert->bootstamp != 0
               && daadvert->bootstamp != entrydaadvert->bootstamp)
         {
            /* Advertising DA must have went down then came back up */
            SLPDLogDAAdvertisement("Replacement", entry);
            SLPDKnownDARegisterAll(msg, 0);
         }

         if ( daadvert->bootstamp == 0 )
         {
            /* Dying DA was found in our KnownDA database. Log that it
             * was removed.
             */
            SLPDLogDAAdvertisement("Removal", entry);
         }

         /* Remove the entry that is the same as the advertised entry */
         /* so that we can put the new advertised entry back in       */
         SLPDatabaseRemove(dh, entry);
         break;
      }
   }

   /* Make sure the DA is not dying */
   if (daadvert->bootstamp != 0)
   {
      if (entry == 0)
      {
         /* create a new database entry using the DAAdvert message */
         entry = SLPDatabaseEntryCreate(msg, buf);
         if (entry)
         {
            /* reset the "time to stale" count to indicate the DA is active (not stale) */
            entry->entryvalue = G_SlpdProperty.staleDACheckPeriod / SLPD_AGE_INTERVAL;

            SLPDatabaseAdd(dh, entry);

            /* register all the services we know about with this new DA */
            SLPDKnownDARegisterAll(msg, 0);

            /* log the addition of a new DA */
            SLPDLogDAAdvertisement("Addition", entry);
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
         entry = SLPDatabaseEntryCreate(msg, buf);
         if (entry)
         {
            /* reset the "time to stale" count to indicate the DA is active (not stale) */
            entry->entryvalue = G_SlpdProperty.staleDACheckPeriod / SLPD_AGE_INTERVAL;

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

   CLEANUP:
   /* If we are here, we need to cleanup the message descriptor and the  */
   /* message buffer because they were not added to the database and not */
   /* cleaning them up would result in a memory leak                     */
   /* We also need to make sure the Database handle is closed.           */
   SLPMessageFree(msg);
   SLPBufferFree(buf);
   if (dh)
      SLPDatabaseClose(dh);

   return result;
}

/*=========================================================================*/
void SLPDKnownDARemove(struct sockaddr_storage * addr)
/* Removes known DAs that sent DAAdverts from the specified in_addr        */
/*=========================================================================*/
{
   SLPDatabaseHandle dh;
   SLPDatabaseEntry * entry;

   dh = SLPDatabaseOpen(&G_SlpdKnownDAs);
   if (dh)
   {
      /*-----------------------------------------------------*/
      /* Check to see if there is already an identical entry */
      /*-----------------------------------------------------*/
      while (1)
      {
         entry = SLPDatabaseEnum(dh);
         if (entry == NULL)
            break;

         /* Assume DAs are identical if their peer match */
         if ((entry->msg->peer.ss_family == AF_INET
              && addr->ss_family == AF_INET
              && (0 == memcmp(&(((struct sockaddr_in *) &(entry->msg->peer))->sin_addr),
                              &(((struct sockaddr_in *) addr)->sin_addr),
                              sizeof(struct in_addr))))
             || (entry->msg->peer.ss_family == AF_INET6
                 && addr->ss_family == AF_INET6
                 && (0 == memcmp(&(((struct sockaddr_in6 *) &(entry->msg->peer))->sin6_addr),
                                 &(((struct sockaddr_in6 *) addr)->sin6_addr),
                                 sizeof(struct in6_addr)))))
         {
            SLPDLogDAAdvertisement("Removal", entry);
            SLPDatabaseRemove(dh, entry);
            break;
         }
      }

      SLPDatabaseClose(dh);
   }
}


/*=========================================================================*/
void * SLPDKnownDAEnumStart()
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
SLPMessage * SLPDKnownDAEnum(void * eh, SLPMessage ** msg, SLPBuffer * buf)
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
   SLPDatabaseEntry * entry;
   entry = SLPDatabaseEnum((SLPDatabaseHandle) eh);
   if (entry)
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
void SLPDKnownDAEnumEnd(void * eh)
/* End an enumeration started by SLPDKnownDAEnumStart()                    */
/*                                                                         */
/* Parameters:  eh (IN) The enumeration handle returned by                 */
/*              SLPDKnownDAEnumStart()                                     */
/*=========================================================================*/
{
   if (eh)
      SLPDatabaseClose((SLPDatabaseHandle) eh);
}

/*=========================================================================*/
int SLPDKnownDAGenerateMyDAAdvert(struct sockaddr_storage * localaddr,
      int errorcode, int deadda, int ismcast, int xid, SLPBuffer * sendbuf) 
/* Pack a buffer with a DAAdvert using information from a SLPDAentry       */
/*                                                                         */
/* localaddr (IN) the address of the DA to advertise                       */
/*                                                                         */
/* errorcode (IN) the errorcode for the DAAdvert                           */
/*                                                                         */
/* deadda (IN) whether the DAAdvert should indicate the DA is shut down    */
/*                                                                         */
/* ismcast (IN) whether the mcast flag should be set for the DAAdvert      */
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
   char localaddr_str[INET6_ADDRSTRLEN + 2];
   size_t size;
   SLPBuffer result = *sendbuf;

#ifdef ENABLE_SLPv2_SECURITY
   int daadvertauthlen = 0;
   unsigned char * daadvertauth = 0;
   size_t spistrlen = 0;
   char * spistr = 0;  

   /** @todo Use correct delay value - 1000 is just an arbitrary choice. */
   uint32_t expires = (uint32_t)time(0) + 1000;

   if (G_SlpdProperty.securityEnabled)
   {
      SLPSpiGetDefaultSPI(G_SlpdSpiHandle, SLPSPI_KEY_TYPE_PRIVATE,
            &spistrlen, &spistr);

      SLPAuthSignDAAdvert(G_SlpdSpiHandle, spistrlen, spistr,
            expires, G_SlpdProperty.urlPrefixLen,
            G_SlpdProperty.urlPrefix, 0, 0, G_SlpdProperty.useScopesLen,
            G_SlpdProperty.useScopes, spistrlen, spistr, &daadvertauthlen,
            &daadvertauth);
   }
#endif

   /*-------------------------------------------------------------*/
   /* ensure the buffer is big enough to handle the whole srvrply */
   /*-------------------------------------------------------------*/
   size = G_SlpdProperty.localeLen + 29; /* 14 bytes for header     */
   /*  2 errorcode  */
   /*  4 bytes for timestamp */
   /*  2 bytes for url len */
   /*  2 bytes for scope list len */
   /*  2 bytes for attr list len */
   /*  2 bytes for spi str len */
   /*  1 byte for authblock count */
   size += G_SlpdProperty.urlPrefixLen;
   localaddr_str[0] = '\0';
   if (localaddr->ss_family == AF_INET)
      inet_ntop(localaddr->ss_family,
            &((struct sockaddr_in *) localaddr)->sin_addr, localaddr_str,
            sizeof(localaddr_str));
   else if (localaddr->ss_family == AF_INET6)
   {
      strcpy(localaddr_str, "[");
      inet_ntop(localaddr->ss_family,
            &((struct sockaddr_in6 *) localaddr)->sin6_addr,
            &localaddr_str[1], sizeof(localaddr_str) - 1);
      strcat(localaddr_str, "]");
   }
   size += strlen(localaddr_str);
   size += G_SlpdProperty.useScopesLen;
#ifdef ENABLE_SLPv2_SECURITY
   size += spistrlen;
   size += daadvertauthlen; 
#endif

   result = SLPBufferRealloc(result, size);
   if (result == 0)
   {
      /* Out of memory, what should we do here! */
      errorcode = SLP_ERROR_INTERNAL_ERROR;
      goto FINISHED;
   }

   /*----------------*/
   /* Add the header */
   /*----------------*/

   /* version */
   *result->curpos++ = 2;

   /* function id */
   *result->curpos++ = SLP_FUNCT_DAADVERT;

   /* length */
   PutUINT24(&result->curpos, size);

   /* flags */
   PutUINT16(&result->curpos,
             (size > SLP_MAX_DATAGRAM_SIZE ? SLP_FLAG_OVERFLOW : 0) |
             (ismcast ? SLP_FLAG_MCAST : 0));

   /* ext offset */
   PutUINT24(&result->curpos, 0);

   /* xid */
   PutUINT16(&result->curpos, xid);

   /* lang tag len */
   PutUINT16(&result->curpos, G_SlpdProperty.localeLen);

   /* lang tag */
   memcpy(result->curpos, G_SlpdProperty.locale, G_SlpdProperty.localeLen);
   result->curpos += G_SlpdProperty.localeLen;

   /*--------------------------*/
   /* Add rest of the DAAdvert */
   /*--------------------------*/

   /* error code */
   PutUINT16(&result->curpos, errorcode);
   if (errorcode == 0)
   {
      /* timestamp */
      if (deadda)
         PutUINT32(&result->curpos, 0);
      else
         PutUINT32(&result->curpos, G_SlpdProperty.DATimestamp);

      /* url len */
      PutUINT16(&result->curpos, G_SlpdProperty.urlPrefixLen 
            + strlen(localaddr_str));

      /* url */
      memcpy(result->curpos, G_SlpdProperty.urlPrefix,
            G_SlpdProperty.urlPrefixLen);
      result->curpos += G_SlpdProperty.urlPrefixLen;
      memcpy(result->curpos, localaddr_str, strlen(localaddr_str));
      result->curpos += strlen(localaddr_str);

      /* scope list len */
      PutUINT16(&result->curpos, G_SlpdProperty.useScopesLen);

      /* scope list */
      memcpy(result->curpos, G_SlpdProperty.useScopes,
            G_SlpdProperty.useScopesLen);
      result->curpos += G_SlpdProperty.useScopesLen;

      /* attr list len */
      PutUINT16(&result->curpos, 0);

      /* attr list */
      /* memcpy(result->start, ???, 0);                          */
      /* result->curpos = result->curpos + daentry->attrlistlen; */
      /* SPI List */
#ifdef ENABLE_SLPv2_SECURITY
      PutUINT16(&result->curpos, spistrlen);
      memcpy(result->curpos, spistr, spistrlen);
      result->curpos += spistrlen;
#else
      PutUINT16(&result->curpos, 0);
#endif

      /* authblock count */
#ifdef ENABLE_SLPv2_SECURITY
      if (daadvertauth)
      {
         /* authcount */
         *result->curpos++ = 1;

         /* authblock */
         memcpy(result->curpos, daadvertauth, daadvertauthlen);
         result->curpos += daadvertauthlen;
      }
      else
#endif
         *result->curpos++ = 0;
   }

FINISHED:

#ifdef ENABLE_SLPv2_SECURITY
   if (daadvertauth)
      xfree(daadvertauth);
   if (spistr)
      xfree(spistr);
#endif
   *sendbuf = result;

   return errorcode;
}

#if defined(ENABLE_SLPv1)
/*=========================================================================*/
int SLPDKnownDAGenerateMyV1DAAdvert(struct sockaddr_storage * localaddr,
      int errorcode, int encoding, unsigned int xid, SLPBuffer * sendbuf)
/* Pack a buffer with a v1 DAAdvert using information from a SLPDAentry    */
/*                                                                         */
/* localaddr (IN) the address of the DA to advertise                       */
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
   size_t size = 0;
   size_t urllen = INT_MAX;
   size_t scopelistlen = INT_MAX;
   SLPBuffer result = *sendbuf;
   char localaddr_str[INET6_ADDRSTRLEN + 2];
   char da_url[INET6_ADDRSTRLEN + 29];

   /*-------------------------------------------------------------*/
   /* ensure the buffer is big enough to handle the whole srvrply */
   /*-------------------------------------------------------------*/
   size = 18; /* 12 bytes for header     */
   /*  2 errorcode  */
   /*  2 bytes for url len */
   /*  2 bytes for scope list len */

   /* Create the local address in string form */
   localaddr_str[0] = '\0';
   if (localaddr->ss_family == AF_INET)
      inet_ntop(localaddr->ss_family,
            &((struct sockaddr_in *) localaddr)->sin_addr, localaddr_str,
            sizeof(localaddr_str));
   else if (localaddr->ss_family == AF_INET6)
   {
      strcpy(localaddr_str, "[");
      inet_ntop(localaddr->ss_family,
            &((struct sockaddr_in6 *) localaddr)->sin6_addr,
            &localaddr_str[1], sizeof(localaddr_str) - 1);
      strcat(localaddr_str, "]");
   }

   /* Create the DA URL */
   da_url[0] = '\0';
   strcpy(da_url, G_SlpdProperty.urlPrefix);
   strcat(da_url, localaddr_str);

   if (!errorcode)
   {
      errorcode = SLPv1ToEncoding(0, &urllen, encoding, da_url, strlen(da_url));
      if (!errorcode)
      {
         size += urllen;
#ifndef FAKE_UNSCOPED_DA
         errorcode = SLPv1ToEncoding(0, &scopelistlen, encoding,
                           G_SlpdProperty.useScopes,
                           G_SlpdProperty.useScopesLen);
#else
         scopelistlen = 0;   /* pretend that we're unscoped */
#endif
         if (!errorcode)
            size += scopelistlen;
      }
   }
   else
   {
      /* don't add these */
      urllen = scopelistlen = 0;
   }

   result = SLPBufferRealloc(result, size);
   if (result == 0)
   {
      /* TODO: out of memory, what should we do here! */
      errorcode = SLP_ERROR_INTERNAL_ERROR;
      goto FINISHED;
   }

   /*----------------*/
   /* Add the header */
   /*----------------*/

   /* version */
   *result->curpos++ = 1;

   /* function id */
   *result->curpos++ = SLP_FUNCT_DAADVERT;

   /* length */
   PutUINT16(&result->curpos, size);

   /* flags */
   /** @todo We have to handle monoling and all that stuff. */
   *result->curpos++ = (size > SLP_MAX_DATAGRAM_SIZE? 
         SLPv1_FLAG_OVERFLOW: 0);

   /* dialect */
   *result->curpos++ = 0;

   /* language code */
   if (G_SlpdProperty.locale)
   {
      memcpy(result->curpos, G_SlpdProperty.locale, 2);
      result->curpos += 2;
   }
   PutUINT16(&result->curpos, encoding);

   /* xid */
   PutUINT16(&result->curpos, xid);

   /*--------------------------*/
   /* Add rest of the DAAdvert */
   /*--------------------------*/

   /* error code */
   PutUINT16(&result->curpos, errorcode);

   /* url len */
   PutUINT16(&result->curpos, urllen);

   /* url */
   SLPv1ToEncoding((char *)result->curpos, &urllen, encoding, da_url, strlen(da_url));
   result->curpos += urllen;

   /* scope list len */
   PutUINT16(&result->curpos, scopelistlen);

   /* scope list */
#ifndef FAKE_UNSCOPED_DA
   SLPv1ToEncoding((char *)result->curpos, &scopelistlen, encoding,
         G_SlpdProperty.useScopes, G_SlpdProperty.useScopesLen);
#endif

   result->curpos += scopelistlen;

FINISHED: 

   *sendbuf = result;

   return errorcode;
}
#endif

/*=========================================================================*/
void SLPDKnownDAEcho(SLPMessage * msg, SLPBuffer buf)
/* Echo a srvreg message to a known DA                                     */
/*                                                                */
/* msg (IN) the SrvReg message descriptor                                  */
/*                                                                         */
/* buf (IN) the SrvReg message buffer to echo                              */
/*                                                                         */
/* Returns:  none                                                          */
/*=========================================================================*/
{
   SLPBuffer dup;
   SLPDatabaseHandle dh;
   SLPDatabaseEntry * entry;
   SLPDAAdvert * entrydaadvert;
   SLPDSocket * sock;
   const char * msgscope;
   size_t msgscopelen;

   /* Do not echo registrations if we are a DA unless they were made  */
   /* local through the API!                                          */
   if (G_SlpdProperty.isDA && !SLPNetIsLocal(&(msg->peer)))
      return;

   if (msg->header.functionid == SLP_FUNCT_SRVREG)
   {
      msgscope = msg->body.srvreg.scopelist;
      msgscopelen = msg->body.srvreg.scopelistlen;
   }
   else if (msg->header.functionid == SLP_FUNCT_SRVDEREG)
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
   if (dh)
   {
      /*-----------------------------------------------------*/
      /* Check to see if there is already an identical entry */
      /*-----------------------------------------------------*/
      while (1)
      {
         entry = SLPDatabaseEnum(dh);
         if (entry == NULL)
            break;

         /* entrydaadvert is the DAAdvert message from the database */
         entrydaadvert = &(entry->msg->body.daadvert);

         /* Send to all DAs that have matching scope */
         if (SLPIntersectStringList(msgscopelen, msgscope,
                  entrydaadvert->scopelistlen, entrydaadvert->scopelist))
         {
            /* Do not echo to ourselves if we are a DA*/
            if (G_SlpdProperty.isDA
                    && (SLPIntersectStringList(G_ifaceurlsLen, 
                                               G_ifaceurls, 
                                               entrydaadvert->urllen,
                                               entrydaadvert->url) > 0))
            {
               /* don't do anything because it makes no sense to echo */
               /* to myself                                           */
            }
            else
            {
               /*------------------------------------------*/
               /* Load the socket with the message to send */
               /*------------------------------------------*/
               sock = SLPDOutgoingConnect(0, &(entry->msg->peer));
               if (sock)
               {
                  dup = SLPBufferDup(buf);
                  if (dup)
                  {
                     SLPListLinkTail(&(sock->sendlist), (SLPListItem *) dup);
                     SLPDOutgoingDatagramWrite(sock, buf);
                  }
                  else
                     sock->state = SOCKET_CLOSE;
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
/*                                                                */
/* seconds (IN) number of seconds that expired since last call             */
/*                                                                         */
/* Returns:  none                                                          */
/*=========================================================================*/
{
   SLPDSocket * sock;

   /* Check to see if we should perform active DA detection */
   if (G_SlpdProperty.DAActiveDiscoveryInterval == 0)
      return;
   /* When activeDiscoveryXmits is < 0 then we should not xmit any more */
   if (G_SlpdProperty.activeDiscoveryXmits < 0)
      return ;

   if (G_SlpdProperty.nextActiveDiscovery <= 0)
   {
      if (G_SlpdProperty.activeDiscoveryXmits == 0)
      {
         if (G_SlpdProperty.DAActiveDiscoveryInterval == 1)
         {
            /* ensures xmit on first call */
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

      /*Send the datagram, either broadcast or multicast*/
      if((G_SlpdProperty.isBroadcastOnly == 1) && SLPNetIsIPV4())
      {
         struct sockaddr_in peeraddr;
         memset(&peeraddr, 0, sizeof(struct sockaddr_in));  /*Some platforms require sin_zero be 0*/
         peeraddr.sin_family = AF_INET;
         peeraddr.sin_addr.s_addr = htonl(SLP_BCAST_ADDRESS);
         sock = SLPDSocketCreateDatagram((struct sockaddr_storage*)&peeraddr, DATAGRAM_BROADCAST);
         if(sock)
         {
            MakeActiveDiscoveryRqst(1, &(sock->sendbuf));
            SLPDOutgoingDatagramWrite(sock, sock->sendbuf);
            SLPDSocketFree(sock);
         }
      }
      else
      {
         /* For each incoming socket, find the sockets that can send multicast
            and send the appropriate datagram.  The incoming list is used because 
            it already has ready-to-use multicast sending sockets allocated for every interface.*/
         sock = (SLPDSocket *)G_IncomingSocketList.head;
         while (sock)
         {
            if(sock->can_send_mcast)
            {
               struct sockaddr_storage mcastaddr;

               if(SLPNetIsIPV6() && (sock->localaddr.ss_family == AF_INET6))
               {
                  MakeActiveDiscoveryRqst(1, &(sock->sendbuf));

                  SLPNetSetAddr(&mcastaddr, AF_INET6, G_SlpdProperty.port, &in6addr_srvlocda_node);
                  SLPDOutgoingDatagramMcastWrite(sock, &mcastaddr, sock->sendbuf);
                  SLPNetSetAddr(&mcastaddr, AF_INET6, G_SlpdProperty.port, &in6addr_srvlocda_link);
                  SLPDOutgoingDatagramMcastWrite(sock, &mcastaddr, sock->sendbuf);
                  if (!IN6_IS_ADDR_LINKLOCAL(&(((struct sockaddr_in6 *) &sock->localaddr)->sin6_addr)))
                  {
                     SLPNetSetAddr(&mcastaddr, AF_INET6, G_SlpdProperty.port, &in6addr_srvlocda_site);
                     SLPDOutgoingDatagramMcastWrite(sock, &mcastaddr, sock->sendbuf);
                  }
               }
               else if((sock->localaddr.ss_family == AF_INET) && SLPNetIsIPV4()) 
               {
                  int tmpaddr = SLP_MCAST_ADDRESS;
                  MakeActiveDiscoveryRqst(1, &(sock->sendbuf));

                  SLPNetSetAddr(&mcastaddr, AF_INET, G_SlpdProperty.port, &tmpaddr);
                  SLPDOutgoingDatagramMcastWrite(sock, &mcastaddr, sock->sendbuf);
               }
            }

            sock = (SLPDSocket *) sock->listitem.next;
         }
      }
   }
   else
      G_SlpdProperty.nextActiveDiscovery = 
            G_SlpdProperty.nextActiveDiscovery - seconds;
}

/*=========================================================================*/
void SLPDKnownDAStaleDACheck(int seconds)
/* Check for stale DAs if properly configured                              */
/*                                                                         */
/* seconds (IN) number seconds that elapsed since the last call to this    */
/*              function                                                   */
/*                                                                         */
/* Returns:  none                                                          */
/*=========================================================================*/
{
   /* Check to see if we should perform stale DA detection */
   if (G_SlpdProperty.staleDACheckPeriod == 0)
      return;

   {
      SLPDatabaseHandle dh;
      SLPDatabaseEntry * entry;
      SLPDatabaseEntry * nextEntry;

      (void) seconds;  /*unused parameter warning*/

      dh = SLPDatabaseOpen(&G_SlpdKnownDAs);
      if (dh)
      {
         nextEntry = SLPDatabaseEnum(dh);
         while (1)
         {
            entry = nextEntry;
            if (entry == NULL)
               break;
            nextEntry = SLPDatabaseEnum(dh);
            if (!entry->entryvalue)
            {
               /* No DAAdvert received for this DA within the check period */
               SLPDLogDAAdvertisement("Removed - stale", entry);
               SLPDatabaseRemove(dh, entry);
            }
            else
               entry->entryvalue--;   /* Decrement the "time to stale" count */
         }
      }
      SLPDatabaseClose(dh);
   }
}

/*=========================================================================*/
void SLPDKnownDAPassiveDAAdvert(int seconds, int dadead)
/* Send passive daadvert messages if properly configured and running as    */
/* a DA                                                                    */
/*                                                                        */
/* seconds (IN) number seconds that elapsed since the last call to this    */
/*              function                                                   */
/*                                                                         */
/* dadead  (IN) nonzero if the DA is dead and a bootstamp of 0 should be   */
/*              sent                                                       */
/*                                                                         */
/* Returns:  none                                                          */
/*=========================================================================*/
{
   SLPDSocket * sock;

   /* SAs don't send passive DAAdverts */
   if (G_SlpdProperty.isDA == 0)
      return;

   /* Check to see if we should perform passive DA detection */
   if ((G_SlpdProperty.passiveDADetection == 0) &&
       (G_SlpdProperty.staleDACheckPeriod == 0))
      return;

   G_SlpdProperty.nextPassiveDAAdvert = G_SlpdProperty.nextPassiveDAAdvert - seconds;
   if (G_SlpdProperty.nextPassiveDAAdvert <= 0 || dadead)
   {
      G_SlpdProperty.nextPassiveDAAdvert += G_SlpdProperty.DAHeartBeat;

      /*Send the datagram, either broadcast or multicast*/
      if((G_SlpdProperty.isBroadcastOnly == 1) && SLPNetIsIPV4())
      {
         struct sockaddr_in peeraddr;
         memset(&peeraddr, 0, sizeof(struct sockaddr_in));  /*Some platforms require sin_zero be 0*/
         peeraddr.sin_family = AF_INET;
         peeraddr.sin_addr.s_addr = htonl(SLP_BCAST_ADDRESS);
         sock = SLPDSocketCreateDatagram((struct sockaddr_storage*)&peeraddr, DATAGRAM_BROADCAST);
         if(sock)
         {
            /* @todo sock doesn't have a localaddr, and some platforms send broadcasts on all
            network interfaces, so I'm not sure what to send -- I'll just pick a non-loopback
            interface for now, and perhaps later cycle through the list and send as all local
            addrs.  Note that the original broadcasting code had the same problem, so this
            probably wasn't used much.*/
            if(G_IncomingSocketList.tail)
            {
               struct sockaddr_storage* myaddr = &((SLPDSocket *)G_IncomingSocketList.tail)->localaddr;
               if (SLPDKnownDAGenerateMyDAAdvert(myaddr, 0, dadead, 1, 0, &(sock->sendbuf)) == 0)
                  SLPDOutgoingDatagramWrite(sock, sock->sendbuf);
#ifdef ENABLE_SLPv1
               if (SLPDKnownDAGenerateMyV1DAAdvert(myaddr, 0, SLP_CHAR_UTF8, 0, &(sock->sendbuf)) == 0)
                  SLPDOutgoingDatagramWrite(sock, sock->sendbuf);
#endif
            }
            SLPDSocketFree(sock);
         }
      }
      else
      {
         /*   For each incoming socket, find the sockets that can send multicast
            and send the appropriate datagram. */
         sock = (SLPDSocket *)G_IncomingSocketList.head;
         while (sock)
         {
            if(sock->can_send_mcast)
            {
               struct sockaddr_storage mcastaddr;

               if(SLPNetIsIPV6() && (sock->localaddr.ss_family == AF_INET6))
               {
                  if (SLPDKnownDAGenerateMyDAAdvert(&sock->localaddr, 0, dadead, 1, 0, &(sock->sendbuf)) == 0)
                  {
                     SLPNetSetAddr(&mcastaddr, AF_INET6, G_SlpdProperty.port, &in6addr_srvlocda_node);
                     SLPDOutgoingDatagramMcastWrite(sock, &mcastaddr, sock->sendbuf);
                     SLPNetSetAddr(&mcastaddr, AF_INET6, G_SlpdProperty.port, &in6addr_srvlocda_link);
                     SLPDOutgoingDatagramMcastWrite(sock, &mcastaddr, sock->sendbuf);
                     if (!IN6_IS_ADDR_LINKLOCAL(&(((struct sockaddr_in6 *) &sock->localaddr)->sin6_addr)))
                     {
                        SLPNetSetAddr(&mcastaddr, AF_INET6, G_SlpdProperty.port, &in6addr_srvlocda_site);
                        SLPDOutgoingDatagramMcastWrite(sock, &mcastaddr, sock->sendbuf);
                     }
                  }
               }
               else if((sock->localaddr.ss_family == AF_INET) && SLPNetIsIPV4()) 
               {
                  int tmpaddr = SLP_MCAST_ADDRESS;

                  if (SLPDKnownDAGenerateMyDAAdvert(&sock->localaddr, 0, dadead, 1, 0, &(sock->sendbuf)) == 0)
                  {
                        SLPNetSetAddr(&mcastaddr, AF_INET, G_SlpdProperty.port, &tmpaddr);
                        SLPDOutgoingDatagramMcastWrite(sock, &mcastaddr, sock->sendbuf);
                  }
#ifdef ENABLE_SLPv1
                  if (SLPDKnownDAGenerateMyV1DAAdvert(&sock->localaddr, 0,
                                    SLP_CHAR_UTF8, 0, &(sock->sendbuf)) == 0)
                  {
                        tmpaddr = SLPv1_DA_MCAST_ADDRESS;
                        SLPNetSetAddr(&mcastaddr, AF_INET, G_SlpdProperty.port, &tmpaddr);
                        SLPDOutgoingDatagramMcastWrite(sock, &mcastaddr, sock->sendbuf);
                  }
#endif
               }
            }

            sock = (SLPDSocket *) sock->listitem.next;
         }
      }
   }
}


/*=========================================================================*/
void SLPDKnownDAImmortalRefresh(int seconds)
/* Refresh all SLP_LIFETIME_MAXIMUM services                               */
/*                                                          */
/* seconds (IN) time in seconds since last call                            */
/*=========================================================================*/
{
   SLPDatabaseHandle dh;
   SLPDatabaseEntry * entry;
   SLPDAAdvert * entrydaadvert;

   G_KnownDATimeSinceLastRefresh += seconds;

   if (G_KnownDATimeSinceLastRefresh >= SLP_LIFETIME_MAXIMUM - seconds)
   {
      /* Refresh all SLP_LIFETIME_MAXIMUM registrations */
      dh = SLPDatabaseOpen(&G_SlpdKnownDAs);
      if (dh)
      {
         /*-----------------------------------------------------*/
         /* Check to see if there is already an identical entry */
         /*-----------------------------------------------------*/
         while (1)
         {
            entry = SLPDatabaseEnum(dh);
            if (entry == NULL)
               break;

            /* entrydaadvert is the DAAdvert message from the database */
            entrydaadvert = &(entry->msg->body.daadvert);

            /* Skip ourself, which we detect by matching the entry's url     */
            /* (ip address) against the list of our own interface addresses. */
            /* If there's no match, then we can go ahead                     */
            if(SLPIntersectStringList(G_ifaceurlsLen, G_ifaceurls, entrydaadvert->urllen, entrydaadvert->url) == 0)
               SLPDKnownDARegisterAll(entry->msg, 1);
         }   

         SLPDatabaseClose(dh);
      }

      G_KnownDATimeSinceLastRefresh = 0;
   }
}

/*=========================================================================*/
void SLPDKnownDADeRegisterWithAllDas(SLPMessage * msg, SLPBuffer buf)
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
   SLPBuffer sendbuf;

   if (msg->header.functionid == SLP_FUNCT_SRVREG)
   {
      if (MakeSrvderegFromSrvReg(msg, buf, &sendbuf) == 0)
      {
         SLPDKnownDAEcho(msg, sendbuf);
         SLPBufferFree(sendbuf);
      }
   }
   else if (msg->header.functionid == SLP_FUNCT_SRVDEREG)
   {
      /* Simply echo the message through as is */
      SLPDKnownDAEcho(msg, buf);
   }
}

/*=========================================================================*/
void SLPDKnownDARegisterWithAllDas(SLPMessage * msg, SLPBuffer buf)
/* Register the registration described by the specified message with all   */
/* known DAs                                                               */
/*                                                                         */
/* msg (IN) A message descriptor for a SrvReg or SrvDereg message to       */
/*          deregister                                                     */
/*                                                                         */
/* buf (IN) Message buffer associated with msg                             */
/*                                                                         */
/* Returns: None                                                           */
/*=========================================================================*/
{
   if (msg->header.functionid == SLP_FUNCT_SRVDEREG)
   {
      /* Simply echo the message through as is */
      SLPDKnownDAEcho(msg, buf);
   }
}

#ifdef DEBUG
/*=========================================================================*/
void SLPDKnownDADump(void)
/*=========================================================================*/
{
   SLPMessage * msg;
   SLPBuffer buf;
   void * eh;

   eh = SLPDKnownDAEnumStart();
   if (eh)
   {
      SLPDLog("========================================================================\n");
      SLPDLog("Dumping KnownDAs \n");
      SLPDLog("========================================================================\n");
      while (SLPDKnownDAEnum(eh, &msg, &buf))
      {
         SLPDLogMessageInternals(msg);
         SLPDLog("\n");
      }

      SLPDKnownDAEnumEnd(eh);
   }
}
#endif

/*=========================================================================*/
