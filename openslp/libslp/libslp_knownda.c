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

/** Tracks Known DA's.
 *
 * Tracks known DA's for the user agent.
 *
 * @file       libslp_knownda.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    LibSLPCode
 */

#include "slp.h"
#include "libslp.h"
#include "slp_dhcp.h"
#include "slp_net.h"
#include "slp_parse.h"
#include "slp_network.h"
#include "slp_database.h"
#include "slp_compare.h"
#include "slp_xmalloc.h"
#include "slp_property.h"

/** The cache DAAdvert messages from known DAs. */
static SLPDatabase G_KnownDACache = {0, 0, 0};

/** Cached known scope list length */
static size_t G_KnownDAScopesLen = 0;

/** Cached known scope list */
static char * G_KnownDAScopes = 0;

/** The time of the last Multicast for known DAs */
static time_t G_KnownDALastCacheRefresh = 0;

/** Locate a known DA matching the desired scope list and SPI.
 *
 * Searches the known DA list in the database for a DA that matches the
 * specified scope list and security parameter index (SPI) value. Returns
 * the network address for the first DA to match the specified criteria.
 *
 * @param[in] scopelist - The list of scopes whose DA's should be found. All
 *    DA's that support a proper subset of this scope list will be returned.
 * @param[in] scopelistlen - The length of @p scopelist.
 * @param[in] spistr - The Security Parameter Index value to use.
 * @param[in] spistrlen - The length of @p spistr.
 * @param[out] daaddr - The address of a DA matching the specified search
 *    criteria.
 * @param[in] daaddrsz - The length in bytes of @p daaddr.
 *
 * @return A Bookean value; Zero if DA cannot be found, non-zero on success.
 *
 * @internal
 */
static SLPBoolean KnownDAListFind(size_t scopelistlen, const char * scopelist, 
      size_t spistrlen, const char * spistr, void * daaddr, size_t daaddrsz)
{
   int result;
   SLPDatabaseHandle dh;
   SLPDatabaseEntry * entry;

#ifndef ENABLE_SLPv2_SECURITY
   (void)spistr;
   (void)spistrlen;
#endif

   result = SLP_FALSE;
   if ((dh = SLPDatabaseOpen(&G_KnownDACache)) != 0)
   {
      /* Check to see if there a matching entry, and then check scopes. */
      while ((entry = SLPDatabaseEnum(dh)) != 0)
      {
         if (SLPSubsetStringList(entry->msg->body.daadvert.scopelistlen,
               entry->msg->body.daadvert.scopelist, scopelistlen, 
               scopelist) != 0)
         {
#if defined(ENABLE_SLPv2_SECURITY)
            if (SLPCompareString(entry->msg->body.daadvert.spilistlen,
                  entry->msg->body.daadvert.spilist, spistrlen, spistr) == 0)
#endif
            {
               memcpy(daaddr, &entry->msg->peer, daaddrsz);
               result = SLP_TRUE;
               break;
            }
         }
      }
      SLPDatabaseClose(dh);
   }
   return result;
}

/** Add an entry to the KnownDA cache.
 *
 * @param[in] msg - The message containing the DA advertisement.
 * @param[in] buf - The buffer associated with @p msg.
 *
 * @return Zero on success, or a non-zero value on failure.
 *
 * @internal
 */
static int KnownDAAdd(SLPMessage * msg, SLPBuffer buf)
{
   int result = 0;
   SLPDatabaseHandle dh = SLPDatabaseOpen(&G_KnownDACache);

   if (dh)
   {
      SLPDatabaseEntry * entry;
      SLPDAAdvert * daadvert = &msg->body.daadvert;

      /* Check to see if there is already an identical entry. */
      while (1)
      {
         SLPDAAdvert * entrydaadvert;

         entry = SLPDatabaseEnum(dh);
         if (!entry) 
            break;

         /* entrydaadvert is the DAAdvert message from the database. */
         entrydaadvert = &entry->msg->body.daadvert;

         /* Assume DAs are identical if their URLs match. */
         if (!SLPCompareString(entrydaadvert->urllen, entrydaadvert->url,
               daadvert->urllen, daadvert->url))
         {
            SLPDatabaseRemove(dh, entry);
            break;
         }
      }

      /* Create and link in a new entry. */
      entry = SLPDatabaseEntryCreate(msg, buf);
      if (entry)
         SLPDatabaseAdd(dh, entry);
      else
         result = SLP_MEMORY_ALLOC_FAILED;
      SLPDatabaseClose(dh);
   }
   return result;
}

