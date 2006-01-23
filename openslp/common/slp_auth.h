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

/** Header file for OpenSLP's SLPv2 authentication implementation. 
 *
 * Currently only BSD 0x0002 (DSA-SHA1) is supported
 *
 * @file       slp_auth.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#ifndef SLP_AUTH_H_INCLUDED
#define SLP_AUTH_H_INCLUDED

#include "slp_message.h"
#include "slp_spi.h"

/*!@defgroup CommonCodeAuth Authentication */

/*!@addtogroup CommonCodeAuth
 * @ingroup CommonCode
 * @{
 */

#ifdef ENABLE_SLPv2_SECURITY

#define SLPAUTH_SHA1_DIGEST_SIZE    20

int SLPAuthVerifyString(SLPSpiHandle hspi, int emptyisfail, 
      unsigned short stringlen, const char * string, int authcount, 
      const SLPAuthBlock * autharray);

int SLPAuthVerifyUrl(SLPSpiHandle hspi, int emptyisfail,
      const SLPUrlEntry * urlentry);

int SLPAuthVerifyDAAdvert(SLPSpiHandle hspi, int emptyisfail, 
      const SLPDAAdvert * daadvert);

int SLPAuthVerifySAAdvert(SLPSpiHandle hspi, int emptyisfail, 
      const SLPSAAdvert * saadvert);

int SLPAuthSignString(SLPSpiHandle hspi, int spistrlen, 
      const char * spistr, unsigned short stringlen, 
      const char * string, int * authblocklen,
      unsigned char ** authblock);

int SLPAuthSignUrl(SLPSpiHandle hspi, int spistrlen, const char * spistr,
      unsigned short urllen, const char * url, int * authblocklen,
      unsigned char ** authblock);

int SLPAuthSignDAAdvert(SLPSpiHandle hspi, unsigned short spistrlen, 
      const char * spistr, unsigned long bootstamp, unsigned short urllen,
      const char * url, unsigned short attrlistlen, const char * attrlist,
      unsigned short scopelistlen, const char * scopelist, 
      unsigned short daspistrlen, const char * daspistr, 
      int * authblocklen, unsigned char ** authblock);

int SLPAuthSignSAAdvert(unsigned short spistrlen, const char * spistr,
      unsigned short urllen, const char * url, unsigned short attrlistlen,
      const char * attrlist, unsigned short scopelistlen, 
      const char * scopelist, int * authblocklen, 
      unsigned char ** authblock);

#endif   /* ENABLE_SLPv2_SECURITY */

/*! @} */

#endif /* SLP_AUTH_H_INCLUDED */

/*=========================================================================*/
