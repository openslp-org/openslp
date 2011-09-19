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

/** Register a service.
 *
 * Implementation for functions register and deregister services -- 
 * SLPReg() call.
 *
 * @file       libslp_reg.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    LibSLPCode
 */

#include "slp.h"
#include "libslp.h"
#include "slp_message.h"
#include "slp_property.h"
#include "slp_xmalloc.h"
#include "slp_pid.h"

/** SLPReg callback routine for NetworkRqstRply.
 *
 * @param[in] errorcode - The network operation SLPError result.
 * @param[in] peeraddr - The network address of the responder.
 * @param[in] replybuf - The response buffer from the network request.
 * @param[in] cookie - Callback context data from ProcessSrvReg.
 *
 * @return SLP_FALSE (to stop any iterative callbacks).
 *
 * @note The SLPv2 wire protocol error codes are negated values of SLP
 * API error codes (SLPError values). Thus, the algorithm for converting 
 * from an SLPv2 wire protocol error to a client SLPReg error is to simply 
 * negate the wire protocol error value and cast the result to an SLPError.
 *
 * @todo Verify that no non-SLPError wire values make it back to this
 * routine, to be mis-converted into a non-existent SLPError value.
 *
 * @internal
 */
static SLPBoolean CallbackSrvReg(SLPError errorcode, 
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
   handle->params.reg.callback(handle, errorcode, 
         handle->params.reg.cookie);

   return SLP_FALSE;
}

/** Formats and sends an SLPReg wire buffer request.
 *
 * @param handle - The OpenSLP session handle, containing request 
 *    parameters. See docs for SLPReg.
 *
 * @return Zero on success, or an SLP API error code.
 * 
 * @internal
 */
static SLPError ProcessSrvReg(SLPHandleInfo * handle)
{
   sockfd_t sock;
   uint8_t * buf;
   uint8_t * curpos;
   SLPError serr;
   size_t extoffset = 0;
   int urlauthlen = 0;
   uint8_t * urlauth = 0;
   int attrauthlen = 0;
   uint8_t * attrauth = 0;
   SLPBoolean watchRegPID;
   struct sockaddr_storage saaddr;

#ifdef ENABLE_SLPv2_SECURITY
   if (SLPPropertyAsBoolean("net.slp.securityEnabled"))
   {
      int err = SLPAuthSignUrl(handle->hspi, 0, 0, handle->params.reg.urllen,
            handle->params.reg.url, &urlauthlen, &urlauth);
      if (err == 0)
         err = SLPAuthSignString(handle->hspi, 0, 0,
               handle->params.reg.attrlistlen, handle->params.reg.attrlist,
               &attrauthlen, &attrauth);
      if (err != 0)
         return SLP_AUTHENTICATION_ABSENT;
   }
#endif

   /* Should we send the "Watch Registration PID" extension? */
   watchRegPID = SLPPropertyAsBoolean("net.slp.watchRegistrationPID");

/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          <URL-Entry>                          \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | length of service type string |        <service-type>         \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     length of <scope-list>    |         <scope-list>          \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  length of attr-list string   |          <attr-list>          \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |# of AttrAuths |(if present) Attribute Authentication Blocks...\
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
   |    WATCH-PID extension ID     |    next extension offset      \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
   |  nxo (cont)   |     process identifier to be monitored        \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
   |  PID (cont)   |
   +-+-+-+-+-+-+-+-+ */

   buf = curpos = xmalloc(
         + SizeofURLEntry(handle->params.reg.urllen, urlauthlen)
         + 2 + handle->params.reg.srvtypelen
         + 2 + handle->params.reg.scopelistlen
         + 2 + handle->params.reg.attrlistlen
         + 1 + attrauthlen 
         + (watchRegPID? (2 + 3 + 4): 0));
   if (buf == 0)
   {
      xfree(urlauth);
      xfree(attrauth);
      return SLP_MEMORY_ALLOC_FAILED;
   }

   /* URL entry */
   PutURLEntry(&curpos, handle->params.reg.lifetime, handle->params.reg.url, 
         handle->params.reg.urllen, urlauth, urlauthlen);

   /* <service-type> */
   PutL16String(&curpos, handle->params.reg.srvtype, 
         handle->params.reg.srvtypelen);

   /* <scope-list> */
   PutL16String(&curpos, handle->params.reg.scopelist,
         handle->params.reg.scopelistlen);

   /* <attr-list> */
   PutL16String(&curpos, handle->params.reg.attrlist, 
         handle->params.reg.attrlistlen);

   /** @todo Handle multiple attribute authentication blocks. */

   /* Attribute Authentication Blocks */
   *curpos++ = attrauth? 1: 0;
   memcpy(curpos, attrauth, attrauthlen);
   curpos += attrauthlen;

   /* SLP_EXTENSION_ID_REG_PID */
   if (watchRegPID)
   {
      extoffset = curpos - buf;

      /** @todo In some future code base, this should be changed to use the 
       * non-deprecated official version, SLP_EXTENSION_ID_REG_PID. For now
       * we need to use the EXPerimental version in order to interoperate
       * properly with OpenSLP 1.x SA's.
       */

      PutUINT16(&curpos, SLP_EXTENSION_ID_REG_PID_EXP);
      PutUINT24(&curpos, 0);
      PutUINT32(&curpos, SLPPidGet());
   }

   /* Call the Request-Reply engine. */
   sock = NetworkConnectToSA(handle, handle->params.reg.scopelist,
         handle->params.reg.scopelistlen, &saaddr);
   if (sock != SLP_INVALID_SOCKET)
   {
      serr = NetworkRqstRply(sock, &saaddr, handle->langtag, extoffset,
            buf, SLP_FUNCT_SRVREG, curpos - buf, CallbackSrvReg, handle, false);
      if (serr)
         NetworkDisconnectSA(handle);
   }
   else
      serr = SLP_NETWORK_INIT_FAILED;    

   xfree(buf);
   xfree(urlauth);
   xfree(attrauth);

   return serr;
}

