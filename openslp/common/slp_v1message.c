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

/** Header file that defines SLPv1 wire protocol message structures.
 *
 * This file contains buffer parsing routines specific to version 1 of 
 * the SLP wire protocol.
 *
 * @file       slp_v1message.c
 * @author     Matthew Peterson, Ganesan Rajagopal, 
 *             John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeMessageV1
 */

#include "slp_types.h"
#include "slp_v1message.h"
#include "slp_utf8.h"

/** Parse an SLPv1 URL entry.
 *
 * @param[in] buffer - The buffer containing the data to be parsed.
 * @param[in] encoding - The language encoding of the message.
 * @param[out] urlentry - The URL entry object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, SLP_ERROR_INTERNAL_ERROR (out of memory) or
 *    SLP_ERROR_PARSE_ERROR.
 *
 * @internal
 */
static int v1ParseUrlEntry(const SLPBuffer buffer, int encoding,
      SLPUrlEntry * urlentry)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Lifetime            |        Length of URL          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   \                              URL                              \
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |              (if present) URL Authentication Block .....
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   int result;

   /* Make sure that min size is met. */
   if (buffer->end - buffer->curpos < 6)
      return SLP_ERROR_PARSE_ERROR;

   /* No reserved stuff for SLPv1. */
   urlentry->reserved = 0;

   /* Parse out lifetime. */
   urlentry->lifetime = GetUINT16(&buffer->curpos);

   /* Parse out URL. */
   urlentry->urllen = GetUINT16(&buffer->curpos);
   urlentry->url = GetStrPtr(&buffer->curpos, urlentry->urllen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   result = SLPv1AsUTF8(encoding, (char *)urlentry->url,
         &urlentry->urllen); 
   if (result != 0)
      return result;

   /* We don't support auth blocks for SLPv1 - no one uses them anyway. */
   urlentry->authcount = 0;
   urlentry->autharray = 0;

   return 0;
}

/** Parse an SLPv1 Service Request message.
 *
 * @param[in] buffer - The buffer containing the data to be parsed.
 * @param[in] encoding - The language encoding of the message.
 * @param[out] srvrqst - The service request object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, SLP_ERROR_INTERNAL_ERROR (out of memory) or
 *    SLP_ERROR_PARSE_ERROR.
 *
 * @internal
 */
