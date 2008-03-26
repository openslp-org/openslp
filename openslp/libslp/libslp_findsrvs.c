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

/** Find servers.
 *
 * Implementation for SLPFindSrvs() call.
 *
 * @file       libslp_findsrvs.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    LibSLPCode
 */

#include "slp.h"
#include "libslp.h"
#include "slp_net.h"
#include "slp_property.h"
#include "slp_xmalloc.h"
#include "slp_message.h"

/** Collates response data to user callback for SLPFindSrv requests.
 *
 * @param[in] hSLP - The SLP handle object associated with the request.
 * @param[in] pcSrvURL - The service URL for this pass.
 * @param[in] sLifetime - The lifetime value for @p pcSrvURL.
 * @param[in] errorcode - The error code received on this pass.
 *
 * @return An SLP boolean value; SLP_TRUE indicates we are finished; 
 *    SLP_FALSE indicates we should continue.
 *
 * @todo Trace the logic of CollateToSLPSrvURLCallback to ensure 
 *    that it works.
 *
 * @internal
 */
static SLPBoolean CollateToSLPSrvURLCallback(SLPHandle hSLP, 
      const char * pcSrvURL, unsigned short sLifetime, 
      SLPError errorcode)
{
   int maxResults;
   SLPHandleInfo * handle = hSLP;
   SLPSrvUrlCollatedItem * collateditem;

#ifdef ENABLE_ASYNC_API
   /* Do not collate for async calls. */
   if (handle->isAsync)
      return handle->params.findsrvs.callback(hSLP, pcSrvURL, 
            sLifetime, errorcode, handle->params.findsrvs.cookie);
#endif

   /* Configure behaviour for desired max results */
   maxResults = SLPPropertyAsInteger("net.slp.maxResults");
   if (maxResults == -1)
      maxResults = INT_MAX;

   if (errorcode == SLP_LAST_CALL || handle->callbackcount > maxResults)
   {
      /* We are done so call the caller's callback for each
       * service URL collated item and clean up the collation list.
       */
      handle->params.findsrvs.callback(handle, 0, 0, 
            SLP_LAST_CALL, handle->params.findsrvs.cookie);
      goto CLEANUP;
   }
   else if (errorcode != SLP_OK)
      return SLP_TRUE;

   /* We're adding another result - increment result count. */
   handle->callbackcount++;

   /* Add the service URL to the colation list. */
   collateditem = (SLPSrvUrlCollatedItem *)handle->collatedsrvurls.head;
   while (collateditem)
   {
      if (strcmp(collateditem->srvurl, pcSrvURL) == 0)
         break;
      collateditem = (SLPSrvUrlCollatedItem *)collateditem->listitem.next;
   }

   /* Create a new item if none was found. */
   if (collateditem == 0)
   {
      collateditem = xmalloc(sizeof(SLPSrvUrlCollatedItem) 
            + strlen(pcSrvURL) + 1);
      if (collateditem)
      {
         memset(collateditem, 0, sizeof(SLPSrvUrlCollatedItem));
         collateditem->srvurl = (char *)(collateditem + 1);
         strcpy(collateditem->srvurl, pcSrvURL);
         collateditem->lifetime = sLifetime;

         /* Add the new item to the collated list. */
         SLPListLinkTail(&handle->collatedsrvurls, 
               (SLPListItem *)collateditem);

         /* Call the caller's callback. */
         if (handle->params.findsrvs.callback(handle, pcSrvURL, sLifetime, 
               SLP_OK, handle->params.findsrvs.cookie) == SLP_FALSE)
            goto CLEANUP;
      }
   }
   return SLP_TRUE;

CLEANUP:

   /* Free the collation list. */
   while (handle->collatedsrvurls.count)
   {
      collateditem = (SLPSrvUrlCollatedItem *)SLPListUnlink(
            &handle->collatedsrvurls, handle->collatedsrvurls.head);
      xfree(collateditem);
   }   
   handle->callbackcount = 0;

   return SLP_FALSE;
}

