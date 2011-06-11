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

/** Find attributes.
 *
 * Implementation for SLPFindAttrs() call.
 *
 * @file       libslp_findattrs.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    LibSLPCode
 */

#include "slp.h"
#include "libslp.h"
#include "slp_message.h"
#include "slp_xmalloc.h"
#include "slp_property.h"
#include "slp_net.h"

/** SLPFindAttrs callback routine for NetworkRqstRply.
 *
 * @param[in] errorcode - The network operation error code.
 * @param[in] peeraddr - The network address of the responder.
 * @param[in] replybuf - The response buffer from the network request.
 * @param[in] cookie - Callback context data from ProcessSrvReg.
 *
 * @return SLP_FALSE (to stop any iterative callbacks).
 *
 * @todo Do we need to add anything for collation here?
 *
 * @internal
 */
static SLPBoolean ProcessAttrRplyCallback(SLPError errorcode, 
      void * peeraddr, SLPBuffer replybuf, void * cookie)
{
   SLPMessage * replymsg;
   SLPHandleInfo * handle = (SLPHandleInfo *)cookie;
   SLPBoolean result = SLP_TRUE;

   /* Check the errorcode and bail if it is set. */
   if (errorcode)
   {
      handle->params.findattrs.callback(handle, 0, errorcode,
            handle->params.findattrs.cookie);
      return SLP_FALSE; /* Network failure, stop now. */
   }

   /* Parse the replybuf into a message. */
   replymsg = SLPMessageAlloc();
   if (replymsg != 0)
   {
      if (!SLPMessageParseBuffer(peeraddr, 0, replybuf, replymsg)
            && replymsg->header.functionid == SLP_FUNCT_ATTRRPLY 
            && !replymsg->body.attrrply.errorcode)
      {
         SLPAttrRply * attrrply = &replymsg->body.attrrply;
         if (attrrply->attrlistlen)
         {
#ifdef ENABLE_SLPv2_SECURITY
            /* Validate the attribute authblocks. */
            if (SLPPropertyAsBoolean("net.slp.securityEnabled") 
                  && SLPAuthVerifyString(handle->hspi, 1,
                        attrrply->attrlistlen, attrrply->attrlist,
                        attrrply->authcount, attrrply->autharray))
            {
               /* Could not verify the attr auth block. */
               SLPMessageFree(replymsg);
               return SLP_TRUE;  /* Authentication failure. */
            }
#endif
            /* Call the user's callback function. */
            result = handle->params.findattrs.callback(handle,
                  attrrply->attrlist, (SLPError)(-attrrply->errorcode), 
                  handle->params.findattrs.cookie);
         }
      }
      SLPMessageFree(replymsg);
   }
   return result;
}

/** Formats and sends an SLPFindAttrs wire buffer request.
 *
 * @param handle - The OpenSLP session handle, containing request 
 *    parameters. See docs for SLPFindAttrs.
 *
 * @return Zero on success, or an SLP API error code.
 * 
 * @internal
 */