/** Callback for DA discovery algorithm.
 *
 * @param[in] errorcode - The error code returned by discovery.
 * @param[in] peerinfo - The address of the remote peer.
 * @param[in] replybuf - The reply information from the peer.
 * @param[in] cookie - Pass through data from the original caller.
 *
 * @return A boolean value; True on success, False to stop caller from
 *    calling this routine again.
 *
 * @remarks The @p cookie parameter is the address of a count that is
 *    either updated to reflect a new entry added, or not in case of error.
 *
 * @internal
 */
static SLPBoolean KnownDADiscoveryCallback(SLPError errorcode,
      void * peerinfo, SLPBuffer replybuf, void * cookie)
{
   SLPBuffer dupbuf;
   SLPMessage * replymsg;
   SLPBoolean result = SLP_TRUE; /* Default is to continue. */

   if (errorcode)                /* Bad response, but do call again. */
      return SLP_TRUE;

   /* Allocate duplicate buffer and message object. */
   dupbuf = SLPBufferDup(replybuf);
   replymsg = SLPMessageAlloc();

   if (dupbuf != 0 && replymsg != 0
         && SLPMessageParseBuffer(peerinfo, 0, dupbuf, replymsg) == 0 
         && replymsg->header.functionid == SLP_FUNCT_DAADVERT)
   {
      if (replymsg->body.daadvert.errorcode == 0)
      {
         SLPParsedSrvUrl * srvurl;

         if (SLPParseSrvUrl(replymsg->body.daadvert.urllen, 
                            replymsg->body.daadvert.url, &srvurl) == 0)
         {
            int retval = -1;

            /* Should call inet_pton with the same address family
             * as was found in the DA url. 
             */
            if (replymsg->peer.ss_family == AF_INET && SLPNetIsIPV4())
            {
               memset(&((struct sockaddr_in *)&replymsg->peer)->sin_addr, 0, 
                     sizeof(struct in_addr));
               retval = inet_pton(replymsg->peer.ss_family, srvurl->host, 
                     &((struct sockaddr_in *)&replymsg->peer)->sin_addr);
            }
            else if (replymsg->peer.ss_family == AF_INET6 && SLPNetIsIPV6()) 
            {
               memset(&((struct sockaddr_in6 *)&replymsg->peer)->sin6_addr, 0, 
                     sizeof(struct in6_addr));
               retval = inet_pton(replymsg->peer.ss_family, srvurl->host, 
                     &((struct sockaddr_in6 *)&replymsg->peer)->sin6_addr);
            }
            if (retval == 0)
            {
               struct addrinfo * he;
               struct addrinfo hints;

               hints.ai_family = replymsg->peer.ss_family;
               getaddrinfo(srvurl->host, 0, &hints, &he);
               if (he)
               {
                  /* Reset the peer to the one in the URL. */
                  if (replymsg->peer.ss_family == AF_INET && SLPNetIsIPV4())
                     memcpy(&((struct sockaddr_in *)&replymsg->peer)->sin_addr, 
                           &((struct sockaddr_in *)he->ai_addr)->sin_addr, 
                           sizeof(struct in_addr));
                  else if (replymsg->peer.ss_family == AF_INET6 && SLPNetIsIPV6())
                     memcpy(&((struct sockaddr_in6 *)&replymsg->peer)->sin6_addr, 
                           &((struct sockaddr_in6 *)he->ai_addr)->sin6_addr, 
                           sizeof(struct in6_addr));
                  retval = 1;
                  freeaddrinfo(he);
               }
            }
            xfree(srvurl);

            if (retval > 0)
            {
               if (KnownDAAdd(replymsg, dupbuf) == 0)
               {
                  /* Increment number of entries processed so far. */
                  (*(int *)cookie)++;
                  return SLP_TRUE;
               /* return (replymsg->header.flags & SLP_FLAG_MCAST)? 
                        SLP_FALSE: SLP_TRUE; */
               }
            }
         }
      }
      else if (replymsg->body.daadvert.errorcode == SLP_ERROR_INTERNAL_ERROR)
         result = SLP_FALSE; /* "end of stream" for loopback IPC. */
   }
   SLPMessageFree(replymsg);
   SLPBufferFree(dupbuf);
   return result;
}

