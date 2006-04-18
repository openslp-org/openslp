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

/** Module static flag indicating property module is initialized */
static bool s_PropInited = 0;

/** Module static spinlock variable protects property module initialization */
static intptr_t s_PropInitLock = 0;

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
 * 
 * @remarks Since SLPOpen (through InitUserAgentLibrary) now atomically
 *    checks the open handle count, the properties are now initialized
 *    there.  Especially since SLPClose already cleans up those properties.
 */
SLPEXP const char * SLPAPI SLPGetProperty(const char * pcName)
{
   SLP_ASSERT(pcName && *pcName);
   if (!pcName || !*pcName)
      return 0;

   /*  The following is commented out because SLPOpen handles this
   if (!s_PropInited)
   {
      SLPAcquireSpinLock(&s_PropInitLock);
      if (!s_PropInited)
      {
         SLPPropertyInit(0);
         s_PropInited = true;
      }
      SLPReleaseSpinLock(&s_PropInitLock);
   }
   */
   return SLPPropertyGet(pcName);
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
 * @remarks This code is currently not implemented because there's no good
 *    way for the library to know when it's safe to delete an old stored 
 *    value, and replace it with the new value, specified in @p pcValue. 
 *    While clients aren't technically supposed to save off pointers into
 *    the configuration store for later use, doing so is not prohibited.
 *    Regardless, even if they don't store the value pointer, there are 
 *    still multi-threaded race conditions that can't be solved with the
 *    current design.
 *
 * @remarks We could implement this by maintaining a list of all old values
 *    so they were always valid for the life of the application, but that
 *    could be expensive, memory-wise, depending on the nature of the 
 *    application using this library. In pathological cases, the property 
 *    memory store could grow without bounds.
 */
SLPEXP void SLPAPI SLPSetProperty(
      const char * pcName, 
      const char * pcValue)
{
   SLP_ASSERT(pcName && *pcName);
   if (!pcName || !*pcName)
      return;

   (void)pcValue;

   /* The following is commented out for threading reasons 

   if (!s_PropInited)
   {
      SLPAcquireSpinLock(&s_PropInitLock);
      if (!s_PropInited)
      {
         SLPPropertyInit(0);
         s_PropInited = true;
      }
      SLPReleaseSpinLock(&s_PropInitLock);
   }
   SLPPropertySet(pcName, pcValue);

   */
}

/*=========================================================================*/