static int v1ParseSrvRqst(const SLPBuffer buffer, int encoding,
      SLPSrvRqst * srvrqst)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |length of prev resp list string|<Previous Responders Addr Spec>|
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   \                  <Previous Responders Addr Spec>              \
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  length of predicate string   |  Service Request <predicate>  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   \               Service Request <predicate>, contd.             \
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
  
   char * tmp;
   int result;

   /* Enforce SLPv1 service request size limits. */
   if (buffer->end - buffer->curpos < 4)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the PRList. */
   srvrqst->prlistlen = GetUINT16(&buffer->curpos);
   srvrqst->prlist = GetStrPtr(&buffer->curpos, srvrqst->prlistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;
   if ((result = SLPv1AsUTF8(encoding, (char *) srvrqst->prlist,
         &srvrqst->prlistlen)) != 0)
      return result;

   /* Parse the predicate string. */
   srvrqst->predicatelen = GetUINT16(&buffer->curpos);
   srvrqst->predicate = GetStrPtr(&buffer->curpos, srvrqst->predicatelen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;
   if ((result = SLPv1AsUTF8(encoding, (char *) srvrqst->predicate,
         &srvrqst->predicatelen)) != 0)
      return result;

   /* Terminate the predicate string. */
   if(srvrqst->predicate)
      *(char *)&srvrqst->predicate[srvrqst->predicatelen] = 0; 

   /* Now split out the service type. */
   srvrqst->srvtype = srvrqst->predicate;
   tmp = strchr(srvrqst->srvtype, '/');
   if (!tmp)
      return SLP_ERROR_PARSE_ERROR;
   *tmp = 0;           /* Terminate service type string. */
   srvrqst->srvtypelen = tmp - srvrqst->srvtype;

   /* Parse out the predicate. */
   srvrqst->predicatever = 1;  /* SLPv1 predicate. */
   srvrqst->predicatelen -= srvrqst->srvtypelen + 1;
   srvrqst->predicate += srvrqst->srvtypelen + 1;

   /* Now split out the scope (if any). */
   if (*srvrqst->predicate == '/')
   {
      /* No scope, so set default scope. */
      srvrqst->scopelist = "DEFAULT";
      srvrqst->scopelistlen = 7;
      srvrqst->predicate++;
      srvrqst->predicatelen--;
   }
   else
   {
      srvrqst->scopelist = srvrqst->predicate;
      tmp = strchr(srvrqst->scopelist, '/');
      if (!tmp)
         return SLP_ERROR_PARSE_ERROR;
      *tmp = 0;   /* Terminate scope list. */

      srvrqst->scopelistlen = tmp - srvrqst->scopelist;
      srvrqst->predicate += srvrqst->scopelistlen + 1;
      srvrqst->predicatelen -= srvrqst->scopelistlen + 1;
   }
   srvrqst->predicatelen--;
   tmp = (char *)&srvrqst->predicate[srvrqst->predicatelen]; 
   *tmp = 0;   /* Terminate the predicate. */

   /* SLPv1 service requests don't have SPI strings. */
   srvrqst->spistrlen = 0;
   srvrqst->spistr = 0;

   return 0;
}

/** Parse an SLPv1 Service Registration message.
 *
 * @param[in] buffer - The buffer containing the data to be parsed.
 * @param[in] encoding - The language encoding of the message.
 * @param[out] srvreg - The service registration object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, SLP_ERROR_INTERNAL_ERROR (out of memory) or
 *    SLP_ERROR_PARSE_ERROR.
 *
 * @internal
 */
static int v1ParseSrvReg(const SLPBuffer buffer, int encoding,
      SLPSrvReg * srvreg)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   \                          <URL-Entry>                          \
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Length of Attr List String   |          <attr-list>          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   \                    <attr-list>, Continued.                    \
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    (if present) Attribute Authentication Block ...
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   char * tmp;
   int result;

   /* Parse the <URL-Entry>, and convert fields to UTF-8. */
   if ((result = v1ParseUrlEntry(buffer, encoding, &srvreg->urlentry)) != 0)
      return result;

   /* SLPv1 registration requests don't have a separate service type.
    * They must be parsed from the URL.
    */
   srvreg->srvtype = srvreg->urlentry.url;
   tmp = strstr(srvreg->srvtype, "://");
   if (!tmp)
      return SLP_ERROR_PARSE_ERROR;
   srvreg->srvtypelen = tmp - srvreg->srvtype;

   /* Parse the <attr-list>, and convert to UTF-8. */
   srvreg->attrlistlen = GetUINT16(&buffer->curpos);
   srvreg->attrlist = GetStrPtr(&buffer->curpos, srvreg->attrlistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;
   if ((result = SLPv1AsUTF8(encoding, (char *)srvreg->attrlist,
         &srvreg->attrlistlen)) != 0)
      return result;

   /* SLPv1 registration requests don't include a scope either.
    * The scope is included in the attribute list.
    */
   if ((tmp = strstr(srvreg->attrlist, "SCOPE")) != 0
         || (tmp = strstr(srvreg->attrlist, "scope")) != 0)
   {
      tmp += 5;
      while (*tmp && (isspace(*tmp) || *tmp == '='))
         tmp++;      /* Find start of scope string. */
      srvreg->scopelist = tmp;
      while (*tmp && !isspace(*tmp) && *tmp != ')')
         tmp++;      /* Find end of scope string. */
      srvreg->scopelistlen = tmp - srvreg->scopelist;
      /** @todo Should we convert to UTF-8 here? */
   }
   else
   {
      srvreg->scopelist = "DEFAULT";
      srvreg->scopelistlen = 7;
   }

   /* We don't support auth blocks for SLPv1 - no one uses them anyway. */
   srvreg->authcount = 0;
   srvreg->autharray = 0;

   return 0;
}

/** Parse an SLPv1 Service Deregister message.
 *
 * @param[in] buffer - The buffer containing the data to be parsed.
 * @param[in] encoding - The language encoding of the message.
 * @param[out] srvdereg - The service deregister object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, SLP_ERROR_INTERNAL_ERROR (out of memory) or
 *    SLP_ERROR_PARSE_ERROR.
 *
 * @internal
 */
static int v1ParseSrvDeReg(const SLPBuffer buffer, int encoding,
      SLPSrvDeReg * srvdereg)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         length of URL         |              URL              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   \              URL of Service to Deregister, contd.             \
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |             (if present) authentication block .....
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  length of <tag spec> string  |            <tag spec>         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   \                     <tag spec>, continued                     \
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   int result;

   /* Enforce SLPv1 service deregistration size limits. */
   if (buffer->end - buffer->curpos < 4)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the URL, and convert to UTF-8. */
   srvdereg->urlentry.urllen = GetUINT16(&buffer->curpos);
   srvdereg->urlentry.url = GetStrPtr(&buffer->curpos, 
         srvdereg->urlentry.urllen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;
   if ((result = SLPv1AsUTF8(encoding, (char *)srvdereg->urlentry.url,
         &srvdereg->urlentry.urllen)) != 0)
      return result;

   /* URL Entry reserved and lifetime fields don't exist in SLPv1. */
   srvdereg->urlentry.reserved = 0;
   srvdereg->urlentry.lifetime = 0;

   /* Almost no one used SLPv1 security, so we'll just ignore it for 
    * now, and hope we never have to parse an SLPv1 request with security
    * enabled.
    */

   /* Parse the <tag spec>, and convert to UTF-8. */
   srvdereg->taglistlen = GetUINT16(&buffer->curpos);
   srvdereg->taglist = GetStrPtr(&buffer->curpos, srvdereg->taglistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;
   if ((result = SLPv1AsUTF8(encoding, (char *)srvdereg->taglist,
         &srvdereg->taglistlen)) != 0)
      return result;

   /* SLPv1 deregistrations do not have a separate scope list. */
   srvdereg->scopelistlen = 0;
   srvdereg->scopelist = 0;

   return 0;
}

/** Parse an SLPv1 Attribute Request message.
 *
 * @param[in] buffer - The buffer containing the data to be parsed.
 * @param[in] encoding - The language encoding of the message.
 * @param[out] attrrqst - The attribute request object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, SLP_ERROR_INTERNAL_ERROR (out of memory) or
 *    SLP_ERROR_PARSE_ERROR.
 *
 * @internal
 */
static int v1ParseAttrRqst(const SLPBuffer buffer, int encoding,
      SLPAttrRqst * attrrqst)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |length of prev resp list string|<Previous Responders Addr Spec>|
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   \         <Previous Responders Addr Spec>, continued            \
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         length of URL         |              URL              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   \                         URL, continued                        \
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |        length of <Scope>      |           <Scope>             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   \                      <Scope>, continued                       \
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   length of <select-list>     |        <select-list>          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   \                   <select-list>, continued                    \
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   int result;

   /* Enforce SLPv1 attribute request size limits. */
   if (buffer->end - buffer->curpos < 8)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <Previous Responders Addr Spec>, and convert to UTF-8. */
   attrrqst->prlistlen = GetUINT16(&buffer->curpos);
   attrrqst->prlist = GetStrPtr(&buffer->curpos, 
         attrrqst->prlistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;
   if ((result = SLPv1AsUTF8(encoding, (char *)attrrqst->prlist,
         &attrrqst->prlistlen)) != 0)
      return result;

   /* Parse the URL, and convert to UTF-8. */
   attrrqst->urllen = GetUINT16(&buffer->curpos);
   attrrqst->url = GetStrPtr(&buffer->curpos, attrrqst->urllen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;
   if ((result = SLPv1AsUTF8(encoding, (char *)attrrqst->url,
         &attrrqst->urllen)) != 0)
      return result;

   /* Parse the <Scope>, and convert to UTF-8. */
   attrrqst->scopelistlen = GetUINT16(&buffer->curpos);
   if (attrrqst->scopelistlen)
   {
      attrrqst->scopelist = GetStrPtr(&buffer->curpos, 
            attrrqst->scopelistlen);
      if (buffer->curpos > buffer->end)
         return SLP_ERROR_PARSE_ERROR;
      if ((result = SLPv1AsUTF8(encoding, (char *)attrrqst->scopelist,
            &attrrqst->scopelistlen)) != 0)
         return result;
   }
   else
   {
      attrrqst->scopelist = "DEFAULT";
      attrrqst->scopelistlen = 7;
   }

   /* Parse the <select-list>, and convert to UTF-8. */
   attrrqst->taglistlen = GetUINT16(&buffer->curpos);
   attrrqst->taglist = GetStrPtr(&buffer->curpos, attrrqst->taglistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;
   if ((result = SLPv1AsUTF8(encoding, (char *)attrrqst->taglist,
         &attrrqst->taglistlen)) != 0)
      return result;

   /* SLPv1 attribute requests don't have SPI strings. */
   attrrqst->spistrlen = 0;
   attrrqst->spistr = 0;

   return 0;
}

/** Parse an SLPv1 Attribute Reply message.
 *
 * @param[in] buffer - The buffer containing the data to be parsed.
 * @param[in] encoding - The language encoding of the message.
 * @param[out] attrrply - The attribute reply object into which 
 *                        buffer should be parsed.
 *
 * @return Zero on success, SLP_ERROR_INTERNAL_ERROR (out of memory) or
 *    SLP_ERROR_PARSE_ERROR.
 *
 * @internal
 */
static int v1ParseAttrRply(const SLPBuffer buffer, int encoding,
      SLPAttrRply * attrrply)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Error Code                   | Length of <attr-list> string  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   \                          <attr-list>                          \
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */
 
   int result;

   /* Enforce SLPv1 attribute request size limits. */
   if (buffer->end - buffer->curpos < 4)
      return SLP_ERROR_PARSE_ERROR;

   /* slpv1 doesn't have auths */
   attrrply->authcount = 0;
   attrrply->autharray = 0;

   /* Parse the error code */
   attrrply->errorcode = GetUINT16(&buffer->curpos);

   /* Parse the attributes as UTF8*/
   attrrply->attrlistlen = GetUINT16(&buffer->curpos);
   attrrply->attrlist = GetStrPtr(&buffer->curpos, attrrply->attrlistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;
   if ((result = SLPv1AsUTF8(encoding, (char *)attrrply->attrlist,
         &attrrply->attrlistlen)) != 0)
      return result;

    /* Terminate the attr list for caller convenience - overwrites the
    *  byte after the string, but the buffer should have more than enough allocated.
    */
   if((attrrply->attrlist) && ((buffer->allocated - (buffer->end - buffer->start)) > 0))
      ((uint8_t *)attrrply->attrlist)[attrrply->attrlistlen] = 0;

   return 0;
}

/** Parse an SLPv1 Service Type Request message.
 *
 * @param[in] buffer - The buffer containing the data to be parsed.
 * @param[in] encoding - The language encoding of the message.
 * @param[out] srvtyperqst - The service type request object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, SLP_ERROR_INTERNAL_ERROR (out of memory) or
 *    SLP_ERROR_PARSE_ERROR.
 *
 * @internal
 */
static int v1ParseSrvTypeRqst(const SLPBuffer buffer, int encoding,
      SLPSrvTypeRqst * srvtyperqst)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  length of prev resp string   |<Previous Responders Addr Spec>|
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   \                  <Previous Responders Addr Spec>              \
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   length of naming authority  |   <Naming Authority String>   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   \            <Naming Authority String>, continued               \
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     length of Scope String    |         <Scope String>        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   \                   <Scope String>, continued                   \
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   int result;

   /* Enforce SLPv1 service type request size limits. */
   if (buffer->end - buffer->curpos < 6)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <Previous Responders Addr Spec>, and convert to UTF-8. */
   srvtyperqst->prlistlen = GetUINT16(&buffer->curpos);
   srvtyperqst->prlist = GetStrPtr(&buffer->curpos, srvtyperqst->prlistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;
   if ((result = SLPv1AsUTF8(encoding, (char *)srvtyperqst->prlist,
         &srvtyperqst->prlistlen)) != 0)
      return result;

   /* Parse the <Naming Authority String>, and convert to UTF-8. */
   srvtyperqst->namingauthlen = GetUINT16(&buffer->curpos);
   if (!srvtyperqst->namingauthlen || srvtyperqst->namingauthlen == 0xffff)
      srvtyperqst->namingauth = 0;
   else
   {
      srvtyperqst->namingauth = GetStrPtr(&buffer->curpos, 
            srvtyperqst->namingauthlen);
      if (buffer->curpos > buffer->end)
         return SLP_ERROR_PARSE_ERROR;
      if ((result = SLPv1AsUTF8(encoding, (char *)srvtyperqst->namingauth,
            &srvtyperqst->namingauthlen)) != 0)
         return result;
   }

   /* Parse the <Scope String>, and convert to UTF-8. */
   srvtyperqst->scopelistlen = GetUINT16(&buffer->curpos);
   if (srvtyperqst->scopelistlen)
   {
      srvtyperqst->scopelist = GetStrPtr(&buffer->curpos, 
            srvtyperqst->scopelistlen);
      if (buffer->curpos > buffer->end)
         return SLP_ERROR_PARSE_ERROR;
      if ((result = SLPv1AsUTF8(encoding, (char *)srvtyperqst->scopelist,
            &srvtyperqst->scopelistlen)) != 0)
         return result;
   }
   else
   {
      srvtyperqst->scopelist = "DEFAULT";
      srvtyperqst->scopelistlen = 7;
   }
   return 0;
}

/** Parse an SLPv1 message header.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] header - The SLP message header into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, SLP_ERROR_VER_NOT_SUPPORTED, 
 *    or SLP_ERROR_PARSE_ERROR.
 *
 * @todo Add more flag constraints.
 */
int SLPv1MessageParseHeader(const SLPBuffer buffer, SLPHeader * header)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    Version    |    Function   |            Length             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |O|M|U|A|F| rsvd|    Dialect    |        Language Code          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |        Char Encoding          |              XID              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   /* Check for invalid length - 12 bytes is the smallest SLPv1 message. */
   if (buffer->end - buffer->start < 12)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse header fields. */
   header->version = *buffer->curpos++;
   header->functionid = *buffer->curpos++;
   header->length = GetUINT16(&buffer->curpos);
   header->flags = *buffer->curpos++;
/* header->dialect = */ buffer->curpos++;  /* dialect */
   header->langtaglen = 2;
   header->langtag = GetStrPtr(&buffer->curpos, header->langtaglen);
   header->encoding = GetUINT16(&buffer->curpos); /* character encoding */
   header->extoffset = 0; /* not used for SLPv1 */
   header->xid = GetUINT16(&buffer->curpos);

   /* Enforce function id value range. */
   if (header->functionid < SLP_FUNCT_SRVRQST 
         || header->functionid > SLP_FUNCT_SRVTYPERPLY)
      return SLP_ERROR_PARSE_ERROR;

   /* Enforce language encoding limits. */
   if (header->encoding != SLP_CHAR_ASCII
         && header->encoding != SLP_CHAR_UTF8
         && header->encoding != SLP_CHAR_UNICODE16
         && header->encoding != SLP_CHAR_UNICODE32)
      return SLP_ERROR_CHARSET_NOT_UNDERSTOOD;

   /* Enforce reserved flags constraint. */
   if (header->flags & 0x07)
      return SLP_ERROR_PARSE_ERROR;

   return 0;
}

/** Parse a wire buffer into an SLPv1 message descriptor.
 *
 * Since this is an SLPv2 server with v1 backward-compatibility, we don't
 * need to implement the entire SLPv1 specification. Only those portions
 * that are necessary for v1 backward-compatibility.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] msg - The message into which @p buffer should be parsed.
 *
 * @return Zero on success, SLP_ERROR_PARSE_ERROR, or 
 *    SLP_ERROR_INTERNAL_ERROR if out of memory.
 *
 * @remarks If successful, pointers in the SLPMessage reference memory in
 *    the parsed SLPBuffer. If SLPBufferFree is called then the pointers 
 *    in SLPMessage will be invalidated.
 *
 * @remarks It is assumed that SLPMessageParseBuffer is calling this 
 *    routine and has already reset the message to accomodate new buffer
 *    data.
 */
int SLPv1MessageParseBuffer(const SLPBuffer buffer, SLPMessage * msg)
{
   int result;

   /* parse the header first */
   result = SLPv1MessageParseHeader(buffer, &msg->header);
   if (result == 0)
   {
      /* switch on the function id to parse the body */
      switch (msg->header.functionid)
      {
         case SLP_FUNCT_SRVRQST:
            result = v1ParseSrvRqst(buffer, msg->header.encoding,
                  &msg->body.srvrqst);
            break;

         case SLP_FUNCT_SRVREG:
            result = v1ParseSrvReg(buffer, msg->header.encoding,
                  &msg->body.srvreg);
            break;

         case SLP_FUNCT_SRVDEREG:
            result = v1ParseSrvDeReg(buffer, msg->header.encoding,
                  &msg->body.srvdereg);
            break;

         case SLP_FUNCT_ATTRRQST:
            result = v1ParseAttrRqst(buffer, msg->header.encoding,
                  &msg->body.attrrqst);
            break;

         case SLP_FUNCT_ATTRRPLY:
            result = v1ParseAttrRply(buffer, msg->header.encoding,
                  &msg->body.attrrply);
            break;

         case SLP_FUNCT_DAADVERT:
            /* We are a SLPv2 DA, drop advertisements from other v1
             * DAs (including our own). The message will be ignored
             * by SLPDv1ProcessMessage(). 
             */
            result = 0;
            break;

         case SLP_FUNCT_SRVTYPERQST:
            result = v1ParseSrvTypeRqst(buffer, msg->header.encoding,
                  &msg->body.srvtyperqst);
            break;

         default:
            result = SLP_ERROR_MESSAGE_NOT_SUPPORTED;
      }
   }
   return result;
}

/*=========================================================================*/