static SLPError ProcessAttrRqst(SLPHandleInfo * handle)
{
   sockfd_t sock;
   uint8_t * buf;
   uint8_t * curpos;
   SLPError serr;
   size_t spistrlen = 0;
   char * spistr = 0;
   struct sockaddr_storage peeraddr;
   struct sockaddr_in* destaddrs = 0;

#ifdef ENABLE_SLPv2_SECURITY
   if (SLPPropertyAsBoolean("net.slp.securityEnabled"))
      SLPSpiGetDefaultSPI(handle->hspi, SLPSPI_KEY_TYPE_PUBLIC, 
            &spistrlen, &spistr);
#endif

/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         length of URL         |              URL              \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    length of <scope-list>     |      <scope-list> string      \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  length of <tag-list> string  |       <tag-list> string       \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   length of <SLP SPI> string  |        <SLP SPI> string       \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   buf = curpos = xmalloc(
         + 2 + handle->params.findattrs.urllen
         + 2 + handle->params.findattrs.scopelistlen
         + 2 + handle->params.findattrs.taglistlen
         + 2 + spistrlen);
   if (buf == 0)
   {
      xfree(spistr);
      return SLP_MEMORY_ALLOC_FAILED;
   }

   /* URL */
   PutL16String(&curpos, handle->params.findattrs.url, 
         handle->params.findattrs.urllen);

   /* <scope-list> */
   PutL16String(&curpos, handle->params.findattrs.scopelist, 
         handle->params.findattrs.scopelistlen);

   /* <tag-list>  */
   PutL16String(&curpos, handle->params.findattrs.taglist, 
         handle->params.findattrs.taglistlen);

   /* <SLP SPI> */
   PutL16String(&curpos, (char *)spistr, spistrlen);

   /* call the RqstRply engine */
   do
   {
#ifndef UNICAST_NOT_SUPPORTED
      if (handle->dounicast == 1) 
      {
         serr = NetworkUcastRqstRply(handle, buf, SLP_FUNCT_ATTRRQST, 
               curpos - buf, ProcessAttrRplyCallback, handle);
         break;
      }
      if (SLPNetIsIPV4())
      {
         if (KnownDASpanningListFromCache(handle,
                                          (int)handle->params.findattrs.scopelistlen,
                                          handle->params.findattrs.scopelist,
                                          &destaddrs) > 0)
         {
            serr = NetworkMultiUcastRqstRply(destaddrs,
                                             handle->langtag,
                                             (char*)buf,
                                             SLP_FUNCT_ATTRRQST,
                                             curpos - buf,
                                             ProcessAttrRplyCallback,
                                             handle);
            xfree(destaddrs);
            break;
         }
      }
#endif

      sock = NetworkConnectToDA(handle, handle->params.findattrs.scopelist,
            handle->params.findattrs.scopelistlen, &peeraddr);
      if (sock == SLP_INVALID_SOCKET)
      {
         /* use multicast as a last resort */
         serr = NetworkMcastRqstRply(handle, buf, SLP_FUNCT_ATTRRQST, 
               curpos - buf, ProcessAttrRplyCallback, 0);
         break;
      }

      serr = NetworkRqstRply(sock, &peeraddr, handle->langtag, 0, buf, 
            SLP_FUNCT_ATTRRQST, curpos - buf, ProcessAttrRplyCallback, 
            handle);
      if (serr)
         NetworkDisconnectDA(handle);

   } while (serr == SLP_NETWORK_ERROR);

   xfree(buf);
   xfree(spistr);

   return serr;
}

#ifdef ENABLE_ASYNC_API
/** Thread start procedure for asynchronous attribute request.
 *
 * @param[in,out] handle - Contains the request parameters, returns the
 *    request result.
 *
 * @return An SLPError code.
 *
 * @internal
 */
static SLPError AsyncProcessAttrRqst(SLPHandleInfo * handle)
{
   SLPError serr = ProcessAttrRqst(handle);
   xfree((void *)handle->params.findattrs.url);
   xfree((void *)handle->params.findattrs.scopelist);
   xfree((void *)handle->params.findattrs.taglist);
   SLPSpinLockRelease(&handle->inUse);
   return serr;
}
#endif

/** Returns a list of service attributes based on a search query.
 *
 * This function returns service attributes matching the attribute ids
 * for the indicated service URL or service type. If @p pcURLOrServiceType
 * is a service URL, the attribute information returned is for that
 * particular advertisement in the language locale of the SLPHandle.
 *
 * @par 
 * If @p pcURLOrServiceType is a service type name (including naming
 * authority if any), then the attributes for all advertisements of that
 * service type are returned regardless of the language of registration.
 * Results are returned through the @p callback.
 *
 * @par
 * The result is filtered with an SLP attribute request filter string
 * parameter, the syntax of which is described in [RFC 2608]. If the 
 * filter string is the empty string, i.e. "", all attributes are 
 * returned.
 *
 * @param[in] hSLP - The language specific SLPHandle on which to search 
 *    for attributes.
 * @param[in] pcURLOrServiceType - The service URL or service type. See 
 *    [RFC 2608] for URL and service type syntax. May not be the empty 
 *    string.
 * @param[in] pcScopeList - A pointer to a char containing a comma 
 *    separated list of scope names. Pass NULL or the empty string ("") 
 *    to find services in all the scopes the local host is configured to 
 *    query.
 * @param[in] pcAttrIds - The filter string indicating which attribute 
 *    values to return. Use empty string, "", to indicate all values. 
 *    Wildcards matching all attribute ids having a particular prefix or 
 *    suffix are also possible. See [RFC 2608] for the exact format of the 
 *    filter string.
 * @param[in] callback - A callback function through which the results of 
 *    the operation are reported.
 * @param[in] pvCookie - Memory passed to the callback code from the client.
 *    May be NULL.
 *
 * @return If an error occurs in starting the operation, one of the 
 *    SLPError codes is returned.
 */