/** SLPFindSrvs callback routine for NetworkRqstRply.
 *
 * @param[in] errorcode - The network operation error code.
 * @param[in] peeraddr - The network address of the responder.
 * @param[in] replybuf - The response buffer from the network request.
 * @param[in] cookie - Callback context data from ProcessSrvReg.
 *
 * @return SLP_FALSE (to stop any iterative callbacks).
 *
 * @internal
 */
static SLPBoolean ProcessSrvRplyCallback(SLPError errorcode, 
      void * peeraddr, SLPBuffer replybuf, void * cookie)
{
   SLPMessage * replymsg;
   SLPBoolean result = SLP_TRUE;
   SLPHandleInfo * handle = (SLPHandleInfo *)cookie;

#ifdef ENABLE_SLPv2_SECURITY
   SLPBoolean securityEnabled = SLPPropertyAsBoolean("net.slp.securityEnabled");
#endif

   /* Check the errorcode and bail if it is set. */
   if (errorcode != SLP_OK)
      return CollateToSLPSrvURLCallback(handle, 0, 0, errorcode);

   /* parse the replybuf */
   replymsg = SLPMessageAlloc();
   if (replymsg)
   {
      if (!SLPMessageParseBuffer(peeraddr, 0, replybuf, replymsg))
      {
         if (replymsg->header.functionid == SLP_FUNCT_SRVRPLY 
               && replymsg->body.srvrply.errorcode == 0)
         {
            int i;
            SLPUrlEntry * urlentry = replymsg->body.srvrply.urlarray;

            for (i = 0; i < replymsg->body.srvrply.urlcount; i++)
            {
#ifdef ENABLE_SLPv2_SECURITY
               /* Validate the service authblocks. */
               if (securityEnabled 
                     && SLPAuthVerifyUrl(handle->hspi, 1, &urlentry[i]))
                  continue; /* Authentication failed, skip this URLEntry. */
#endif
               result = CollateToSLPSrvURLCallback(handle, urlentry[i].url, 
                     (unsigned short)urlentry[i].lifetime, SLP_OK);
               if (result == SLP_FALSE)
                  break;
            } 
         }
         else if (replymsg->header.functionid == SLP_FUNCT_DAADVERT 
               && replymsg->body.daadvert.errorcode == 0)
         {
#ifdef ENABLE_SLPv2_SECURITY
            if (securityEnabled && SLPAuthVerifyDAAdvert(handle->hspi, 
                  1, &replymsg->body.daadvert))
            {
               /* Verification failed. Ignore message. */
               SLPMessageFree(replymsg);
               return SLP_TRUE;
            }
#endif
            result = CollateToSLPSrvURLCallback(handle, 
                  replymsg->body.daadvert.url, SLP_LIFETIME_MAXIMUM, 
                  SLP_OK);
         }
         else if (replymsg->header.functionid == SLP_FUNCT_SAADVERT)
         {
#ifdef ENABLE_SLPv2_SECURITY
            if (securityEnabled && SLPAuthVerifySAAdvert(handle->hspi, 1, 
                  &replymsg->body.saadvert))
            {
               /* Verification failed. Ignore message. */
               SLPMessageFree(replymsg);
               return SLP_TRUE;
            }
#endif
            result = CollateToSLPSrvURLCallback(handle, 
                  replymsg->body.saadvert.url, SLP_LIFETIME_MAXIMUM, 
                  SLP_OK);
         }
      }
      SLPMessageFree(replymsg);
   }
   return result;
}

/** Formats and sends an SLPFindSrvs wire buffer request.
 *
 * @param handle - The OpenSLP session handle, containing request 
 *    parameters. See docs for SLPFindSrvs.
 *
 * @return Zero on success, or an SLP API error code.
 * 
 * @internal
 */
