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

/** Header file that defines SLP string and string-list routines.
 *
 * @file       slp_compare.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeStrings
 */

#ifndef SLP_STRING_H_INCLUDED
#define SLP_STRING_H_INCLUDED

/*!@defgroup CommonCodeUtility Utility
 * @ingroup CommonCode
 */

/*!@defgroup CommonCodeStrings String Compare
 * @ingroup CommonCodeUtility
 * @{
 */

#ifndef _WIN32
# ifndef HAVE_STRNCASECMP
int strncasecmp(const char * s1, const char * s2, size_t len);
# endif
# ifndef HAVE_STRCASECMP
int strcasecmp(const char * s1, const char * s2);
# endif
#endif

size_t SLPNormalizeString(size_t len, const char * srcstr, char * dststr, int trim);

int SLPCompareString(size_t str1len, const char * str1, 
      size_t str2len, const char * str2);

int SLPCompareNamingAuth(size_t srvtypelen, const char * srvtype, 
      size_t namingauthlen, const char * namingauth);

int SLPCompareSrvType(size_t srvtype1len, const char * srvtype1, 
      size_t srvtype2len, const char * srvtype2);

int SLPContainsStringList(size_t listlen, const char * list, 
      size_t stringlen, const char * string);

int SLPIntersectStringList(size_t list1len, const char * list1, 
      size_t list2len, const char * list2);

int SLPIntersectRemoveStringList(int list1len, const char* list1,
      int* list2len, char* list2);

int SLPUnionStringList(size_t list1len, const char * list1, size_t list2len, 
      const char * list2, size_t * unionlistlen, char * unionlist);

int SLPSubsetStringList(size_t listlen, const char * list, 
      size_t sublistlen, const char * sublist);

int SLPCheckServiceUrlSyntax(const char * srvurl, size_t srvurllen);

int SLPCheckAttributeListSyntax(const char * attrlist, size_t attrlistlen);

/*! @} */

#endif /* SLP_STRING_H_INCLUDED */

/*=========================================================================*/
