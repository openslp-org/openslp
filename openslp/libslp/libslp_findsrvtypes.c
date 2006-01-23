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

/** Find server types.
 *
 * Implementation for SLPFindSrvType() call.
 *
 * @file       libslp_findsrvtypes.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    LibSLPCode
 */

#include "slp.h"
#include "libslp.h"
#include "slp_property.h"
#include "slp_xmalloc.h"
#include "slp_compare.h"
#include "slp_message.h"

/** Collates response data to user callback for SLPFindSrvType requests.
 *
 * @param[in] hSLP - The SLP handle object associated with the request.
 * @param[in] pcSrvTypes - The service type for this pass.
 * @param[in] errorcode - The error code received on this pass.
 *
 * @return An SLP boolean value; SLP_TRUE indicates we are finished; 
 *    SLP_FALSE indicates we should continue.
 *
 * @todo Trace the logic of CollateToSLPSrvTypeCallback to ensure that 
 *    it works.
 *
 * @internal
 */
static SLPBoolean CollateToSLPSrvTypeCallback(SLPHandle hSLP, 
      const char * pcSrvTypes, SLPError errorcode)
{
   int maxResults;
   char * srvtypes;
   size_t srvtypeslen;
   SLPHandleInfo * handle = hSLP;

   handle->callbackcount++;

   /* Do not collate for async calls. */
   if (handle->isAsync)
      return handle->params.findsrvtypes.callback(hSLP, pcSrvTypes, 
            errorcode, handle->params.findsrvtypes.cookie);

   /* Configure behaviour for desired max results. */
   maxResults = SLPPropertyAsInteger(SLPGetProperty("net.slp.maxResults"));
   if (maxResults == -1)
      maxResults = INT_MAX;

   if (errorcode == SLP_LAST_CALL || handle->callbackcount > maxResults)
   {
      /* We're done. Send back the collated srvtype string. */
      if (handle->collatedsrvtypes)
         if (handle->params.findsrvtypes.callback(handle, 
               handle->collatedsrvtypes, SLP_OK, 
               handle->params.findsrvtypes.cookie) == SLP_TRUE)
            handle->params.findsrvtypes.callback(handle, 0,
                  SLP_LAST_CALL, handle->params.findsrvtypes.cookie);

      /* Free the collatedsrvtype string. */
      if (handle->collatedsrvtypes)
      {
         xfree(handle->collatedsrvtypes);
         handle->collatedsrvtypes = 0;
      }
      handle->callbackcount = 0;
      return SLP_FALSE;
   }
   else if (errorcode != SLP_OK)
      return SLP_TRUE;

   /* Add the service types to the colation. */
   srvtypeslen = strlen(pcSrvTypes) + 1; /* +1 - terminator */
   if (handle->collatedsrvtypes)
      srvtypeslen += strlen(handle->collatedsrvtypes) + 1; /* +1 - comma */

   srvtypes = xmalloc(srvtypeslen);
   if (srvtypes)
   {
      if (handle->collatedsrvtypes)
      {
         if (SLPUnionStringList(strlen(handle->collatedsrvtypes), 
               handle->collatedsrvtypes, strlen(pcSrvTypes), pcSrvTypes,
               &srvtypeslen, srvtypes) != (int)srvtypeslen)
         {
            xfree(handle->collatedsrvtypes);
            handle->collatedsrvtypes = srvtypes;
         }
         else
         {
#ifndef COLLATION_CHANGES
            xfree(handle->collatedsrvtypes);
            handle->collatedsrvtypes = srvtypes;
            handle->collatedsrvtypes[srvtypeslen] = 0;
#else
            xfree(srvtypes);
#endif
         }   
      }
      else
      {
         strcpy(srvtypes, pcSrvTypes);
         handle->collatedsrvtypes = srvtypes;
      }
   }
   return SLP_TRUE;
}

/** SLPFindSrvTypes callback routine for NetworkRqstRply.
 *
 * @param[in] errorcode - The network operation error code.
 * @param[in] peerinfo - The network address of the responder.
 * @param[in] replybuf - The response buffer from the network request.
 * @param[in] cookie - Callback context data from ProcessSrvReg.
 *
 * @return SLP_FALSE (to stop any iterative callbacks).
 *
 * @internal
 */