static SLPError ProcessSrvRqst(SLPHandleInfo * handle)
{
   uint8_t * buf;
   uint8_t * curpos;
   SLPError serr;
   size_t spistrlen = 0;
   char * spistr = 0;
   struct sockaddr_storage peeraddr;
   sockfd_t sock = SLP_INVALID_SOCKET;

   /* Is this a special attempt to locate DAs? */
   if (strncasecmp(handle->params.findsrvs.srvtype, SLP_DA_SERVICE_TYPE,
         handle->params.findsrvs.srvtypelen) == 0)
   {
      KnownDAProcessSrvRqst(handle);
      return 0;
   }

#ifdef ENABLE_SLPv2_SECURITY
   if (SLPPropertyAsBoolean("net.slp.securityEnabled"))
      SLPSpiGetDefaultSPI(handle->hspi, SLPSPI_KEY_TYPE_PUBLIC, 
            &spistrlen, &spistr);
#endif

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

   buf = curpos = xmalloc(
         + 2 + handle->params.findsrvs.srvtypelen
         + 2 + handle->params.findsrvs.scopelistlen
         + 2 + handle->params.findsrvs.predicatelen
         + 2 + spistrlen);
   if (buf == 0)
   {
      xfree(spistr);
      return SLP_MEMORY_ALLOC_FAILED;
   }

   /* <service-type> */
   PutL16String(&curpos, handle->params.findsrvs.srvtype, 
         handle->params.findsrvs.srvtypelen);

   /* <scope-list> */
   PutL16String(&curpos, handle->params.findsrvs.scopelist,
         handle->params.findsrvs.scopelistlen);

   /* predicate string */
   PutL16String(&curpos, handle->params.findsrvs.predicate,
         handle->params.findsrvs.predicatelen);

   /* <SLP SPI> */
   PutL16String(&curpos, (char *)spistr, spistrlen);

   /* Call the RqstRply engine. */
   do
   {
#ifndef UNICAST_NOT_SUPPORTED
      if (handle->dounicast == 1) 
      {
         serr = NetworkUcastRqstRply(handle, buf, SLP_FUNCT_SRVRQST, 
               curpos - buf, ProcessSrvRplyCallback, handle);
         break;
      }
#endif

      if (strncasecmp(handle->params.findsrvs.srvtype, SLP_SA_SERVICE_TYPE,
            handle->params.findsrvs.srvtypelen) != 0)
         sock = NetworkConnectToDA(handle, handle->params.findsrvs.scopelist,
               handle->params.findsrvs.scopelistlen, &peeraddr);

      if (sock == SLP_INVALID_SOCKET)
      {
         /* Use multicast as a last resort. */
         serr = NetworkMcastRqstRply(handle, buf, SLP_FUNCT_SRVRQST, 
               curpos - buf, ProcessSrvRplyCallback, 0);
         break;
      }

      serr = NetworkRqstRply(sock, &peeraddr, handle->langtag, 0, buf, 
            SLP_FUNCT_SRVRQST, curpos - buf, ProcessSrvRplyCallback, 
            handle);
      if (serr)
         NetworkDisconnectDA(handle);

   } while (serr == SLP_NETWORK_ERROR);

   xfree(buf);
   xfree(spistr);

   return serr;
}   

#ifdef ENABLE_ASYNC_API
/** Thread start procedure for asynchronous find services request.
 *
 * @param[in,out] handle - Contains the request parameters, returns the
 *    request result.
 *
 * @return An SLPError code.
 *
 * @internal
 */
static SLPError AsyncProcessSrvRqst(SLPHandleInfo * handle)
{
   SLPError serr = ProcessSrvRqst(handle);
   xfree((void *)handle->params.findsrvs.srvtype);
   xfree((void *)handle->params.findsrvs.scopelist);
   xfree((void *)handle->params.findsrvs.predicate);
   SLPSpinLockRelease(&handle->inUse);
   return serr;
}
#endif