#ifdef ENABLE_ASYNC_API
/** Thread start procedure for asynchronous service registration.
 *
 * @param[in] handle - Contains the request parameters.
 *
 * @return An SLPError code.
 *
 * @internal
 */
static SLPError AsyncProcessSrvReg(SLPHandleInfo * handle)
{
   SLPError serr = ProcessSrvReg(handle);
   xfree((void *)handle->params.reg.url);
   xfree((void *)handle->params.reg.srvtype);
   xfree((void *)handle->params.reg.scopelist);
   xfree((void *)handle->params.reg.attrlist);
   SLPSpinLockRelease(&handle->inUse);
   return serr;
}
#endif

/** Register a service URL through OpenSLP.
 *
 * Registers the URL in @p pcSrvURL having the lifetime usLifetime with
 * the attribute list in pcAttrs. The @p pcAttrs list is a comma separated
 * list of attribute assignments in the wire format (including escaping
 * of reserved characters). The @p usLifetime parameter must be nonzero
 * and less than or equal to SLP_LIFETIME_MAXIMUM. If the @p fresh flag
 * is SLP_TRUE, then the registration is new (the SLP protocol FRESH flag
 * is set) and the registration replaces any existing registrations.
 * The @p pcSrvType parameter is a service type name and can be included
 * for service URLs that are not in the service: scheme. If the URL is
 * in the service: scheme, the @p pcSrvType parameter is ignored. If the
 * @p fresh flag is SLP_FALSE, then an existing registration is updated.
 * Rules for new and updated registrations, and the format for @p pcAttrs
 * and @p pcScopeList can be found in [RFC 2608]. Registrations and 
 * updates take place in the language locale of the @p hSLP handle.
 *
 * @par
 * The API library is required to perform the operation in all scopes
 * obtained through configuration.
 *
 * @param[in] hSLP - The language-specific SLPHandle on which to register
 *    the advertisement.
 * @param[in] srvUrl - The URL to register. May not be the empty string. 
 *    May not be NULL. Must conform to SLP Service URL syntax. 
 *    SLP_INVALID_REGISTRATION will be returned if not.
 * @param[in] lifetime - An unsigned short giving the life time of the 
 *    service advertisement, in seconds. The value must be an unsigned
 *    integer less than or equal to SLP_LIFETIME_MAXIMUM, and greater
 *    than zero. If SLP_LIFETIME_MAXIMUM is used, the registration 
 *    will remain for the life of the calling process. Also, OpenSLP,
 *    will not allow registrations to be made with SLP_LIFETIME_MAXIMUM 
 *    unless SLP_REG_FLAG_WATCH_PID is also used. Note that RFC 2614
 *    defines this parameter as const unsigned short. The 'const' is 
 *    superfluous.
 * @param[in] srvType - The service type. If @p pURL is a "service:" 
 *    URL, then this parameter is ignored. (ALWAYS ignored since the SLP 
 *    Service URL syntax required for the @p pcSrvURL encapsulates the 
 *    service type.)
 * @param[in] attrList -  A comma separated list of attribute assignment 
 *    expressions for the attributes of the advertisement. Use empty 
 *    string ("") for no attributes.
 * @param[in] fresh - An SLPBoolean that is SLP_TRUE if the registration 
 *    is new or SLP_FALSE if a RE-registration. Use of non-fresh 
 *    registrations is deprecated (post RFC 2614). SLP_TRUE must be 
 *    passed in for this parameter or SLP_BAD_PARAMETER will be returned. 
 *    It also appears that this flag has been overloaded to accept certain 
 *    specific values of "true" for internal purposes... (jmc)
 * @param[in] callback - A callback to report the operation completion 
 *    status.
 * @param[in] cookie - Memory passed to the callback code from the 
 *    client. May be NULL.
 *
 * @return If an error occurs in starting the operation, one of the 
 *    SLPError codes is returned.
 */