/** Format a Service Request for DA services and send on a socket.
 *
 * @param[in] sock - A socket connected to a server that can respond to 
 *    A DA SrvRequest.
 * @param[in] peeraddr - The address connected to on @p sock.
 * @param[in] scopelistlen - The length of @p scopelist in bytes.
 * @param[in] scopelist - The DA's returned must support these scopes.
 * @param[in] handle - The OpenSLP handle on which this request was made.
 *
 * @return The number of *new* DAEntries found.
 *
 * @internal
 */
static int KnownDADiscoveryRqstRply(sockfd_t sock, 
      void * peeraddr, size_t scopelistlen,
      const char * scopelist, SLPHandleInfo * handle)
{
   uint8_t * buf;
   uint8_t * cur;
   int result = 0;

/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   length of <service-type>    |    <service-type> String      \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    length of <scope-list>     |     <scope-list> String       \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  length of predicate string   |  Service Request <predicate>  \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  length of <SLP SPI> string   |       <SLP SPI> String        \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

#define SLP_DA_SERVICE_TYPE_LEN (sizeof(SLP_DA_SERVICE_TYPE) - 1)

   /** @todo Make sure that we don't exceed the MTU. */
   buf = cur = xmalloc(
         + 2 + SLP_DA_SERVICE_TYPE_LEN
         + 2 + scopelistlen
         + 2 + 0
         + 2 + 0);
   if (buf == 0)
      return 0;

   /* <service-type> */
   PutUINT16(&cur, SLP_DA_SERVICE_TYPE_LEN);
   memcpy(cur, SLP_DA_SERVICE_TYPE, SLP_DA_SERVICE_TYPE_LEN);
   cur += SLP_DA_SERVICE_TYPE_LEN;

   /* <scope-list> */
   PutUINT16(&cur, scopelistlen);
   memcpy(cur, scopelist, scopelistlen);
   cur += scopelistlen;

   /* <predicate> */
   PutUINT16(&cur, 0);

   /* <SLP SPI> */
   PutUINT16(&cur, 0);

   if (sock == SLP_INVALID_SOCKET)
      NetworkMcastRqstRply(handle, buf, SLP_FUNCT_DASRVRQST, 
            cur - buf, KnownDADiscoveryCallback, &result);
   else
      NetworkRqstRply(sock, peeraddr, "en", 0, buf, SLP_FUNCT_DASRVRQST,
            cur - buf, KnownDADiscoveryCallback, &result);

   xfree(buf);
   return result;
}

/** Locates DAs via multicast convergence.
 *
 * @param[in] scopelistlen - The length of @p scopelist. 
 * @param[in] scopelist - A list of scopes that must be supported.
 * @param[in] handle - The SLP handle associated with this request.
 *
 * @return The number of *new* DAs found.
 *
 * @internal
 */
static int KnownDADiscoverFromMulticast(size_t scopelistlen, 
      const char * scopelist, SLPHandleInfo * handle)
{
   int result = 0;

   if (SLPPropertyAsBoolean("net.slp.activeDADetection") 
         && SLPPropertyAsInteger("net.slp.DADiscoveryMaximumWait"))
      result = KnownDADiscoveryRqstRply(SLP_INVALID_SOCKET, 0, scopelistlen,
            scopelist, handle);

   return result;
}

/** Locates DAs via DHCP.
 *
 * @param[in] handle - The SLP handle associated with this request.
 *
 * @return The number of *new* DAs found via DHCP.
 *
 * @internal
 */