SLPEXP SLPError SLPAPI SLPFindAttrs(
      SLPHandle hSLP,
      const char * pcURLOrServiceType,
      const char * pcScopeList,
      const char * pcAttrIds,
      SLPAttrCallback callback,
      void * pvCookie)
{
   bool inuse;
   SLPError serr = 0;
   SLPHandleInfo * handle = hSLP; 

   /* Check for invalid parameters. */
   SLP_ASSERT(handle != 0);
   SLP_ASSERT(handle->sig == SLP_HANDLE_SIG);
   SLP_ASSERT(pcURLOrServiceType != 0);
   SLP_ASSERT(*pcURLOrServiceType != 0);
   SLP_ASSERT(callback != 0);

   if (!handle || handle->sig != SLP_HANDLE_SIG 
         || pcURLOrServiceType == 0 || *pcURLOrServiceType == 0
         || callback == 0)
      return SLP_PARAMETER_BAD;

   /* Check to see if the handle is in use. */
   inuse = SLPSpinLockTryAcquire(&handle->inUse);
   SLP_ASSERT(!inuse);
   if (inuse)
      return SLP_HANDLE_IN_USE;

   /* Get a scope list if none was specified. */
   if (pcScopeList == 0 || *pcScopeList == 0)
      pcScopeList = SLPPropertyGet("net.slp.useScopes", 0, 0);

   /* Get a tag list if none was specified. */
   if (pcAttrIds == 0)
      pcAttrIds = "";

   /* Set the handle up to reference parameters. */
   handle->params.findattrs.urllen = strlen(pcURLOrServiceType);
   handle->params.findattrs.url = pcURLOrServiceType;
   handle->params.findattrs.scopelistlen = strlen(pcScopeList);
   handle->params.findattrs.scopelist = pcScopeList;
   handle->params.findattrs.taglistlen = strlen(pcAttrIds);
   handle->params.findattrs.taglist = pcAttrIds;
   handle->params.findattrs.callback = callback;
   handle->params.findattrs.cookie = pvCookie;

   /* Check to see if we should be async or sync. */
#ifdef ENABLE_ASYNC_API
   if (handle->isAsync)
   {
      /* Copy all of the referenced parameters before creating thread. */
      handle->params.findattrs.url = xstrdup(handle->params.findattrs.url);
      handle->params.findattrs.scopelist = xstrdup(handle->params.findattrs.scopelist);
      handle->params.findattrs.taglist = xstrdup(handle->params.findattrs.taglist);

      /* Ensure strdups and thread create succeed. */
      if (handle->params.findattrs.url == 0
            || handle->params.findattrs.scopelist == 0
            || handle->params.findattrs.taglist == 0
            || (handle->th = SLPThreadCreate((SLPThreadStartProc)
                  AsyncProcessAttrRqst, handle)) == 0)
      {
         serr = SLP_MEMORY_ALLOC_FAILED;    
         xfree((void *)handle->params.findattrs.url);
         xfree((void *)handle->params.findattrs.scopelist);
         xfree((void *)handle->params.findattrs.taglist);
         SLPSpinLockRelease(&handle->inUse);
      }
   }
   else
#endif
   {
      /* Reference all the parameters. */
      serr = ProcessAttrRqst(handle);
      SLPSpinLockRelease(&handle->inUse);
   }
   return serr;
}

/*=========================================================================*/
