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

/** Delete attributes.
 *
 * Implementation for SLPDelAttrs() call.
 *
 * @file       libslp_delattrs.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    LibSLPCode
 */

#include "slp.h"
#include "libslp.h"

/** Delete selected attributes.
 *
 * Delete the selected attributes in the locale of the SLPHandle. The
 * API library is required to perform the operation in all scopes
 * obtained through configuration.
 *
 * @param[in] hSLP - The language specific SLPHandle to use for deleting 
 *    attributes.
 * @param[in] pcURL - The URL of the advertisement from which the 
 *    attributes should be deleted. May not be the empty string.
 * @param[in] pcAttrs - A comma separated list of attribute ids for the 
 *    attributes to deregister. See Section 9.8 in [RFC 2608] for a 
 *    description of the list format. May not be the empty string.
 * @param[in] callback - A callback to report the operation completion 
 *    status.
 * @param[in] pvCookie - Memory passed to the callback code from the 
 *    client. May be 0.
 *
 * @return If an error occurs in starting the operation, one of the 
 *    SLPError codes is returned.
 */
SLPEXP SLPError SLPAPI SLPDelAttrs(SLPHandle hSLP, const char * pcURL,
      const char * pcAttrs, SLPRegReport callback, void * pvCookie)
{
   SLPHandleInfo * handle = hSLP;

   SLP_ASSERT(handle != 0);
   SLP_ASSERT(handle->sig == SLP_HANDLE_SIG);
   SLP_ASSERT(pcURL != 0);
   SLP_ASSERT(*pcURL != 0);
   SLP_ASSERT(pcAttrs != 0);
   SLP_ASSERT(*pcAttrs != 0);
   SLP_ASSERT(callback != 0);

   /* Check for invalid parameters. */
   if (handle == 0 || handle->sig != SLP_HANDLE_SIG
         || pcURL == 0 || *pcURL == 0
         || pcAttrs == 0 || *pcAttrs == 0
         || callback == 0)
      return SLP_PARAMETER_BAD;

   (void)pvCookie;   /* nothing else for now */

   return SLP_NOT_IMPLEMENTED;
}

/*=========================================================================*/
