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

/** Open and close handle.
 *
 * Implementation for functions open, close, and associate ifc.
 *
 * @file       libslp_handle.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    LibSLPCode
 */

#include "slp.h"
#include "libslp.h"
#include "slp_net.h"
#include "slp_xmalloc.h"
#include "slp_xid.h"
#include "slp_message.h"
#include "slp_property.h"

static bool s_HandlesInitialized = false; /*!< Indicates we're fully initialized. */
static intptr_t s_OpenSLPHandleCount = 0; /*!< Tracks OpenSLP handles. */

/** Initialize the User Agent library. 
 *
 * Initializes the network sub-system if required, the debug memory 
 * allocator, and the transaction id (XID) sub-system. 
 *
 * @return An OpenSLP API error code.
 * 
 * @internal
 */
static SLPError InitUserAgentLibrary(void)
{
   /* Initialize the system if this is the first handle opened. */
   if (SLPAtomicInc(&s_OpenSLPHandleCount) == 1)
   {
      if (LIBSLPPropertyInit(LIBSLP_CONFFILE) != 0)
      {
         SLPAtomicDec(&s_OpenSLPHandleCount);
         return SLP_MEMORY_ALLOC_FAILED;
      }
#ifdef _WIN32
      {
         WSADATA wsaData; 
         WORD wVersionRequested = MAKEWORD(1,1); 
         if (WSAStartup(wVersionRequested, &wsaData) != 0)
         {
            SLPAtomicDec(&s_OpenSLPHandleCount);
            return SLP_NETWORK_INIT_FAILED;
         }
      }
#endif
#ifdef DEBUG
      xmalloc_init("libslp_xmalloc.log", 0);
#endif
      SLPXidSeed();
      s_HandlesInitialized = true;
   }
   else 
   {
      /* wait for first thread to finish before going on */
      while (!s_HandlesInitialized) 
         sleep(0);
   }
   return SLP_OK;
}

/** Cleans up the User Agent library. 
 *
 * Deinitializes the network sub-system if required, and the debug
 * memory allocator.
 * 
 * @internal
 */
static void ExitUserAgentLibrary(void)
{
   if (SLPAtomicDec(&s_OpenSLPHandleCount) == 0)
   {
      KnownDAFreeAll();
#ifdef DEBUG
      xmalloc_deinit();
#endif
#ifdef _WIN32
      WSACleanup();
#endif
      s_HandlesInitialized = false;
   }
}

/** Open an OpenSLP session handle.
 *
 * Returns a SLPHandle handle in the phSLP parameter for the language
 * locale passed in as the pcLang parameter. The client indicates if
 * operations on the handle are to be synchronous or asynchronous
 * through the isAsync parameter. The handle encapsulates the language
 * locale for SLP requests issued through the handle, and any other
 * resources required by the implementation. However, SLP properties
 * are not encapsulated by the handle; they are global. The return
 * value of the function is an SLPError code indicating the status of
 * the operation. Upon failure, the phSLP parameter is NULL.
 *
 * @par
 * An SLPHandle can only be used for one SLP API operation at a time.
 * If the original operation was started asynchronously, any attempt
 * to start an additional operation on the handle while the original
 * operation is pending results in the return of an SLP_HANDLE_IN_USE
 * error from the API function. The SLPClose() API function terminates
 * any outstanding calls on the handle. If an implementation is unable
 * to support a asynchronous (resp. synchronous) operation, due to
 * memory constraints or lack of threading support, the
 * SLP_NOT_IMPLEMENTED flag may be returned when the isAsync flag
 * is SLP_TRUE (resp. SLP_FALSE).
 *
 * @param[in] pcLang -  A pointer to an array of characters containing 
 *    the [RFC 1766] Language Tag for the natural language locale of 
 *    requests and registrations issued on the handle. (Pass NULL or
 *    the empty string to use the default locale.)
 *
 * @param[in] isAsync - An SLPBoolean indicating whether the SLPHandle 
 *    should be opened for asynchronous operation or not.
 *
 * @param[out] phSLP - A pointer to an SLPHandle, in which the open  
 *    SLPHandle is returned. If an error occurs, the value upon return 
 *    is NULL.
 *
 * @return An SLPError code; SLP_OK(0) on success, SLP_PARAMETER_BAD,
 *    SLP_NOT_IMPLEMENTED, SLP_MEMORY_ALLOC_FAILED, 
 *    SLP_NETWORK_INIT_FAILED, SLP_INTERNAL_SYSTEM_ERROR
 */