/** Return a list of service matching a query specification.
 *
 * Issue the query for services on the language specific SLPHandle and
 * return the results through the @p callback. The parameters determine
 * the results.
 *
 * @param[in] hSLP - The language specific SLPHandle on which to search 
 *    for services.
 * @param[in] pcServiceType - The Service Type String, including authority 
 *    string if any, for the request, such as can be discovered using 
 *    SLPSrvTypes. This could be, for example "service:printer:lpr" or
 *    "service:nfs". May not be the empty string or NULL.
 * @param[in] pcScopeList - A pointer to a char containing a comma-separated 
 *    list of scope names. Pass in NULL or the empty string ("") to find 
 *    services in all the scopes the local host is configured to query.
 * @param[in] pcSearchFilter - A query formulated of attribute pattern 
 *    matching expressions in the form of a LDAPv3 Search Filter, see 
 *    [RFC 2254]. If this filter is empty, i.e. "" or NULL, all services 
 *    of the requested type in the specified scopes are returned.
 * @param[in] callback - A callback function through which the results of 
 *    the operation are reported.
 * @param[in] pvCookie - Memory passed to the @p callback code from the 
 *    client. May be NULL.
 *
 * @return If an error occurs in starting the operation, one of the SLPError
 *    codes is returned.
 */
SLPEXP SLPError SLPAPI SLPFindSrvs(
      SLPHandle hSLP,
      const char * pcServiceType,
      const char * pcScopeList,
      const char * pcSearchFilter,
      SLPSrvURLCallback callback,
      void * pvCookie)
{
   bool inuse;
   SLPError serr = 0;
   SLPHandleInfo * handle = hSLP;

   /* Check for invalid parameters. */
   SLP_ASSERT(handle != 0);
   SLP_ASSERT(handle->sig == SLP_HANDLE_SIG);
   SLP_ASSERT(pcServiceType != 0);
   SLP_ASSERT(*pcServiceType != 0);
   SLP_ASSERT(callback != 0);

   if (handle == 0 || handle->sig != SLP_HANDLE_SIG 
         || pcServiceType == 0 || *pcServiceType == 0 
         || callback == 0)
      return SLP_PARAMETER_BAD;

   /* Check to see if the handle is in use. */
   inuse = SLPSpinLockTryAcquire(&handle->inUse);
   SLP_ASSERT(!inuse);
   if (inuse)
      return SLP_HANDLE_IN_USE;

   /* Get a scope list if not supplied. */
   if (pcScopeList == 0 || *pcScopeList == 0)
      pcScopeList = SLPPropertyGet("net.slp.useScopes", 0, 0);

   /* Get a search filter if not supplied */
   if (pcSearchFilter == 0)
      pcSearchFilter = "";

   /* Set the handle up to reference parameters. */
   handle->params.findsrvs.srvtypelen = strlen(pcServiceType);
   handle->params.findsrvs.srvtype = pcServiceType;
   handle->params.findsrvs.scopelistlen = strlen(pcScopeList);
   handle->params.findsrvs.scopelist = pcScopeList;
   handle->params.findsrvs.predicatelen = strlen(pcSearchFilter);
   handle->params.findsrvs.predicate = pcSearchFilter;
   handle->params.findsrvs.callback = callback;
   handle->params.findsrvs.cookie = pvCookie; 

   /* Check to see if we should be async or sync. */
#ifdef ENABLE_ASYNC_API
   if (handle->isAsync)
   {
      /* Copy all of the referenced parameters before creating thread. */
      handle->params.findsrvs.srvtype = xstrdup(handle->params.findsrvs.srvtype);
      handle->params.findsrvs.scopelist = xstrdup(handle->params.findsrvs.scopelist);
      handle->params.findsrvs.predicate = xstrdup(handle->params.findsrvs.predicate);

      /* Ensure strdups and thread create succeed. */
      if (handle->params.findsrvs.srvtype == 0
            || handle->params.findsrvs.scopelist == 0
            || handle->params.findsrvs.predicate == 0
            || (handle->th = SLPThreadCreate((SLPThreadStartProc)
                  AsyncProcessSrvRqst, handle)) == 0)
      {
         serr = SLP_MEMORY_ALLOC_FAILED;    
         xfree((void *)handle->params.findsrvs.srvtype);
         xfree((void *)handle->params.findsrvs.scopelist);
         xfree((void *)handle->params.findsrvs.predicate);
         SLPSpinLockRelease(&handle->inUse);
      }
   }
   else
#endif
   {
      /* Leave all parameters referenced. */
      serr = ProcessSrvRqst(handle);
      SLPSpinLockRelease(&handle->inUse);
   }
   return serr;
}

/*=========================================================================*/