static SLPBoolean ProcessSrvTypeRplyCallback(SLPError errorcode, 
      void * peerinfo, SLPBuffer replybuf, void * cookie)
{
   SLPMessage replymsg;
   SLPBoolean result = SLP_TRUE;
   SLPHandleInfo * handle = (SLPHandleInfo *)cookie;

   /* Check the errorcode and bail if it is set. */
   if (errorcode)
      return CollateToSLPSrvTypeCallback(handle, 0, errorcode);

   /* Parse the replybuf. */
   replymsg = SLPMessageAlloc();
   if (replymsg)
   {
      if (!SLPMessageParseBuffer(peerinfo, 0, replybuf, replymsg)
            && replymsg->header.functionid == SLP_FUNCT_SRVTYPERPLY 
            && !replymsg->body.srvtyperply.errorcode)
      {
         SLPSrvTypeRply * srvtyperply = &replymsg->body.srvtyperply;
         if (srvtyperply->srvtypelistlen)
            result = CollateToSLPSrvTypeCallback((SLPHandle)handle, 
                  srvtyperply->srvtypelist, srvtyperply->errorcode * -1);
      }
      SLPMessageFree(replymsg);
   }
   return result;
}

/** Formats and sends an SLPFindSrvTypes wire buffer request.
 *
 * @param handle - The OpenSLP session handle, containing request 
 *    parameters. See docs for SLPFindSrvTypes.
 *
 * @return Zero on success, or an SLP API error code.
 */
static SLPError ProcessSrvTypeRqst(SLPHandleInfo * handle)
{
   sockfd_t sock;
   uint8_t * buf;
   uint8_t * curpos;
   SLPError serr = SLP_OK;
   struct sockaddr_storage peeraddr;

/* 0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |   length of Naming Authority  |   <Naming Authority String>   \
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |     length of <scope-list>    |      <scope-list> String      \
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   /** @todo Ensure that we don't exceed the MTU. */

   buf = curpos = xmalloc(
         + 2 + handle->params.findsrvtypes.namingauthlen
         + 2 + handle->params.findsrvtypes.scopelistlen);
   if (buf == 0)
      return SLP_MEMORY_ALLOC_FAILED;

   /* Naming Authority */
   if (strcmp(handle->params.findsrvtypes.namingauth, "*") == 0)
      PutUINT16(&curpos, 0xffff); /* 0xffff is wildcard */
   else
   {
      PutUINT16(&curpos, handle->params.findsrvtypes.namingauthlen);
      memcpy(curpos, handle->params.findsrvtypes.namingauth,
            handle->params.findsrvtypes.namingauthlen);
      curpos += handle->params.findsrvtypes.namingauthlen;
   }

   /* <scope-list> */
   PutUINT16(&curpos, handle->params.findsrvtypes.scopelistlen);
   memcpy(curpos, handle->params.findsrvtypes.scopelist,
         handle->params.findsrvtypes.scopelistlen);
   curpos += handle->params.findsrvtypes.scopelistlen;

   /* Send request, receive reply. */
   do
   {
#ifndef UNICAST_NOT_SUPPORTED
      if (handle->dounicast == 1) 
      {
         serr = NetworkUcastRqstRply(handle, buf, SLP_FUNCT_SRVTYPERQST,
               curpos - buf, ProcessSrvTypeRplyCallback, handle);
         break;
      }
#endif
      sock = NetworkConnectToDA(handle, 
            handle->params.findsrvtypes.scopelist,
            handle->params.findsrvtypes.scopelistlen, 
            &peeraddr);

      if (sock == SLP_INVALID_SOCKET)
      {
         serr = NetworkMcastRqstRply(handle, buf, SLP_FUNCT_SRVTYPERQST, 
               curpos - buf, ProcessSrvTypeRplyCallback, 0);
         break;
      }
      serr = NetworkRqstRply(sock, &peeraddr, handle->langtag, 0, buf,
            SLP_FUNCT_SRVTYPERQST, curpos - buf, ProcessSrvTypeRplyCallback,
            handle);

      if (serr)
         NetworkDisconnectDA(handle);

   } while (serr == SLP_NETWORK_ERROR);

   xfree(buf);
   return serr;
}                                   

#ifdef ENABLE_ASYNC_API
/** Thread start procedure for asynchronous service type request.
 *
 * @param[in,out] handle - Contains the request parameters, returns the
 *    request result.
 *
 * @return An SLPError code.
 *
 * @internal
 */
static SLPError AsyncProcessSrvTypeRqst(SLPHandleInfo * handle)
{
   SLPError result = ProcessSrvTypeRqst(handle);
   xfree(handle->params.findsrvtypes.namingauth);
   xfree(handle->params.findsrvtypes.scopelist);
   handle->inUse = SLP_FALSE;
   return result;
}
#endif