SLPEXP SLPError SLPAPI SLPOpen(
      const char *   pcLang,
      SLPBoolean     isAsync,
      SLPHandle *    phSLP)
{
   SLPError serr;
   SLPHandleInfo * handle;

   /* Check for invalid parameters. */
   SLP_ASSERT(phSLP != 0);

   if (phSLP == 0)
      return SLP_PARAMETER_BAD;

#ifndef ENABLE_ASYNC_API
   if (isAsync)
      return SLP_NOT_IMPLEMENTED;
#endif

   *phSLP = 0;

   serr = InitUserAgentLibrary();
   if (serr != SLP_OK)
      return serr;

   /* Allocate and clear an SLPHandleInfo structure. */
   handle = xcalloc(1, sizeof(SLPHandleInfo));
   if (handle == 0)
   {
      ExitUserAgentLibrary();
      return SLP_MEMORY_ALLOC_FAILED;
   }

   handle->sig = SLP_HANDLE_SIG;
   handle->inUse = 0;

#ifdef ENABLE_ASYNC_API
   handle->isAsync = isAsync;
#endif

   handle->dasock = SLP_INVALID_SOCKET;
   handle->sasock = SLP_INVALID_SOCKET;

#ifndef UNICAST_NOT_SUPPORTED
   handle->unicastsock = SLP_INVALID_SOCKET;
#endif

   /* Set the language tag. */
   if (pcLang == 0 || *pcLang == 0)
      pcLang = SLPPropertyGet("net.slp.locale", 0, 0);

   handle->langtaglen = strlen(pcLang);
   handle->langtag = xmemdup(pcLang, handle->langtaglen + 1);
   if (handle->langtag == 0)
   {
      xfree(handle);
      ExitUserAgentLibrary();
      return SLP_MEMORY_ALLOC_FAILED;
   }

#ifdef ENABLE_SLPv2_SECURITY
   handle->hspi = SLPSpiOpen(LIBSLP_SPIFILE, 0);
   if (!handle->hspi)
   {
      xfree(handle->langtag);
      xfree(handle);
      ExitUserAgentLibrary();
      return SLP_INTERNAL_SYSTEM_ERROR;
   }
#endif

   *phSLP = handle;

   return SLP_OK;
}

/** Close an SLP handle.
 *
 * @param[in] hSLP - The handle to be closed.
 */
SLPEXP void SLPAPI SLPClose(SLPHandle hSLP)
{
   SLPHandleInfo * handle = hSLP;

   /* Check for invalid parameters. */
   SLP_ASSERT(handle != 0);
   SLP_ASSERT(handle->sig == SLP_HANDLE_SIG);

   if (!handle || handle->sig != SLP_HANDLE_SIG)
      return;

#ifdef ENABLE_ASYNC_API
   if (handle->isAsync)
      SLPThreadWait(handle->th);
#endif

   SLP_ASSERT(handle->inUse == 0);

#ifdef ENABLE_SLPv2_SECURITY
   if (handle->hspi) 
      SLPSpiClose(handle->hspi);
#endif

   if (handle->langtag)
      xfree(handle->langtag);

#ifndef UNICAST_NOT_SUPPORTED
   xfree(handle->unicastscope);
   if (handle->unicastsock != SLP_INVALID_SOCKET)
      closesocket(handle->dasock);
#endif

   xfree(handle->sascope);
   if (handle->sasock != SLP_INVALID_SOCKET)
      closesocket(handle->sasock);

   xfree(handle->dascope);
   if (handle->dasock != SLP_INVALID_SOCKET)
      closesocket(handle->dasock);

   handle->sig = 0;
   xfree(handle);
   ExitUserAgentLibrary();
}

