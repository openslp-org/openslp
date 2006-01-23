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

/** Header file for common string parsing functionality.
 *
 * @file       slp_parse.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#ifndef SLP_PARSE_H_INCLUDED
#define SLP_PARSE_H_INCLUDED

/*!@defgroup CommonCodeParse String Parsing */

/*!@addtogroup CommonCodeParse
 * @ingroup CommonCode
 * @{
 */

#include "slp_types.h"

/** A parsed Universal Resource Locator (URL).
 */
typedef struct _SLPParsedSrvUrl
{
   /** The service type.
    *
    * A pointer to a character string containing the service
    * type name, including naming authority.  The service type
    * name includes the "service:" if the URL is of the service:
    * scheme.
    */
   char * srvtype;

   /** The host name.
    *
    * A pointer to a character string containing the host
    * identification information.
    */
   char * host;

   /** The port number.
    *
    * The port number, or zero if none.  The port is only available
    * if the transport is IP.
    *
    */
   uint16_t port;

   /** The address family identifier.
    *
    * A pointer to a character string containing the network address
    * family identifier.  Possible values are "ipx" for the IPX
    * family, "at" for the Appletalk family, and "" (i.e. the empty
    * string) for the IP address family.
    */
   char * family;

   /** Everything following the "scheme://hostname:port/"
    *
    * The remainder of the URL, after the host identification.
    */
   char * remainder;
} SLPParsedSrvUrl;

int SLPParseSrvUrl(size_t srvurllen, const char * srvurl, 
      SLPParsedSrvUrl ** parsedurl);

/*! @} */

#endif   /* SLP_PARSE_H_INCLUDED */

/*=========================================================================*/
