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

/** Property management.
 *
 * Implementation for SLPGetProperty() and SLPSetProperty() calls.
 *
 * @file       libslp_property.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    LibSLPCode
 */

#include "slp.h"
#include "libslp.h"
#include "slp_property.h"
#include "slp_atomic.h"

/** A flag that indicates that we can still allow calls to SLPSetProperty. */
static bool s_UserAllowedToSet = true;

/** A flag that indicates the property sub-system has been initialized. */
static bool s_PropInited = 0;

/** The libslp property sub-system initialization wrapper.
 * 
 * @param[in] gconffile - The global configuration file to use.
 * 
 * @return Zero on success, or a non-zero error value.
 * 
 * @internal
 */
int LIBSLPPropertyInit(char const * gconffile)
{
   static intptr_t s_PropInitLock = 0;

   int rv = 0;
   if (!s_PropInited)
   {
      SLPAcquireSpinLock(&s_PropInitLock);
      if (!s_PropInited && (rv = SLPPropertyInit(gconffile)) == 0);
         s_PropInited = true;
      SLPReleaseSpinLock(&s_PropInitLock);
   }
   SLP_ASSERT(rv == 0);
   return rv;
}

/** Returns a string value for a specified property.
 *
 * Returns the value of the corresponding SLP property name. The
 * returned string is owned by the library and MUST NOT be freed.
 *
 * @param[in] pcName - Null terminated string with the property name, 
 *    from Section 2.1 in [RFC 2614].
 *
 * @return If no error, returns a pointer to a character buffer 
 *    containing the property value. If the property was not set, 
 *    returns the default value. If an error occurs, returns NULL. 
 *    The returned string MUST NOT be freed.
 */
SLPEXP const char * SLPAPI SLPGetProperty(const char * pcName)
{
   SLP_ASSERT(pcName && *pcName);
   if (!pcName || !*pcName)
      return 0;

   /* This wrapper ensures that we only get initialized once */
   if (!s_PropInited && LIBSLPPropertyInit(LIBSLP_CONFFILE) != 0)
      return 0;

   /* At this point, the caller may no longer call SLPSetProperty because
    * we're about to return a pointer to internal memory, and we can no longer
    * guarantee thread-safety. This is a defect in RFC 2614 - it defines an 
    * interface that can't be made to be thread safe, so we allow setting 
    * until the first call to SLPGetProperty is detected here.
    */
   s_UserAllowedToSet = false;

   return SLPPropertyGet(pcName, 0, 0);
} 

/** Set a property value by name. 
 *
 * Sets the value of the SLP property to the new value. The pcValue
 * parameter should be the property value as a string.
 *
 * @param[in] pcName - Null terminated string with the property name, 
 *    from Section 2.1 in [RFC 2614].
 *
 * @param[in] pcValue - Null terminated string with the property value, 
 *    in UTF-8 character encoding.
 *
 * @remarks This function is implemented such that it can be successfully 
 *    called as long as SLPGetProperty has not yet been called in this 
 *    process. The reason for this is because there's no good way for the 
 *    library to know when it's safe to delete an old stored value, and 
 *    replace it with the new value, specified in @p pcValue. While clients 
 *    aren't technically supposed to save off pointers into the configuration 
 *    store for later use, doing so is not prohibited. Regardless, even if 
 *    they don't store the value pointer, there are still multi-threaded 
 *    race conditions that can't be solved with the current API design.
 */
SLPEXP void SLPAPI SLPSetProperty(const char * pcName, const char * pcValue)
{
   SLP_ASSERT(pcName && *pcName);
   if (!pcName || !*pcName)
      return;

   /* This wrapper ensures that we only get initialized once */
   if (!s_PropInited && LIBSLPPropertyInit(LIBSLP_CONFFILE) != 0)
      return;

   if (s_UserAllowedToSet)
   {
      int rv = SLPPropertySet(pcName, pcValue, true);
      SLP_ASSERT(rv == 0);
      (void)rv;   /* quite the compiler in release code */
   }
}

/** Set the application-specific configuration file full path name.
 *
 * Sets an application-specific configuration override file. The contents of
 * this property file will override the contents of the default or global 
 * UA configuration file (usually /etc/slp.conf or c:\windows\slp.conf).
 *
 * @param[in] pathname - Null terminated string containing the full path
 *    name of the application property override file. [POST RFC 2614]
 */
SLPEXP SLPError SLPAPI SLPSetAppPropertyFile(const char * pathname)
{
   SLP_ASSERT(pathname && *pathname);
   if (!pathname || !*pathname)
      return SLP_PARAMETER_BAD;

   return SLPPropertySetAppConfFile(pathname)? SLP_PARAMETER_BAD: SLP_OK;
}

/*=========================================================================*/