/** Associates an interface list with an SLP handle.
 *
 * Associates a list of interfaces @p McastIFList on which multicast needs
 * to be done with a particular SLPHandle @p hSLP. @p McastIFList is a comma 
 * separated list of host interface IP addresses.
 *
 * @param[in] hSLP - The SLPHandle with which the interface list is to be 
 *    associated with.
 *
 * @param[in] McastIFList - A comma separated list of host interface IP 
 *    addresses on which multicast needs to be done.
 *
 * @return An SLPError code.
 */
SLPEXP SLPError SLPAPI SLPAssociateIFList(
      SLPHandle hSLP, 
      const char * McastIFList)
{
#ifndef MI_NOT_SUPPORTED
   SLPHandleInfo * handle;

   SLP_ASSERT(hSLP != 0);
   SLP_ASSERT(*(unsigned int *)hSLP == SLP_HANDLE_SIG);
   SLP_ASSERT(McastIFList != 0);
   SLP_ASSERT(*McastIFList != 0);

   /* check for invalid parameters */
   if (!hSLP || *(unsigned int*)hSLP != SLP_HANDLE_SIG 
         || !McastIFList || !*McastIFList)
      return SLP_PARAMETER_BAD;

   handle = (SLPHandleInfo *)hSLP;

   /** @todo Copy the interface list, rather than just assign it. */

   handle->McastIFList = McastIFList;

   return SLP_OK;
#else
   (void)hSLP;
   (void)McastIFList;
   return SLP_NOT_IMPLEMENTED;
#endif   /* ! MI_NOT_SUPPORTED */
}

/** Associates a unicast IP address with an open SLP handle.
 *
 * Associates an IP address @p unicast_ip with a particular SLPHandle 
 * @p hSLP. @p unicast_ip is the IP address of the SA/DA from which service
 * is requested.
 *
 * @param[in] hSLP - The SLPHandle with which the @p unicast_ip address is 
 *    to be associated.
 *
 * @param[in] unicast_ip - IP address of the SA/DA from which service 
 *    is requested.
 *
 * @return An SLPError code.
 */
SLPEXP SLPError SLPAPI SLPAssociateIP(
      SLPHandle hSLP, 
      const char * unicast_ip)
{
#ifndef UNICAST_NOT_SUPPORTED
   SLPHandleInfo * handle;
   int result;

   SLP_ASSERT(hSLP != 0);
   SLP_ASSERT(*(unsigned int *)hSLP == SLP_HANDLE_SIG);
   SLP_ASSERT(unicast_ip != 0);
   SLP_ASSERT(*unicast_ip != 0);

   /* check for invalid parameters */
   if (!hSLP || *(unsigned int*)hSLP != SLP_HANDLE_SIG
         || !unicast_ip || !*unicast_ip)
      return SLP_PARAMETER_BAD;

   handle = (SLPHandleInfo *)hSLP;
   handle->dounicast = SLP_TRUE;

   /** @todo Verify error conditions in associate ip address. */

   result = SLPNetResolveHostToAddr(unicast_ip, &handle->ucaddr);
   if (SLPNetSetPort(&handle->ucaddr, (uint16_t)SLPPropertyAsInteger("net.slp.port")) != 0)
      return SLP_PARAMETER_BAD;

   return SLP_OK;
#else
   (void)hSLP;
   (void)unicast_ip;
   return SLP_NOT_IMPLEMENTED;
#endif   /* ! UNICAST_NOT_SUPPORTED */
}

/*=========================================================================*/
