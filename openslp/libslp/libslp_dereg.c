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

/** Deregister service.
 *
 * Implementation for SLPDereg() call.
 *
 * @file       libslp_dereg.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    LibSLPCode
 */

#include "slp.h"
#include "libslp.h"
#include "slp_property.h"
#include "slp_message.h"
#include "slp_xmalloc.h"

/** SLPDereg callback routine for NetworkRqstRply.
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
static SLPBoolean CallbackSrvDeReg(SLPError errorcode, 
      void * peeraddr, SLPBuffer replybuf, void * cookie)
{
   SLPHandleInfo * handle = (SLPHandleInfo *)cookie;

   /* Check the errorcode and bail if it is set. */
   if (errorcode == 0)
   {
      /* Parse the replybuf into a message. */
      SLPMessage * replymsg = SLPMessageAlloc();
      if (replymsg)
      {
         errorcode = (SLPError)(-SLPMessageParseBuffer(
               peeraddr, 0, replybuf, replymsg));
         if (errorcode == 0 
               && replymsg->header.functionid == SLP_FUNCT_SRVACK)
            errorcode = (SLPError)(-replymsg->body.srvack.errorcode);
         else
            errorcode = SLP_NETWORK_ERROR;
         SLPMessageFree(replymsg);
      }
      else
         errorcode = SLP_MEMORY_ALLOC_FAILED;
   }

   /* Call the user's callback function. */
   handle->params.dereg.callback(handle, errorcode, 
         handle->params.dereg.cookie);

   return SLP_FALSE;
}

/** Formats and sends an SLPDereg wire buffer request.
 * 
 * This is an unqualified SLPDereg wire request (the tag-list is empty). 
 * See SLPDelAttrs for information qualified, or partial deregistrations.
 * 
 * @param handle - The OpenSLP session handle, containing request 
 *    parameters. See docs for SLPDereg.
 *
 * @return Zero on success, or an SLP API error code.
 * 
 * @internal
 */
static SLPError ProcessSrvDeReg(SLPHandleInfo * handle)
{
   sockfd_t sock;
   uint8_t * buf;
   uint8_t * curpos;
   SLPError serr;
   size_t urlauthlen = 0;
   uint8_t * urlauth = 0;
   struct sockaddr_storage saaddr;

#ifdef ENABLE_SLPv2_SECURITY
   if (SLPPropertyAsBoolean(SLPGetProperty("net.slp.securityEnabled")))
      if (SLPAuthSignUrl(handle->hspi, 0, 0, 
            handle->params.dereg.urllen, handle->params.dereg.url, 
            &urlauthlen, &urlauth) != 0)
         return SLP_AUTHENTICATION_ABSENT;
#endif

/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    Length of <scope-list>     |         <scope-list>          \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           URL Entry                           \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      Length of <tag-list>     |            <tag-list>         \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   buf = curpos = xmalloc(
         + 2 + handle->params.dereg.scopelistlen 
         + SizeofURLEntry(handle->params.dereg.urllen, urlauthlen) 
         + 2 + 0);   /* Full deregistration - no tag-list needed. */
   if (buf == 0)
   {
      xfree(urlauth);
      return SLP_MEMORY_ALLOC_FAILED;
   }

   /* <scope-list> */
   PutL16String(&curpos, handle->params.dereg.scopelist, 
         handle->params.dereg.scopelistlen);

   /* URL Entry */
   PutURLEntry(&curpos, 0, handle->params.dereg.url, 
         handle->params.dereg.urllen, urlauth, urlauthlen);

   /* empty <tag-list> */
   PutL16String(&curpos, 0, 0);

   /* Call the Request-Reply engine. */
   sock = NetworkConnectToSA(handle, handle->params.dereg.scopelist,
         handle->params.dereg.scopelistlen, &saaddr);
   if (sock != SLP_INVALID_SOCKET)
   {
      serr = NetworkRqstRply(sock, &saaddr, handle->langtag, 0, buf,
            SLP_FUNCT_SRVDEREG, curpos - buf, CallbackSrvDeReg, handle);
      if (serr)
         NetworkDisconnectSA(handle);
   }
   else
      serr = SLP_NETWORK_INIT_FAILED;

   xfree(buf);
   xfree(urlauth);

   return serr;
}