/** Return a list of service types available on the network.
 *
 * The SLPFindSrvType function issues an SLP service type request for
 * service types in the scopes indicated by the @p pcScopeList. The
 * results are returned through the @p callback parameter. The service
 * types are independent of language locale, but only for services
 * registered in one of scopes and for the indicated naming authority.
 *
 * @par
 * If the naming authority is "*", then results are returned for all
 * naming authorities. If the naming authority is the empty string,
 * i.e. "", then the default naming authority, "IANA", is used. "IANA"
 * is not a valid naming authority name, and it is a PARAMETER_BAD error
 * to include it explicitly.
 *
 * @par
 * The service type names are returned with the naming authority intact.
 * If the naming authority is the default (i.e. empty string) then it
 * is omitted, as is the separating ".". Service type names from URLs
 * of the service: scheme are returned with the "service:" prefix
 * intact. [RFC 2608] See [RFC 2609] for more information on the 
 * syntax of service type names.
 *
 * @param[in] hSLP - The SLPHandle on which to search for types.
 * @param[in] pcNamingAuthority - The naming authority to search. Use "*" 
 *    for all naming authorities and the empty string, "", for the default 
 *    naming authority.
 * @param[in] pcScopeList - A pointer to a char containing comma separated 
 *    list of scope names to search for service types. May not be the empty
 *    string, "".
 * @param[in] callback - A callback function through which the results of 
 *    the operation are reported.
 * @param[in] pvCookie - Memory passed to the @p callback code from the 
 *    client. May be NULL.
 *
 * @return If an error occurs in starting the operation, one of the 
 *    SLPError codes is returned.
 */
SLPError SLPAPI SLPFindSrvTypes(
      SLPHandle hSLP,
      const char * pcNamingAuthority,
      const char * pcScopeList,
      SLPSrvTypeCallback callback,
      void * pvCookie)
{
   bool inuse;
   SLPError serr;
   SLPHandleInfo * handle = hSLP;

   /* Check for invalid parameters. */
   SLP_ASSERT(handle != 0);
   SLP_ASSERT(handle->sig == SLP_HANDLE_SIG);
   SLP_ASSERT(pcNamingAuthority != 0);
   SLP_ASSERT(strcmp(pcNamingAuthority, "IANA") != 0);
   SLP_ASSERT(callback != 0);

   if (handle == 0 || handle->sig != SLP_HANDLE_SIG
         || pcNamingAuthority == 0 
         || strcmp(pcNamingAuthority, "IANA") == 0
         || callback == 0)
      return SLP_PARAMETER_BAD;

   /* Check to see if the handle is in use. */
   inuse = SLPTryAcquireSpinLock(&handle->inUse);
   SLP_ASSERT(!inuse);
   if (inuse)
      return SLP_HANDLE_IN_USE;

   /* Get a scope list if none was specified. */
   if (pcScopeList == 0 || *pcScopeList == 0)
      pcScopeList = SLPGetProperty("net.slp.useScopes");

   /* Set the handle up to reference parameters. */
   handle->params.findsrvtypes.namingauthlen = strlen(pcNamingAuthority);
   handle->params.findsrvtypes.namingauth = pcNamingAuthority;
   handle->params.findsrvtypes.scopelistlen = strlen(pcScopeList); 
   handle->params.findsrvtypes.scopelist = pcScopeList;
   handle->params.findsrvtypes.callback = callback;
   handle->params.findsrvtypes.cookie = pvCookie; 

   /* Check to see if we should be async or sync. */
#ifdef ENABLE_ASYNC_API
   if (handle->isAsync)
   {
      /* Copy all the referenced parameters. */
      handle->params.findsrvtypes.namingauth = 
            xstrdup(handle->params.findsrvtypes.namingauth);
      handle->params.findsrvtypes.scopelist = 
            xstrdup(handle->params.findsrvtypes.scopelist);

      /* Ensure strdups and thread create succeed. */
      if (handle->params.findsrvtypes.namingauth == 0
            || handle->params.findsrvtypes.scopelist == 0
            || (handle->th = ThreadCreate((ThreadStartProc)
                  AsyncProcessSrvTypeRqst, handle)) == 0)
      {
         serr = SLP_MEMORY_ALLOC_FAILED;
         xfree(handle->params.findsrvtypes.namingauth);
         xfree(handle->params.findsrvtypes.scopelist);
         SLPReleaseSpinLock(&handle->inUse);
      }
      return serr;
   }
#endif

   /* Reference all parameters. */
   serr = ProcessSrvTypeRqst(handle);
   SLPReleaseSpinLock(&handle->inUse);
   return serr;
}

/*=========================================================================*/