static int KnownDADiscoverFromDHCP(SLPHandleInfo * handle)
{
   int count = 0;
   size_t scopelistlen;
   DHCPContext ctx;
   uint8_t * alp;
   struct sockaddr_storage peeraddr;
   unsigned char dhcpOpts[] = {TAG_SLP_SCOPE, TAG_SLP_DA};

   /* Only do DHCP discovery if IPv4 is enabled. */
   if (!SLPNetIsIPV4())
      return 0;

   *ctx.scopelist = 0;
   ctx.addrlistlen = 0;

   DHCPGetOptionInfo(dhcpOpts, sizeof(dhcpOpts), DHCPParseSLPTags, &ctx);

   if (!*ctx.scopelist)
   {
      const char * useScopes = SLPPropertyGet("net.slp.useScopes", 0, 0);
      if (useScopes)
         strcpy(ctx.scopelist, useScopes);
   }
   scopelistlen = strlen(ctx.scopelist);

   SLPNetSetAddr(&peeraddr, AF_INET, SLPPropertyAsInteger("net.slp.port"), 0);

   alp = ctx.addrlist;

   while (ctx.addrlistlen >= 4)
   {
      memcpy(&((struct sockaddr_in *)&peeraddr)->sin_addr.s_addr, alp, 4);
      if (((struct sockaddr_in *)&peeraddr)->sin_addr.s_addr)
      {
         sockfd_t sockfd;
         if ((sockfd = SLPNetworkCreateDatagram(peeraddr.ss_family)) != SLP_INVALID_SOCKET)
         {
            count = KnownDADiscoveryRqstRply(sockfd, &peeraddr, 
                  scopelistlen, ctx.scopelist, handle);
            closesocket(sockfd);
            if (scopelistlen && count)
               break;   /* stop after the first set found */
         }
      }
      ctx.addrlistlen -= 4;
      alp += 4;
   }
   return count;
}

/** Locates DAs from the property list of DA hostnames.
 *
 * @param[in] scopelistlen - The length of @p scopelist.
 * @param[in] scopelist - The list of scopes that must be supported.
 * @param[in] handle - The SLP handle associated with this request.
 *
 * @return The number of *new* DAs found.
 *
 * @internal
 */
static int KnownDADiscoverFromProperties(size_t scopelistlen,
      const char * scopelist, SLPHandleInfo * handle)
{
   char * temp;
   char * slider1;
   char * slider2;
   int result = 0;

   slider1 = slider2 = temp = SLPPropertyXDup("net.slp.DAAddresses");
   if (temp)
   {
      char * tempend = temp + strlen(temp);
      while (slider1 != tempend)
      {
         struct sockaddr_storage peeraddr;

         while (*slider2 && *slider2 != ',') 
            slider2++;
         *slider2 = 0;

         if (SLPNetResolveHostToAddr(slider1, &peeraddr) == 0)
         {
            sockfd_t sockfd;

            SLPNetSetParams(&peeraddr, peeraddr.ss_family, SLPPropertyAsInteger("net.slp.port"));
            sockfd = SLPNetworkCreateDatagram(peeraddr.ss_family);
            if (sockfd != SLP_INVALID_SOCKET)
            {
               result = KnownDADiscoveryRqstRply(sockfd, &peeraddr,
                     scopelistlen, scopelist, handle);
               closesocket(sockfd);
               if (scopelistlen && result)
                  break; /* return if we found at least one DA */
            }
         }
         slider1 = slider2;
         slider2++;
      }
      xfree(temp);
   }
   return result;
}

/** Asks slpd if it knows about a DA.
 *
 * @param[in] handle - The SLP handle associated with this request.
 *
 * @return The number of *new* DAs found.
 *
 * @internal
 */
static int KnownDADiscoverFromIPC(SLPHandleInfo * handle)
{
   int result = 0;
   struct sockaddr_storage peeraddr;
   
   sockfd_t sockfd = NetworkConnectToSlpd(&peeraddr);
   
   if (sockfd != SLP_INVALID_SOCKET)
   {
      result = KnownDADiscoveryRqstRply(sockfd, &peeraddr, 0, "", handle);
      closesocket(sockfd);
   }
   return result;
}

/** Asks slpd if it knows about a DA.
 *
 * @param[in] scopelistlen - The length of @p scopelist.
 * @param[in] scopelist - The list of scopes that must be supported.
 * @param[in] spistrlen - The length of @p spistr.
 * @param[in] spistr - The Security Provider Index.
 * @param[in] daaddr - The DA address.
 * @param[in] handle - The SLP handle associated with this request.
 *
 * @return Non-zero on success, zero if DA can not be found.
 *
 * @internal
 */