#ifdef ENABLE_ASYNC_API
/** Thread start procedure for asynchronous service deregistration.
 *
 * @param[in] handle - Contains the request parameters.
 *
 * @return An SLPError code.
 * 
 * @internal
 */
static SLPError AsyncProcessSrvDeReg(SLPHandleInfo * handle)
{
   SLPError serr = ProcessSrvDeReg(handle);
   xfree(handle->params.dereg.scopelist);
   xfree(handle->params.dereg.url);
   SLPReleaseSpinLock(&handle->inUse);
   return serr;
}
#endif

/** Deregisters a service URL with OpenSLP.
 *
 * Deregisters the advertisement for URL @p pcURL in all scopes where the
 * service is registered and all language locales. The deregistration
 * is not just confined to the locale of the SLPHandle, it is in all
 * locales. The API library is required to perform the operation in all
 * scopes obtained through configuration.
 *
 * @param[in] hSLP - The language specific SLPHandle to use for 
 *    deregistering.
 * @param[in] srvUrl - The URL to deregister. May not be the empty 
 *    string. May not be NULL. Must conform to SLP Service URL syntax or 
 *    SLP_INVALID_REGISTRATION will be returned.
 * @param[in] callback - A callback to report the operation completion 
 *    status.
 * @param[in] cookie - Memory passed to the callback code from the 
 *    client. May be NULL.
 *
 * @return If an error occurs in starting the operation, one of the 
 *    SLPError codes is returned.
 */
SLPError SLPAPI SLPDereg(
      SLPHandle hSLP,
      const char * srvUrl,
      SLPRegReport callback,
      void * cookie)
{
   bool inuse;
   SLPError serr;
   SLPSrvURL * parsedurl = 0;
   SLPHandleInfo * handle = hSLP;

   /* Check for invalid parameters. */
   SLP_ASSERT(handle != 0);
   SLP_ASSERT(handle->sig == SLP_HANDLE_SIG);
   SLP_ASSERT(srvUrl != 0);
   SLP_ASSERT(*srvUrl != 0);
   SLP_ASSERT(callback != 0);

   if (handle == 0 || handle->sig != SLP_HANDLE_SIG 
         || srvUrl == 0 || *srvUrl == 0 
         || callback == 0)
      return SLP_PARAMETER_BAD;

   /* Parse the service URL - just to see if we can, apparently. */
   serr = SLPParseSrvURL(srvUrl, &parsedurl);
   SLPFree(parsedurl);
   if (serr != SLP_OK)
      return serr == SLP_PARSE_ERROR? SLP_INVALID_REGISTRATION: serr;

   /* Check to see if the handle is in use. */
   inuse = SLPTryAcquireSpinLock(&handle->inUse);
   SLP_ASSERT(!inuse);
   if (inuse)
      return SLP_HANDLE_IN_USE;

   /* Set the handle up to reference parameters. */
   handle->params.dereg.scopelist = SLPGetProperty("net.slp.useScopes");
   handle->params.dereg.scopelistlen = strlen(handle->params.dereg.scopelist);
   handle->params.dereg.urllen = strlen(srvUrl); 
   handle->params.dereg.url = srvUrl;
   handle->params.dereg.callback = callback;
   handle->params.dereg.cookie = cookie;

   /* Check to see if we should be async or sync. */
#ifdef ENABLE_ASYNC_API
   if (handle->isAsync)
   {
      /* Copy all of the referenced parameters before creating thread. */
      handle->params.dereg.url = xstrdup(handle->params.dereg.url);
      handle->params.dereg.scopelist = xstrdup(handle->params.dereg.scopelist);

      /* Ensure strdups and thread create succeed. */
      if (handle->params.dereg.url == 0
            || handle->params.dereg.scopelist == 0
            || (handle->th = SLPThreadCreate((SLPThreadStartProc)
                  AsyncProcessSrvDeReg, handle)) == 0)
      {
         serr = SLP_MEMORY_ALLOC_FAILED;
         xfree(handle->params.dereg.scopelist);
         xfree(handle->params.dereg.url);
         SLPReleaseSpinLock(&handle->inUse);
      }
   }
   else
#endif
   {
      /* Reference all the parameters. */
      serr = ProcessSrvDeReg(handle);
      SLPReleaseSpinLock(&handle->inUse);
   }
   return serr;
}

/*=========================================================================*/
