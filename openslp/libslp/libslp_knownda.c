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

#ifndef _WIN32
# define closesocket close
#endif

#include <time.h>

SLPDatabase G_KnownDACache = {0, 0, 0};
/*!< The cache DAAdvert messages from known DAs. 
 */

int G_KnownDAScopesLen = 0;
/*!< Cached known scope list length.
 */

char * G_KnownDAScopes = 0;
/*!< Cached known scope list.
 */

time_t G_KnownDALastCacheRefresh = 0;
/*!< The time of the last Multicast for known DAs.
 */

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
 *
 * @return A Bookean value; Zero if DA cannot be found, non-zero on success.
 *
 * @internal
 */
SLPBoolean KnownDAListFind(int scopelistlen,
      const char* scopelist,
      int spistrlen,
      const char* spistr,
      struct sockaddr_storage* daaddr)
{
   SLPDatabaseHandle   dh;
   SLPDatabaseEntry*   entry;
   int result = SLP_FALSE;

   dh = SLPDatabaseOpen(&G_KnownDACache);
   if (dh)
   {
      /*----------------------------------------*/
      /* Check to see if there a matching entry */
      /*----------------------------------------*/
      while (1)
      {
         entry = SLPDatabaseEnum(dh);
         if (entry == NULL) break;

         /* Check scopes */
         if (SLPSubsetStringList(entry->msg->body.daadvert.scopelistlen,
               entry->msg->body.daadvert.scopelist,
               scopelistlen,
               scopelist))
         {
#ifdef ENABLE_SLPv2_SECURITY
            if (SLPCompareString(entry->msg->body.daadvert.spilistlen,
                  entry->msg->body.daadvert.spilist,
                  spistrlen,
                  spistr) == 0)
#endif
            {

               memcpy(daaddr, 
                     &(entry->msg->peer),
                     sizeof(struct sockaddr_storage));

               result = SLP_TRUE;
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
int KnownDAAdd(SLPMessage msg, SLPBuffer buf)
{
   SLPDatabaseHandle   dh;
   SLPDatabaseEntry*   entry;
   SLPDAAdvert*        entrydaadvert;
   SLPDAAdvert*        daadvert;
   int                 result;

   result = 0;

   dh = SLPDatabaseOpen(&G_KnownDACache);
   if (dh)
   {
      /* daadvert is the DAAdvert message being added */
      daadvert = &(msg->body.daadvert);

      /*-----------------------------------------------------*/
      /* Check to see if there is already an identical entry */
      /*-----------------------------------------------------*/
      while (1)
      {
         entry = SLPDatabaseEnum(dh);
         if (entry == NULL) break;

         /* entrydaadvert is the DAAdvert message from the database */
         entrydaadvert = &(entry->msg->body.daadvert);

         /* Assume DAs are identical if their URLs match */
         if (SLPCompareString(entrydaadvert->urllen,
               entrydaadvert->url,
               daadvert->urllen,
               daadvert->url) == 0)
         {
            SLPDatabaseRemove(dh,entry);
            break;
         }
      }

      /* Create and link in a new entry */
      entry = SLPDatabaseEntryCreate(msg,buf);
      if (entry)
      {
         SLPDatabaseAdd(dh, entry);
      }
      else
      {
         result = SLP_MEMORY_ALLOC_FAILED;
      }

      SLPDatabaseClose(dh);
   }

   return result;
}

/** Callback for DA discovery algorithm.
 *
 * @param[in] errorcode - The error code returned by discovery.
 * @param[in] peerinfo - The address of the remote peer.
 * @param[in] rplybuf - The reply information from the peer.
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
SLPBoolean KnownDADiscoveryCallback(SLPError errorcode,
      struct sockaddr_storage* peerinfo,
      SLPBuffer rplybuf, 
      void* cookie)
{
   SLPMessage      replymsg;
   SLPBuffer       dupbuf;
   struct addrinfo* he;
   struct addrinfo hints;
   SLPSrvURL*      srvurl;
   int*            count;
   SLPBoolean      result = SLP_TRUE;

   count = (int*)cookie;

   if (errorcode == 0)
   {
      dupbuf = SLPBufferDup(rplybuf);
      if (dupbuf)
      {
         replymsg = SLPMessageAlloc();
         if (replymsg)
         {
            if (SLPMessageParseBuffer(peerinfo,NULL,dupbuf,replymsg) == 0 &&
                  replymsg->header.functionid == SLP_FUNCT_DAADVERT)
            {
               if (replymsg->body.daadvert.errorcode == 0)
               {
                  /* TRICKY: NULL terminate the DA url */
                  ((char*)(replymsg->body.daadvert.url))[replymsg->body.daadvert.urllen] = 0;
                  if (SLPParseSrvURL(replymsg->body.daadvert.url, &srvurl) == 0)
                  {
                     int retval = -1;
                     /* Should call inet_pton with the same address family as was found in the
                     DA url. */
                     if (replymsg->peer.ss_family == AF_INET && SLPNetIsIPV4())
                     {
                        memset(&((struct sockaddr_in *)&replymsg->peer)->sin_addr, 0, sizeof(struct in_addr));
                        retval = inet_pton(replymsg->peer.ss_family, srvurl->s_pcHost, &((struct sockaddr_in *)&replymsg->peer)->sin_addr);
                     }
                     else if (replymsg->peer.ss_family == AF_INET6 && SLPNetIsIPV6())
                     {
                        memset(&((struct sockaddr_in6 *)&replymsg->peer)->sin6_addr, 0, sizeof(struct in6_addr));
                        retval = inet_pton(replymsg->peer.ss_family, srvurl->s_pcHost, &((struct sockaddr_in6 *)&replymsg->peer)->sin6_addr);
                     }

                     if (retval == 0)
                     {
                        hints.ai_family = replymsg->peer.ss_family;
                        getaddrinfo(srvurl->s_pcHost, NULL, &hints, &he);
                        if (he)
                        {
                           /* Reset the peer to the one in the URL */
                           if (replymsg->peer.ss_family == AF_INET && SLPNetIsIPV4())
                              memcpy(&((struct sockaddr_in *)&replymsg->peer)->sin_addr, &((struct sockaddr_in *)he->ai_addr)->sin_addr, sizeof(struct in_addr));
                           else if (replymsg->peer.ss_family == AF_INET6 && SLPNetIsIPV6())
                              memcpy(&((struct sockaddr_in6 *)&replymsg->peer)->sin6_addr, &((struct sockaddr_in6 *)he->ai_addr)->sin6_addr, sizeof(struct in6_addr));
                           retval = 1;
                           freeaddrinfo(he);
                        }
                     }

                     SLPFree(srvurl);

                     if (retval > 0)
                     {
                        (*count) += 1;

                        KnownDAAdd(replymsg,dupbuf);
                        if (replymsg->header.flags & SLP_FLAG_MCAST)
                        {
                           return SLP_FALSE;
                        }

                        return SLP_TRUE;
                     }
                  }
               }
               else if (replymsg->body.daadvert.errorcode == SLP_ERROR_INTERNAL_ERROR)
               {
                  /* SLP_ERROR_INTERNAL_ERROR is a "end of stream" */
                  /* marker for looppack IPC                       */
                  result = SLP_FALSE;
               }
            }

            SLPMessageFree(replymsg);
         }

         SLPBufferFree(dupbuf);
      }
   }

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
int KnownDADiscoveryRqstRply(int sock, 
      struct sockaddr_storage* peeraddr,
      int scopelistlen,
#ifndef MI_NOT_SUPPORTED
      const char* scopelist,
      PSLPHandleInfo handle)