static SLPBoolean KnownDAFromCache(size_t scopelistlen, 
      const char * scopelist, size_t spistrlen, const char * spistr, 
      void * daaddr, SLPHandleInfo * handle)
{
   if (KnownDAListFind(scopelistlen, scopelist, spistrlen, spistr, 
         daaddr, sizeof(struct sockaddr_storage)) == SLP_FALSE)
   {
      time_t curtime = time(&curtime);
      if (G_KnownDALastCacheRefresh == 0 || curtime 
            - G_KnownDALastCacheRefresh > MINIMUM_DISCOVERY_INTERVAL)
      {
         G_KnownDALastCacheRefresh = curtime;

         /* Discover DAs. */
         if (KnownDADiscoverFromIPC(handle) == 0
               && KnownDADiscoverFromProperties(
                     scopelistlen, scopelist, handle) == 0
               && KnownDADiscoverFromDHCP(handle) == 0) 
            KnownDADiscoverFromMulticast(scopelistlen, scopelist, handle);
      }
      return KnownDAListFind(scopelistlen, scopelist, spistrlen, spistr, 
            daaddr, sizeof(struct sockaddr_storage));
   }
   return SLP_TRUE; 
}


/** Get a connected socket to a DA that supports the specified scope.
 *
 * @param[in] handle - The SLP handle associated with this request.
 * @param[in] scopelistlen - The length of @p scopelist.
 * @param[in] scopelist - The scopes the DA must support.
 * @param[out] peeraddr - The peer to which we connected.
 *
 * @return A valid socket file descriptor or SLP_INVALID_SOCKET if 
 *    no DA supportting the requested scopelist is found.
 */
sockfd_t KnownDAConnect(SLPHandleInfo * handle, size_t scopelistlen, 
      const char * scopelist, void * peeraddr)
{
   sockfd_t sock = SLP_INVALID_SOCKET;
   size_t spistrlen = 0;
   char * spistr = 0;

#ifdef ENABLE_SLPv2_SECURITY
   if (SLPPropertyAsBoolean("net.slp.securityEnabled"))
      SLPSpiGetDefaultSPI(handle->hspi, SLPSPI_KEY_TYPE_PUBLIC, 
            &spistrlen, &spistr);
#endif

   while (1)
   {
      struct sockaddr * addr = peeraddr;
      memset(peeraddr, 0, sizeof(struct sockaddr_storage));

      if (KnownDAFromCache(scopelistlen, scopelist, spistrlen, spistr,
            peeraddr, handle) == SLP_FALSE)
         break;

      if ((addr->sa_family == AF_INET6 && SLPNetIsIPV6())
            || (addr->sa_family == AF_INET && SLPNetIsIPV4()))
      {
         SLPNetSetPort(peeraddr, SLPPropertyAsInteger("net.slp.port"));
         sock = SLPNetworkCreateDatagram(addr->sa_family);
         /*Now test if the DA will actually respond*/
         if((sock != SLP_INVALID_SOCKET) &&
            (0 < KnownDADiscoveryRqstRply(sock, peeraddr, scopelistlen, scopelist, handle)))
            break;
      }

      KnownDABadDA(peeraddr);
   }
   xfree(spistr);
   return sock;
}

/** Mark a KnownDA as a Bad DA.
 *
 * @param[in] daaddr - The address of the bad DA.
 */
void KnownDABadDA(void * daaddr)
{
   SLPDatabaseHandle dh = SLPDatabaseOpen(&G_KnownDACache);
   if (dh)
   {
      /* Check to find the requested entry. */
      while (1)
      {
         SLPDatabaseEntry * entry = SLPDatabaseEnum(dh);
         if (!entry)
            break;

         /* Assume DAs are identical if their in_addrs match. */
         if (SLPNetCompareAddrs(daaddr, &entry->msg->peer) == 0)
         {
            SLPDatabaseRemove(dh, entry);
            break;            
         }
      }
      SLPDatabaseClose(dh);
   }
}

/** Gets a list of scopes from the known DA list.
 *
 * @param[out] scopelistlen - The address of storage for the length of the 
 *    returned scope list in @p scopelist.
 * @param[out] scopelist - The address of storage for a scope list ptr.
 * @param[in] handle - The SLP session handle associated with this request.
 *
 * @return Zero on success, non-zero on memory allocation failure.
 */