SLPEXP SLPError SLPAPI SLPReg(
      SLPHandle hSLP,
      const char * srvUrl,
      unsigned short lifetime,
      const char * srvType,
      const char * attrList,
      SLPBoolean fresh,
      SLPRegReport callback,
      void * cookie)
{
   bool inuse;
   SLPError serr;
   SLPSrvURL * parsedurl = 0;
   SLPHandleInfo * handle = hSLP;

   /** @todo Add code to accept non- "service:" scheme 
    * URL's - normalize with srvType parameter info. 
    */
   (void)srvType; /* not used yet */

   /* Check for invalid parameters. */
   SLP_ASSERT(handle != 0);
   SLP_ASSERT(handle->sig == SLP_HANDLE_SIG);
   SLP_ASSERT(srvUrl != 0);
   SLP_ASSERT(*srvUrl != 0);
   SLP_ASSERT(lifetime != 0);
   SLP_ASSERT(attrList != 0);
   SLP_ASSERT(callback != 0);

   if (handle == 0 || handle->sig != SLP_HANDLE_SIG 
         || srvUrl == 0 || *srvUrl == 0 
         || lifetime == 0
         || attrList == 0 
         || callback == 0)
      return SLP_PARAMETER_BAD;

   /** @todo Handle non-fresh registrations in SLPReg. */
   if (fresh == SLP_FALSE)
      return SLP_NOT_IMPLEMENTED;

   /* Check to see if the handle is in use. */
   inuse = SLPSpinLockTryAcquire(&handle->inUse);
   SLP_ASSERT(!inuse);
   if (inuse)
      return SLP_HANDLE_IN_USE;

   /* Parse the srvurl - mainly for service type info. */
   serr = SLPParseSrvURL(srvUrl, &parsedurl);
   if (serr)
   {
      SLPSpinLockRelease(&handle->inUse);
      return serr == SLP_PARSE_ERROR? SLP_INVALID_REGISTRATION: serr;
   }

   /* Set the handle up to reference parameters. */
   handle->params.reg.fresh = fresh;
   handle->params.reg.lifetime = lifetime;
   handle->params.reg.urllen = strlen(srvUrl);
   handle->params.reg.url = srvUrl;
   handle->params.reg.srvtype = parsedurl->s_pcSrvType;
   handle->params.reg.srvtypelen = strlen(handle->params.reg.srvtype);
   handle->params.reg.scopelist = SLPPropertyGet("net.slp.useScopes", 0, 0);
   handle->params.reg.scopelistlen = strlen(handle->params.reg.scopelist);
   handle->params.reg.attrlistlen = strlen(attrList);
   handle->params.reg.attrlist = attrList;
   handle->params.reg.callback = callback;
   handle->params.reg.cookie = cookie; 

#ifdef ENABLE_ASYNC_API
   if (handle->isAsync)
   {
      /* Copy all of the referenced parameters before creating thread. */
      handle->params.reg.url = xstrdup(handle->params.reg.url);
      handle->params.reg.srvtype = xstrdup(handle->params.reg.url);
      handle->params.reg.scopelist = xstrdup(handle->params.reg.scopelist);
      handle->params.reg.attrlist = xstrdup(handle->params.reg.attrlist);

      /* Ensure strdups and thread create succeed. */
      if (handle->params.reg.url == 0
            || handle->params.reg.srvtype == 0
            || handle->params.reg.scopelist == 0
            || handle->params.reg.attrlist == 0
            || (handle->th = SLPThreadCreate((SLPThreadStartProc)
                  AsyncProcessSrvReg, handle)) == 0)
      {
         serr = SLP_MEMORY_ALLOC_FAILED;    
         xfree((void *)handle->params.reg.url);
         xfree((void *)handle->params.reg.srvtype);
         xfree((void *)handle->params.reg.scopelist);
         xfree((void *)handle->params.reg.attrlist);
         SLPSpinLockRelease(&handle->inUse);
      }
   }
   else
#endif
   {
      /* Reference all the parameters. */
      serr = ProcessSrvReg(handle);            
      SLPSpinLockRelease(&handle->inUse);
   }
   SLPFree(parsedurl);
   return serr;
}

/*=========================================================================*/