#else
const char* scopelist)
#endif /* MI_NOT_SUPPORTED */
{
   char*   buf;
   char*   curpos;
   int     bufsize;
   int     result = 0;

   /*-------------------------------------------------------------------*/
   /* determine the size of the fixed portion of the SRVRQST            */
   /*-------------------------------------------------------------------*/
   bufsize  = 31; /*  2 bytes for the srvtype length */
   /* 23 bytes for "service:directory-agent" srvtype */
   /*  2 bytes for scopelistlen */
   /*  2 bytes for predicatelen */
   /*  2 bytes for sprstrlen */
   bufsize += scopelistlen;

   /* TODO: make sure that we don't exceed the MTU */
   buf = curpos = (char*)xmalloc(bufsize);
   if (buf == 0)
   {
      return 0;
   }
   memset(buf,0,bufsize);

   /*------------------------------------------------------------*/
   /* Build a buffer containing the fixed portion of the SRVRQST */
   /*------------------------------------------------------------*/
   /* service type */
   ToUINT16(curpos,23);
   curpos = curpos + 2;
   /* 23 is the length of SLP_DA_SERVICE_TYPE */
   memcpy(curpos,SLP_DA_SERVICE_TYPE,23);
   curpos += 23;
   /* scope list */
   ToUINT16(curpos,scopelistlen);
   curpos = curpos + 2;
   memcpy(curpos,scopelist,scopelistlen);
   /* predicate zero length */
   /* spi list zero length */

   if (sock == -1)
   {

#ifndef MI_NOT_SUPPORTED
      NetworkMcastRqstRply(handle,
#else
            NetworkMcastRqstRply("en",
#endif /* MI_NOT_SUPPORTED */
            buf,
            SLP_FUNCT_DASRVRQST,
            bufsize,
            KnownDADiscoveryCallback,
            &result);

         }
   else
   {
      NetworkRqstRply(sock,
            peeraddr,
            "en",
            0,
            buf,
            SLP_FUNCT_DASRVRQST,
            bufsize,
            KnownDADiscoveryCallback,
            &result);
   }

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
#ifndef MI_NOT_SUPPORTED
int KnownDADiscoverFromMulticast(int scopelistlen, const char* scopelist, PSLPHandleInfo handle)
#else
int KnownDADiscoverFromMulticast(int scopelistlen, const char* scopelist)
#endif /* MI_NOT_SUPPORTED */
{
   int result = 0;

   if (SLPPropertyAsBoolean(SLPGetProperty("net.slp.activeDADetection")) &&
         SLPPropertyAsInteger(SLPGetProperty("net.slp.DADiscoveryMaximumWait")))
   {
      result = KnownDADiscoveryRqstRply(-1,
            NULL,
            scopelistlen,
#ifndef MI_NOT_SUPPORTED
                                          scopelist,
            handle);
#else
                                          scopelist);
#endif /* MI_NOT_SUPPORTED */
   }

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
#ifndef MI_NOT_SUPPORTED
int KnownDADiscoverFromDHCP(PSLPHandleInfo handle)
#else
int KnownDADiscoverFromDHCP()
#endif /* MI_NOT_SUPPORTED */
{
   int count = 0;
   int scopelistlen;
   DHCPContext ctx;
   unsigned char *alp;
   struct timeval timeout;
   struct sockaddr_storage peeraddr;
   unsigned char dhcpOpts[] = {TAG_SLP_SCOPE, TAG_SLP_DA};

   /* only do DHCP discovery if IPv4 is enabled. */
   if (!SLPNetIsIPV4())
      return 0;

   *ctx.scopelist = 0;
   ctx.addrlistlen = 0;

   DHCPGetOptionInfo(dhcpOpts, sizeof(dhcpOpts), DHCPParseSLPTags, &ctx);

   if (!*ctx.scopelist)
   {
      const char *slp = SLPGetProperty("net.slp.useScopes");
      if (slp)
         strcpy(ctx.scopelist, slp);
   }
   scopelistlen = strlen(ctx.scopelist);

   SLPNetSetAddr(&peeraddr, AF_INET, SLP_RESERVED_PORT, NULL, 0);

   timeout.tv_sec = SLPPropertyAsInteger(SLPGetProperty("net.slp.DADiscoveryMaximumWait"));
   timeout.tv_usec = (timeout.tv_sec % 1000) * 1000;
   timeout.tv_sec = timeout.tv_sec / 1000;

   alp = ctx.addrlist;

   while (ctx.addrlistlen >= 4)
   {
      memcpy(&(((struct sockaddr_in *)&peeraddr)->sin_addr.s_addr), alp, 4);
      if (((struct sockaddr_in*)&peeraddr)->sin_addr.s_addr)
      {
         int sockfd;
         if ((sockfd = SLPNetworkConnectStream(&peeraddr, &timeout)) >= 0)
         {
            count = KnownDADiscoveryRqstRply(sockfd, 
                  &peeraddr, 
                  scopelistlen, 
#ifndef MI_NOT_SUPPORTED
                                                                 ctx.scopelist,
                  handle);
#else
                                                                 ctx.scopelist);
#endif /* MI_NOT_SUPPORTED  */
            closesocket(sockfd);
            if (scopelistlen && count)
               break;	/* stop after the first set found */
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
int KnownDADiscoverFromProperties(int scopelistlen,
#ifndef MI_NOT_SUPPORTED
      const char* scopelist,
      PSLPHandleInfo handle)
#else
const char* scopelist)
#endif /* MI_NOT_SUPPORTED */
{
   char*					temp;
   char*					tempend;
   char*					slider1;
   char*					slider2;
   int						sockfd;
   struct sockaddr_storage peeraddr;
   struct timeval			timeout;
   int						result      = 0;

   slider1 = slider2 = temp = xstrdup(SLPGetProperty("net.slp.DAAddresses"));
   if (temp)
   {
      tempend = temp + strlen(temp);
      while (slider1 != tempend)
      {
         timeout.tv_sec = SLPPropertyAsInteger(SLPGetProperty("net.slp.DADiscoveryMaximumWait"));
         timeout.tv_usec = (timeout.tv_sec % 1000) * 1000;
         timeout.tv_sec = timeout.tv_sec / 1000;

         while (*slider2 && *slider2 != ',') slider2++;
         *slider2 = 0;

         if (SLPNetResolveHostToAddr(slider1, &peeraddr) == 0)
         {
            SLPNetSetParams(&peeraddr, peeraddr.ss_family, SLP_RESERVED_PORT);
            sockfd = SLPNetworkConnectStream(&peeraddr,&timeout);
            if (sockfd >= 0)
            {
               result = KnownDADiscoveryRqstRply(sockfd,
                     &peeraddr,
                     scopelistlen,
#ifndef MI_NOT_SUPPORTED
                                                      scopelist,
                     handle);
#else                                                 
                                       scopelist);
#endif		    
               closesocket(sockfd);
               if (scopelistlen && result)
               {
                  /* return if we found at least one DA */
                  break;
               }
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
#ifndef MI_NOT_SUPPORTED
int KnownDADiscoverFromIPC(PSLPHandleInfo handle)
#else
int KnownDADiscoverFromIPC()
#endif
{
   struct sockaddr_storage peeraddr;
   int sockfd;
   int result = 0;

   sockfd = NetworkConnectToSlpd(&peeraddr);
   if (sockfd >= 0)
   {
#ifndef MI_NOT_SUPPORTED
      result = KnownDADiscoveryRqstRply(sockfd, &peeraddr, 0, "", handle);
#else
      result = KnownDADiscoveryRqstRply(sockfd, &peeraddr, 0, "");
#endif
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
SLPBoolean KnownDAFromCache(int scopelistlen,
      const char* scopelist,
      int spistrlen,
      const char* spistr,
#ifndef MI_NOT_SUPPORTED
      struct sockaddr_storage* daaddr,
      PSLPHandleInfo handle)
#else
struct sockaddr_storage* daaddr)
#endif /* MI_NOT_SUPPORTED */
   {
      time_t          curtime;

      if (KnownDAListFind(scopelistlen,
            scopelist,
            spistrlen,
            spistr,
            daaddr) == SLP_FALSE)
      {
         curtime = time(&curtime);
         if (G_KnownDALastCacheRefresh == 0 ||
               curtime - G_KnownDALastCacheRefresh > MINIMUM_DISCOVERY_INTERVAL)
         {
            G_KnownDALastCacheRefresh = curtime;

            /* discover DAs */
#ifndef MI_NOT_SUPPORTED
            if (KnownDADiscoverFromIPC(handle) == 0)
               if (KnownDADiscoverFromProperties(scopelistlen, scopelist, handle) == 0)
                  if (KnownDADiscoverFromDHCP(handle) == 0)
                     KnownDADiscoverFromMulticast(scopelistlen, scopelist, handle);
#else
            if (KnownDADiscoverFromIPC() == 0)
               if (KnownDADiscoverFromProperties(scopelistlen, scopelist) == 0)
                  if (KnownDADiscoverFromDHCP() == 0)
                     KnownDADiscoverFromMulticast(scopelistlen, scopelist);
#endif		
         }

         return KnownDAListFind(scopelistlen,
               scopelist,
               spistrlen,
               spistr,
               daaddr);
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
int KnownDAConnect(PSLPHandleInfo handle,
      int scopelistlen,
      const char* scopelist,
      struct sockaddr_storage* peeraddr)
{
   struct timeval  timeout;
   int             sock = -1;
   int                 spistrlen   = 0;
   char*               spistr      = 0;
#ifdef ENABLE_SLPv2_SECURITY
   if (SLPPropertyAsBoolean(SLPGetProperty("net.slp.securityEnabled")))
   {
      SLPSpiGetDefaultSPI(handle->hspi,
            SLPSPI_KEY_TYPE_PUBLIC,
            &spistrlen,
            &spistr);
   }
#endif

   /* Set up connect timeout */
   timeout.tv_sec = SLPPropertyAsInteger(SLPGetProperty("net.slp.DADiscoveryMaximumWait"));
   timeout.tv_usec = (timeout.tv_sec % 1000) * 1000;
   timeout.tv_sec = timeout.tv_sec / 1000;

   while (1)
   {
      memset(peeraddr,0,sizeof(struct sockaddr_storage));

      if (KnownDAFromCache(scopelistlen,
            scopelist,
            spistrlen,
            spistr,
#ifndef MI_NOT_SUPPORTED
            peeraddr,
            handle) == 0)
#else
         peeraddr) == 0)
#endif /* MI_NOT_SUPPORTED */
      {
         break;
      }

      if (peeraddr->ss_family == AF_INET6 && SLPNetIsIPV6())
      {
         SLPNetSetPort(peeraddr, SLP_RESERVED_PORT);
         sock = SLPNetworkConnectStream(peeraddr,&timeout);
      }
      if (peeraddr->ss_family == AF_INET && SLPNetIsIPV4())
      {
         SLPNetSetPort(peeraddr, SLP_RESERVED_PORT);
         sock = SLPNetworkConnectStream(peeraddr,&timeout);
      }

      if (sock >= 0)
      {
         break;
      }

      KnownDABadDA(peeraddr);
   }


#ifdef ENABLE_SLPv2_SECURITY
   if (spistr) xfree(spistr);
#endif

   return sock;
}

/** Mark a KnownDA as a Bad DA.
 *
 * @param[in] daaddr - The address of the bad DA.
 */
void KnownDABadDA(struct sockaddr_storage* daaddr)
{
   SLPDatabaseHandle   dh;
   SLPDatabaseEntry*   entry;

   dh = SLPDatabaseOpen(&G_KnownDACache);
   if (dh)
   {
      /*-----------------------------------*/
      /* Check to find the requested entry */
      /*-----------------------------------*/
      while (1)
      {
         entry = SLPDatabaseEnum(dh);
         if (entry == NULL) break;

         /* Assume DAs are identical if their in_addrs match */
         if (SLPNetCompareAddrs(daaddr,&(entry->msg->peer)) == 0)
         {
            SLPDatabaseRemove(dh,entry);
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
int KnownDAGetScopes(int* scopelistlen,
#ifndef MI_NOT_SUPPORTED
      char** scopelist,
      PSLPHandleInfo handle)
#else
char** scopelist)
#endif
{
   int                 newlen;
   SLPDatabaseHandle   dh;
   SLPDatabaseEntry*   entry;

   /* discover all DAs */
#ifndef MI_NOT_SUPPORTED
   if (KnownDADiscoverFromIPC(handle) == 0)
#else
      if (KnownDADiscoverFromIPC() == 0)
#endif
      {
#ifndef MI_NOT_SUPPORTED
         KnownDADiscoverFromDHCP(handle);
         KnownDADiscoverFromProperties(0,"", handle);
         KnownDADiscoverFromMulticast(0,"", handle);
#else
         KnownDADiscoverFromDHCP();
         KnownDADiscoverFromProperties(0,"");
         KnownDADiscoverFromMulticast(0,"");
#endif
      }


      /* enumerate through all the knownda entries and generate a */
      /* scopelist                                                */
      dh = SLPDatabaseOpen(&G_KnownDACache);
   if (dh)
   {
      /*-----------------------------------*/
      /* Check to find the requested entry */
      /*-----------------------------------*/
      while (1)
      {
         entry = SLPDatabaseEnum(dh);
         if (entry == NULL) break;
         newlen = G_KnownDAScopesLen;
         while (SLPUnionStringList(G_KnownDAScopesLen,
               G_KnownDAScopes,
               entry->msg->body.daadvert.scopelistlen,
               entry->msg->body.daadvert.scopelist,
               &newlen,
               G_KnownDAScopes) < 0)
         {
            G_KnownDAScopes = xrealloc(G_KnownDAScopes,newlen);
            if (G_KnownDAScopes == 0)
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
   while (SLPUnionStringList(G_KnownDAScopesLen,
         G_KnownDAScopes,
         strlen(SLPPropertyGet("net.slp.useScopes")),
         SLPPropertyGet("net.slp.useScopes"),
         &newlen,
         G_KnownDAScopes) < 0)
   {
      G_KnownDAScopes = xrealloc(G_KnownDAScopes,newlen);
      if (G_KnownDAScopes == 0)
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
      {
         return -1;
      }
      memcpy(*scopelist,G_KnownDAScopes, G_KnownDAScopesLen);
      (*scopelist)[G_KnownDAScopesLen] = 0;
      *scopelistlen = G_KnownDAScopesLen;
   }
   else
   {
      *scopelist = xstrdup("");
      if (*scopelist == 0)
      {
         return -1;
      }
      *scopelistlen = 0;
   }

   return 0;
}

/** Process a SrvRqst for service:directory-agent.
 *
 * @param[in] handle - The SLP session handle associated with this request.
 */
void KnownDAProcessSrvRqst(PSLPHandleInfo handle)
{
   SLPDatabaseHandle   dh;
   SLPDatabaseEntry*   entry;
   SLPBoolean          cb_result;
   char                tmp;

   /* discover all DAs */
#ifndef MI_NOT_SUPPORTED
   if (KnownDADiscoverFromIPC(handle) == 0)
#else
      if (KnownDADiscoverFromIPC() == 0)
#endif
      {
#ifndef MI_NOT_SUPPORTED
         KnownDADiscoverFromDHCP(handle);
         KnownDADiscoverFromProperties(0,"", handle);
         KnownDADiscoverFromMulticast(0,"", handle);
#else
         KnownDADiscoverFromDHCP();
         KnownDADiscoverFromProperties(0,"");
         KnownDADiscoverFromMulticast(0,"");
#endif
      }

      /* Enumerate through knownDA database */
      dh = SLPDatabaseOpen(&G_KnownDACache);
   if (dh)
   {
      /* Check to see if there a matching entry */
      while (1)
      {
         entry = SLPDatabaseEnum(dh);

         /* is there anything left? */
         if (entry == NULL) break;

         /* TRICKY temporary null termination of DA url */
         tmp = entry->msg->body.daadvert.url[entry->msg->body.daadvert.urllen];
         ((char*)(entry->msg->body.daadvert.url))[entry->msg->body.daadvert.urllen] = 0;

         /* Call the SrvURLCallback */
         cb_result = handle->params.findsrvs.callback((SLPHandle)handle,
               entry->msg->body.daadvert.url,
               SLP_LIFETIME_MAXIMUM,
               SLP_OK,
               handle->params.findsrvs.cookie);

         /* TRICKY: undo temporary null termination of DA url */
         ((char*)(entry->msg->body.daadvert.url))[entry->msg->body.daadvert.urllen] = tmp;

         /* does the caller want more? */
         if (cb_result == SLP_FALSE)
         {
            break;
         }
      }

      SLPDatabaseClose(dh);
   }

   /* Make SLP_LAST_CALL */
   handle->params.findsrvs.callback((SLPHandle)handle,
         NULL,
         0,
         SLP_LAST_CALL,
         handle->params.findsrvs.cookie);
}

#ifdef DEBUG
/** Frees all (cached) resources associated with known DAs.
 */
void KnownDAFreeAll(void)
{
   SLPDatabaseHandle   dh;
   SLPDatabaseEntry*   entry;
   dh = SLPDatabaseOpen(&G_KnownDACache);
   if (dh)
   {
      while (1)
      {
         entry = SLPDatabaseEnum(dh);
         if (entry == NULL) break;

         SLPDatabaseRemove(dh,entry);
      }

      SLPDatabaseClose(dh);
   }


   if (G_KnownDAScopes) xfree(G_KnownDAScopes);
   G_KnownDAScopesLen = 0;
   G_KnownDALastCacheRefresh = 0;

}
#endif

/*=========================================================================*/