int KnownDAGetScopes(size_t * scopelistlen,
      char ** scopelist, SLPHandleInfo * handle)
{
   size_t newlen;
   SLPDatabaseHandle dh;
   SLPDatabaseEntry * entry;
   char const * useScopes;

   /* Discover all DAs. */
   if (KnownDADiscoverFromIPC(handle) == 0)
   {
      KnownDADiscoverFromDHCP(handle);
      KnownDADiscoverFromProperties(0,"", handle);
      KnownDADiscoverFromMulticast(0,"", handle);
   }

   /* Enumerate all the knownda entries and generate a scopelist. */
   dh = SLPDatabaseOpen(&G_KnownDACache);
   if (dh)
   {
      /* Check to find the requested entry. */
      while (1)
      {
         entry = SLPDatabaseEnum(dh);
         if (!entry) 
            break;

         newlen = G_KnownDAScopesLen;
         while (SLPUnionStringList(G_KnownDAScopesLen, G_KnownDAScopes,
               entry->msg->body.daadvert.scopelistlen,
               entry->msg->body.daadvert.scopelist, &newlen,
               G_KnownDAScopes) < 0)
         {
            G_KnownDAScopes = xrealloc(G_KnownDAScopes, newlen);
            if (!G_KnownDAScopes)
            {
               G_KnownDAScopesLen = 0;
               break;
            }
         }
         G_KnownDAScopesLen = newlen;
      }
      SLPDatabaseClose(dh);
   }

   /* Explicitly add in the useScopes property */
   newlen = G_KnownDAScopesLen;
   useScopes = SLPPropertyGet("net.slp.useScopes", 0, 0);
   while (SLPUnionStringList(G_KnownDAScopesLen, G_KnownDAScopes, 
         strlen(useScopes), useScopes, &newlen, G_KnownDAScopes) < 0)
   {
      G_KnownDAScopes = xrealloc(G_KnownDAScopes, newlen);
      if (!G_KnownDAScopes)
      {
         G_KnownDAScopesLen = 0;
         break;
      }
   }
   G_KnownDAScopesLen = newlen;

   if (G_KnownDAScopesLen)
   {
      *scopelist = xmalloc(G_KnownDAScopesLen + 1);
      if (*scopelist == 0)
         return -1;
      memcpy(*scopelist, G_KnownDAScopes, G_KnownDAScopesLen);
      (*scopelist)[G_KnownDAScopesLen] = 0; 
      *scopelistlen = G_KnownDAScopesLen;
   }
   else
   {
      *scopelist = xstrdup("");
      if (*scopelist == 0)
         return -1;
      *scopelistlen = 0; 
   }
   return 0;
}

/** Process a SrvRqst for service:directory-agent.
 *
 * @param[in] handle - The SLP session handle associated with this request.
 */
void KnownDAProcessSrvRqst(SLPHandleInfo * handle)
{
   SLPDatabaseHandle dh;

   /* Discover all DAs. */
   if (KnownDADiscoverFromIPC(handle) == 0)
   {
      KnownDADiscoverFromDHCP(handle);
      KnownDADiscoverFromProperties(0,"", handle);
      KnownDADiscoverFromMulticast(0,"", handle);
   }

   /* Enumerate through knownDA database. */
   dh = SLPDatabaseOpen(&G_KnownDACache);
   if (dh)
   {
      /* Check to see if there a matching entry. */
      while (1)
      {
         SLPBoolean cb_result;
         SLPDatabaseEntry * entry = SLPDatabaseEnum(dh);
         if (!entry)
            break;

         /* Call the SrvURLCallback. */
         cb_result = handle->params.findsrvs.callback(handle,
               entry->msg->body.daadvert.url, SLP_LIFETIME_MAXIMUM, 
               SLP_OK, handle->params.findsrvs.cookie);

         /* Does the caller want more? */
         if (cb_result == SLP_FALSE)
            break;
      }
      SLPDatabaseClose(dh);
   }

   /* Make SLP_LAST_CALL. */
   handle->params.findsrvs.callback(handle, 0, 0, 
         SLP_LAST_CALL, handle->params.findsrvs.cookie);
}

/** Frees all (cached) resources associated with known DAs.
 */
void KnownDAFreeAll(void)
{
   SLPDatabaseHandle dh = SLPDatabaseOpen(&G_KnownDACache);
   if (dh)
   {
      while (1)
      {
         SLPDatabaseEntry * entry = SLPDatabaseEnum(dh);
         if (!entry)
            break;
         SLPDatabaseRemove(dh,entry);
      }
      SLPDatabaseClose(dh);
   }
   xfree(G_KnownDAScopes);
   G_KnownDAScopesLen = 0;
   G_KnownDALastCacheRefresh = 0;
}

/*=========================================================================*/
