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

/** Functions specific to the SLP wire protocol messages.
 *
 * @file       slp_v2message.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeMessageV2
 */

#include "slp_message.h"
#include "slp_xmalloc.h"

/** Parse an authentication block.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] authblock - The authentication block object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, SLP_ERROR_INTERNAL_ERROR (out of memory)
 *    or SLP_ERROR_PARSE_ERROR.
 *
 * @internal
 */
static int v2ParseAuthBlock(SLPBuffer buffer, SLPAuthBlock * authblock)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Block Structure Descriptor   |  Authentication Block Length  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           Timestamp                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     SLP SPI String Length     |         SLP SPI String        \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |              Structured Authentication Block ...              \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   /* Enforce v2 authentication block size limits. */
   if (buffer->end - buffer->curpos < 10)
      return SLP_ERROR_PARSE_ERROR;

   /* Save pointer to opaque authentication block. */
   authblock->opaque = buffer->curpos;

   /* Parse individual authentication block fields. */
   authblock->bsd = GetUINT16(&buffer->curpos);
   authblock->length = GetUINT16(&buffer->curpos);
   authblock->timestamp = GetUINT32(&buffer->curpos);
   authblock->spistrlen = GetUINT16(&buffer->curpos);
   authblock->spistr = GetStrPtr(&buffer->curpos, authblock->spistrlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse structured authentication block. */
   authblock->authstruct = (char *)buffer->curpos;
   authblock->opaquelen = authblock->length;
   buffer->curpos = authblock->opaque + authblock->length;
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   return 0;
}

/** Parse a URL entry.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] urlentry - The URL entry object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, SLP_ERROR_INTERNAL_ERROR (out of memory)
 *    or SLP_ERROR_PARSE_ERROR.
 *
 * @internal
 */
static int v2ParseUrlEntry(SLPBuffer buffer, SLPUrlEntry * urlentry)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   Reserved    |          Lifetime             |   URL Length  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |URL len, contd.|            URL (variable length)              \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |# of URL auths |            Auth. blocks (if any)              \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   /* Enforce SLPv2 URL entry size limits. */
   if (buffer->end - buffer->curpos < 6)
      return SLP_ERROR_PARSE_ERROR;

   /* Save pointer to opaque URL entry block. */
   urlentry->opaque = buffer->curpos;

   /* Parse individual URL entry fields. */
   urlentry->reserved = *buffer->curpos++;
   urlentry->lifetime = GetUINT16(&buffer->curpos);
   urlentry->urllen = GetUINT16(&buffer->curpos);
   urlentry->url = GetStrPtr(&buffer->curpos, urlentry->urllen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse authentication block. */
   urlentry->authcount = *buffer->curpos++;
   if (urlentry->authcount)
   {
      int i;
      urlentry->autharray = xmalloc(urlentry->authcount 
            * sizeof(SLPAuthBlock));
      if (urlentry->autharray == 0)
         return SLP_ERROR_INTERNAL_ERROR;
      memset(urlentry->autharray, 0, urlentry->authcount 
            * sizeof(SLPAuthBlock));
      for (i = 0; i < urlentry->authcount; i++)
      {
         int result = v2ParseAuthBlock(buffer, &urlentry->autharray[i]);
         if (result != 0)
            return result;
      }
   }
   urlentry->opaquelen = buffer->curpos - urlentry->opaque;

   /* Terminate the URL string for caller convenience - we're overwriting 
    * the first byte of the "# of URL auths" field, but it's okay because
    * we've already read and stored it away.
    */
   if(urlentry->url)
      ((uint8_t *)urlentry->url)[urlentry->urllen] = 0;

   return 0;
}

/** Parse a service request.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] srvrqst - The service request object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, or a non-zero error code.
 *
 * @internal
 */
static int v2ParseSrvRqst(SLPBuffer buffer, SLPSrvRqst * srvrqst)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      length of <PRList>       |        <PRList> String        \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   length of <service-type>    |    <service-type> String      \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    length of <scope-list>     |     <scope-list> String       \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  length of predicate string   |  Service Request <predicate>  \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  length of <SLP SPI> string   |       <SLP SPI> String        \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   /* Enforce v2 service request size limits. */
   if (buffer->end - buffer->curpos < 10)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <PRList> string. */
   srvrqst->prlistlen = GetUINT16(&buffer->curpos);
   srvrqst->prlist = GetStrPtr(&buffer->curpos, srvrqst->prlistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <service-type> string. */
   srvrqst->srvtypelen = GetUINT16(&buffer->curpos);
   srvrqst->srvtype = GetStrPtr(&buffer->curpos, srvrqst->srvtypelen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <scope-list> string. */
   srvrqst->scopelistlen = GetUINT16(&buffer->curpos);
   srvrqst->scopelist = GetStrPtr(&buffer->curpos, srvrqst->scopelistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <predicate> string. */
   srvrqst->predicatever = 2;  /* SLPv2 predicate (LDAPv3) */
   srvrqst->predicatelen = GetUINT16(&buffer->curpos);
   srvrqst->predicate = GetStrPtr(&buffer->curpos, srvrqst->predicatelen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <SLP SPI> string. */
   srvrqst->spistrlen = GetUINT16(&buffer->curpos);
   srvrqst->spistr = GetStrPtr(&buffer->curpos, srvrqst->spistrlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   return 0;
}

/** Parse a service reply.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] srvrply - The service reply object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, or a non-zero error code.
 *
 * @internal
 */
static int v2ParseSrvRply(SLPBuffer buffer, SLPSrvRply * srvrply)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |        Error Code             |        URL Entry count        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |       <URL Entry 1>          ...       <URL Entry N>          \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   /* Enforce v2 service reply size limits. */
   if (buffer->end - buffer->curpos < 4)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse out the error code. */
   srvrply->errorcode = GetUINT16(&buffer->curpos);
   if (srvrply->errorcode)
   {  /* don't trust the rest of the packet */
      int err = srvrply->errorcode;
      memset(srvrply, 0, sizeof(SLPSrvRply));
      srvrply->errorcode = err;
      return 0;
   }

   /* Parse the URL entry. */
   srvrply->urlcount = GetUINT16(&buffer->curpos);
   if (srvrply->urlcount)
   {
      int i;
      srvrply->urlarray = xmalloc(sizeof(SLPUrlEntry) * srvrply->urlcount);
      if (srvrply->urlarray == 0)
         return SLP_ERROR_INTERNAL_ERROR;
      memset(srvrply->urlarray, 0, sizeof(SLPUrlEntry) * srvrply->urlcount);
      for (i = 0; i < srvrply->urlcount; i++)
      {
         int result;
         result = v2ParseUrlEntry(buffer, &srvrply->urlarray[i]);
         if (result != 0)
            return result;
      }
   }
   return 0;
}

/** Parse a service registration.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] srvreg - The service registration object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, or a non-zero error code.
 *
 * @internal
 */
static int v2ParseSrvReg(SLPBuffer buffer, SLPSrvReg * srvreg)
{
   int result;

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
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   /* Parse the <URL-Entry>. */
   result = v2ParseUrlEntry(buffer, &srvreg->urlentry);
   if (result != 0)
      return result;

   /* Parse the <service-type> string. */
   srvreg->srvtypelen = GetUINT16(&buffer->curpos);
   srvreg->srvtype = GetStrPtr(&buffer->curpos, srvreg->srvtypelen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <scope-list> string. */
   srvreg->scopelistlen = GetUINT16(&buffer->curpos);
   srvreg->scopelist = GetStrPtr(&buffer->curpos, srvreg->scopelistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <attr-list> string. */
   srvreg->attrlistlen = GetUINT16(&buffer->curpos);
   srvreg->attrlist = GetStrPtr(&buffer->curpos, srvreg->attrlistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse AttrAuth block list (if present). */
   srvreg->authcount = *buffer->curpos++;
   if (srvreg->authcount)
   {
      int i;
      srvreg->autharray = xmalloc(srvreg->authcount * sizeof(SLPAuthBlock));
      if (srvreg->autharray == 0)
         return SLP_ERROR_INTERNAL_ERROR;
      memset(srvreg->autharray, 0,  srvreg->authcount * sizeof(SLPAuthBlock));
      for (i = 0; i < srvreg->authcount; i++)
      {
         result = v2ParseAuthBlock(buffer, &srvreg->autharray[i]);
         if (result != 0)
            return result;
      }
   }
   return 0;
}

/** Parse a service deregistration.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] srvdereg - The service deregistration object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, or a non-zero error code.
 *
 * @internal
 */
static int v2ParseSrvDeReg(SLPBuffer buffer, SLPSrvDeReg * srvdereg)
{
   int result;

/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    Length of <scope-list>     |         <scope-list>          \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           URL Entry                           \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      Length of <tag-list>     |            <tag-list>         \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   /* Enforce SLPv2 service deregister size limits. */
   if (buffer->end - buffer->curpos < 4)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <scope-list>. */
   srvdereg->scopelistlen = GetUINT16(&buffer->curpos);
   srvdereg->scopelist = GetStrPtr(&buffer->curpos, srvdereg->scopelistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the URL entry. */
   result = v2ParseUrlEntry(buffer, &srvdereg->urlentry);
   if (result)
      return result;

   /* Parse the <tag-list>. */
   srvdereg->taglistlen = GetUINT16(&buffer->curpos);
   srvdereg->taglist = GetStrPtr(&buffer->curpos, srvdereg->taglistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   return 0;
}

/** Parse a server ACK.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] srvack - The server ACK object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero (success) always.
 *
 * @internal
 */
static int v2ParseSrvAck(SLPBuffer buffer, SLPSrvAck * srvack)
{
/*  0                   1
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          Error Code           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   /* Parse the Error Code. */
   srvack->errorcode = GetUINT16(&buffer->curpos);

   return 0;
}

/** Parse an attribute request.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] attrrqst - The attribute request object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, or a non-zero error code.
 *
 * @internal
 */
static int v2ParseAttrRqst(SLPBuffer buffer, SLPAttrRqst * attrrqst)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |       length of PRList        |        <PRList> String        \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         length of URL         |              URL              \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    length of <scope-list>     |      <scope-list> string      \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  length of <tag-list> string  |       <tag-list> string       \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   length of <SLP SPI> string  |        <SLP SPI> string       \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   /* Enforce v2 attribute request size limits. */
   if (buffer->end - buffer->curpos < 10)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <PRList> string. */
   attrrqst->prlistlen = GetUINT16(&buffer->curpos);
   attrrqst->prlist = GetStrPtr(&buffer->curpos, attrrqst->prlistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the URL. */
   attrrqst->urllen = GetUINT16(&buffer->curpos);
   attrrqst->url = GetStrPtr(&buffer->curpos, attrrqst->urllen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <scope-list> string. */
   attrrqst->scopelistlen = GetUINT16(&buffer->curpos);
   attrrqst->scopelist = GetStrPtr(&buffer->curpos, attrrqst->scopelistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <tag-list> string. */
   attrrqst->taglistlen = GetUINT16(&buffer->curpos);
   attrrqst->taglist = GetStrPtr(&buffer->curpos, attrrqst->taglistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <SLP SPI> string. */
   attrrqst->spistrlen = GetUINT16(&buffer->curpos);
   attrrqst->spistr = GetStrPtr(&buffer->curpos, attrrqst->spistrlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   return 0;
}

/** Parse an attribute reply.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] attrrply - The attribute reply object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, or a non-zero error code.
 *
 * @internal
 */
static int v2ParseAttrRply(SLPBuffer buffer, SLPAttrRply * attrrply)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         Error Code            |      length of <attr-list>    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         <attr-list>                           \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |# of AttrAuths |  Attribute Authentication Block (if present)  \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   /* Enforce SLPv2 attribute request size limits. */
   if (buffer->end - buffer->curpos < 5)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the Error Code. */
   attrrply->errorcode = GetUINT16(&buffer->curpos);
   if (attrrply->errorcode)
   {
      /* Don't trust the rest of the packet. */
      int err = attrrply->errorcode;
      memset(attrrply, 0, sizeof(SLPAttrRply));
      attrrply->errorcode = err;
      return 0;
   }

   /* Parse the <attr-list>. */
   attrrply->attrlistlen = GetUINT16(&buffer->curpos);
   attrrply->attrlist = GetStrPtr(&buffer->curpos, attrrply->attrlistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the Attribute Authentication Block list (if present). */
   attrrply->authcount = *buffer->curpos++;
   if (attrrply->authcount)
   {
      int i;
      attrrply->autharray = xmalloc(attrrply->authcount * sizeof(SLPAuthBlock));
      if (attrrply->autharray == 0)
         return SLP_ERROR_INTERNAL_ERROR;
      memset(attrrply->autharray, 0, attrrply->authcount * sizeof(SLPAuthBlock));
      for (i = 0; i < attrrply->authcount; i++)
      {
         int result = v2ParseAuthBlock(buffer, &attrrply->autharray[i]);
         if (result != 0)
            return result;
      }
   }

   /* Terminate the attr list for caller convenience - overwrites the
    * first byte of the "# of AttrAuths" field, but we've processed it. 
    */
   if(attrrply->attrlist)
      ((uint8_t *)attrrply->attrlist)[attrrply->attrlistlen] = 0;

   return 0;
}

/** Parse a DA advertisement.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] daadvert - The DA advertisement object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, or a non-zero error code.
 *
 * @internal
 */
static int v2ParseDAAdvert(SLPBuffer buffer, SLPDAAdvert * daadvert)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          Error Code           |  DA Stateless Boot Timestamp  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |DA Stateless Boot Time,, contd.|         Length of URL         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   \                              URL                              \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Length of <scope-list>    |         <scope-list>          \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Length of <attr-list>     |          <attr-list>          \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    Length of <SLP SPI List>   |     <SLP SPI List> String     \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | # Auth Blocks |         Authentication block (if any)         \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   /* Enforce SLPv2 DA advertisement size limits. */
   if (buffer->end - buffer->curpos < 15)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the Error Code. */
   daadvert->errorcode = GetUINT16(&buffer->curpos);
   if (daadvert->errorcode)
   {
      /* Don't trust the rest of the packet. */
      int err = daadvert->errorcode;
      memset(daadvert, 0, sizeof(SLPDAAdvert));
      daadvert->errorcode = err;
      return 0;
   }

   /* Parse the DA Stateless Boot Timestamp. */
   daadvert->bootstamp = GetUINT32(&buffer->curpos);

   /* Parse out the URL. */
   daadvert->urllen = GetUINT16(&buffer->curpos);
   daadvert->url = GetStrPtr(&buffer->curpos, daadvert->urllen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <scope-list>. */
   daadvert->scopelistlen = GetUINT16(&buffer->curpos);
   daadvert->scopelist = GetStrPtr(&buffer->curpos, daadvert->scopelistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <attr-list>. */
   daadvert->attrlistlen = GetUINT16(&buffer->curpos);
   daadvert->attrlist = GetStrPtr(&buffer->curpos, daadvert->attrlistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <SLP SPI List> String. */
   daadvert->spilistlen = GetUINT16(&buffer->curpos);
   daadvert->spilist = GetStrPtr(&buffer->curpos, daadvert->spilistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the authentication block list (if any). */
   daadvert->authcount = *buffer->curpos++;
   if (daadvert->authcount)
   {
      int i;
      daadvert->autharray = xmalloc(sizeof(SLPAuthBlock) 
            * daadvert->authcount);
      if (daadvert->autharray == 0)
         return SLP_ERROR_INTERNAL_ERROR;
      memset(daadvert->autharray, 0, sizeof(SLPAuthBlock) 
            * daadvert->authcount);
      for (i = 0; i < daadvert->authcount; i++)
      {
         int result = v2ParseAuthBlock(buffer, &daadvert->autharray[i]);
         if (result != 0)
            return result;
      }
   }

   /* Terminate the URL string for caller convenience - we're overwriting 
    * the first byte of the "Length of <scope-list>" field, but it's okay 
    * because we've already read and stored it away.
    */
   if(daadvert->url)
      ((uint8_t *)daadvert->url)[daadvert->urllen] = 0;

   return 0;
}

/** Parse a service type request.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] srvtyperqst - The service type request object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, or a non-zero error code.
 *
 * @internal
 */
static int v2ParseSrvTypeRqst(SLPBuffer buffer, 
      SLPSrvTypeRqst * srvtyperqst)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |        length of PRList       |        <PRList> String        \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   length of Naming Authority  |   <Naming Authority String>   \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     length of <scope-list>    |      <scope-list> String      \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   /* Enforce SLPv2 service type request size limits. */
   if (buffer->end - buffer->curpos < 6)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the PRList. */
   srvtyperqst->prlistlen = GetUINT16(&buffer->curpos);
   srvtyperqst->prlist = GetStrPtr(&buffer->curpos, srvtyperqst->prlistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the Naming Authority. */
   srvtyperqst->namingauthlen = GetUINT16(&buffer->curpos);
   if (srvtyperqst->namingauthlen == 0
         || srvtyperqst->namingauthlen == 0xffff)
      srvtyperqst->namingauth = 0;
   else
      srvtyperqst->namingauth = GetStrPtr(&buffer->curpos, 
            srvtyperqst->namingauthlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <scope-list>. */
   srvtyperqst->scopelistlen = GetUINT16(&buffer->curpos);
   srvtyperqst->scopelist = GetStrPtr(&buffer->curpos, srvtyperqst->scopelistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   return 0;
}

/** Parse a service type reply.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] srvtyperply - The service type reply object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, or a non-zero error code.
 *
 * @internal
 */
static int v2ParseSrvTypeRply(SLPBuffer buffer, 
      SLPSrvTypeRply * srvtyperply)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Error Code          |    length of <srvType-list>   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       <srvtype--list>                         \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   /* Enforce SLPv2 service type reply size limits. */
   if (buffer->end - buffer->curpos < 4)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the Error Code. */
   srvtyperply->errorcode = GetUINT16(&buffer->curpos);
   if (srvtyperply->errorcode)
   {
      /* Don't trust the rest of the packet. */
      int err = srvtyperply->errorcode;
      memset(srvtyperply, 0, sizeof(SLPSrvTypeRply));
      srvtyperply->errorcode = err;
      return 0;
   }

   /* Parse the <srvType-list>. */
   srvtyperply->srvtypelistlen = GetUINT16(&buffer->curpos);
   srvtyperply->srvtypelist = GetStrPtr(&buffer->curpos, 
         srvtyperply->srvtypelistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Terminate the service type list string for caller convenience - while 
    * it appears that we're writing one byte past the end of the buffer here, 
    * it's not so - message buffers are always allocated one byte larger than 
    * requested for just this reason.
    */
   if(srvtyperply->srvtypelist)
      ((uint8_t *)srvtyperply->srvtypelist)[srvtyperply->srvtypelistlen] = 0;

   return 0;
}

/** Parse an SA advertisement.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] saadvert - The SA advertisement object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, or a non-zero error code.
 *
 * @internal
 */
static int v2ParseSAAdvert(SLPBuffer buffer, SLPSAAdvert * saadvert)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         Length of URL         |              URL              \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Length of <scope-list>    |         <scope-list>          \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Length of <attr-list>     |          <attr-list>          \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | # auth blocks |        authentication block (if any)          \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   /* Enforce SLPv2 SA advertisement size limits. */
   if (buffer->end - buffer->curpos < 7)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse out the URL. */
   saadvert->urllen = GetUINT16(&buffer->curpos);
   saadvert->url = GetStrPtr(&buffer->curpos, saadvert->urllen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <scope-list>. */
   saadvert->scopelistlen = GetUINT16(&buffer->curpos);
   saadvert->scopelist = GetStrPtr(&buffer->curpos, saadvert->scopelistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the <attr-list>. */
   saadvert->attrlistlen = GetUINT16(&buffer->curpos);
   saadvert->attrlist = GetStrPtr(&buffer->curpos, saadvert->attrlistlen);
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse the authentication block list (if any). */
   saadvert->authcount = *buffer->curpos++;
   if (saadvert->authcount)
   {
      int i;
      saadvert->autharray = xmalloc(saadvert->authcount
            * sizeof(SLPAuthBlock));
      if (saadvert->autharray == 0)
         return SLP_ERROR_INTERNAL_ERROR;
      memset(saadvert->autharray, 0, saadvert->authcount
            * sizeof(SLPAuthBlock));
      for (i = 0; i < saadvert->authcount; i++)
      {
         int result = v2ParseAuthBlock(buffer, &saadvert->autharray[i]);
         if (result != 0)
            return result;
      }
   }

   /* Terminate the URL string for caller convenience - we're overwriting 
    * the first byte of the "Length of <scope-list>" field, but it's okay 
    * because we've already read and stored it away.
    */
   if(saadvert->url)
      ((uint8_t *)saadvert->url)[saadvert->urllen] = 0;

   return 0;
}

/* -------------------------------------------------------------------
   The following code is for managing a list of extension references
   maintained by the extension parser. This allows us to verify that
   there are no extension reference loops in the message. This code 
   assumes we we have bigger fish to fry if we run out of memory so
   it simply doesn't add a reference to the list if it can't allocate.
   ------------------------------------------------------------------- */

struct extref {
   struct extref * next;
   int ref;
};

/** Add an extension reference to a list of references.
 * 
 * The list should previously have been checked for duplicates before
 * this routine is called with a reference.
 *
 * @param list - The address of the list to which an extension
 *    reference should be added.
 * @param ref - The reference to be added to the list.
 */
static void addExtRefToList(struct extref ** list, int ref) {
   struct extref * er = (struct extref *)malloc(sizeof(struct extref));
   if (er) {
      er->ref = ref;
      er->next = *list;
      *list = er;
   }
}

/** Check if a given reference has already been processed.
 *
 * @param list - The list to be checked for duplicate entry.
 * @param ref - The reference to check for.
 *
 * @return A boolean value - True (1) if the specified reference is
 *    already in the list, False (0) if not found.
 */
static int isExtRefInList(struct extref * list, int ref) {
   struct extref * cur = list;
   while (cur) {
      if (cur->ref == ref)
         return 1;
      cur = cur->next;
   }
   return 0;
}

/** Destroy an existing reference list.
 *
 * @param list - The list to be destroyed.
 */
static void cleanupExtRefList(struct extref * list) {
   struct extref * cur = list; 
   while (cur) {
      struct extref * save = cur;
      cur = cur->next;
      free(save);
   }
}

/** Parse a service extension.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] msg - The service extension object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, or a non-zero error code.
 *
 * @note Parse extensions @b after all standard protocol fields are parsed.
 *
 * @internal
 */
static int v2ParseExtension(SLPBuffer buffer, SLPMessage * msg)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         Extension ID          |       Next Extension Offset   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Offset, contd.|                Extension Data                 \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   int result = 0;
   struct extref * erlist = 0;
   int nextoffset = msg->header.extoffset;

   while (nextoffset)
   {
      int extid;

      /* check for circular reference in list */
      if (isExtRefInList(erlist, nextoffset)) {
         result = SLP_ERROR_PARSE_ERROR;
         goto errout;
      }
      addExtRefToList(&erlist, nextoffset);

      buffer->curpos = buffer->start + nextoffset;
      
      if (buffer->curpos + 5 > buffer->end) {
         result = SLP_ERROR_PARSE_ERROR;
         goto errout;
      }

      extid = GetUINT16(&buffer->curpos);
      nextoffset = GetUINT24(&buffer->curpos);
      switch (extid)
      {
         /* Support the standard and experimental versions of this extension 
          * in order to support 1.2.x for a time while the experimental
          * version is deprecated.
          */
         case SLP_EXTENSION_ID_REG_PID:
         case SLP_EXTENSION_ID_REG_PID_EXP:
            if (msg->header.functionid == SLP_FUNCT_SRVREG)
            {
               if (buffer->curpos + 4 > buffer->end) {
                  result = SLP_ERROR_PARSE_ERROR;
                  goto errout;
               }
               msg->body.srvreg.pid = GetUINT32(&buffer->curpos);
            }
            break;

         default:
            /* These are required extensions. Error if not handled. */
            if (extid >= 0x4000 && extid <= 0x7FFF) {
               result = SLP_ERROR_OPTION_NOT_UNDERSTOOD;
               goto errout;
            }
            break;
      }
   }

errout:
   cleanupExtRefList(erlist);
   return result;
}

/** Parse an SLPv2 message header.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] header - The address of a message header into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, or a non-zero error code, either
 *    SLP_ERROR_VER_NOT_SUPPORTED or SLP_ERROR_PARSE_ERROR.
 */
int SLPv2MessageParseHeader(SLPBuffer buffer, SLPHeader * header)
{
/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    Version    |  Function-ID  |            Length             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Length, contd.|O|F|R|       reserved          |Next Ext Offset|
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Next Extension Offset, contd.|              XID              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      Language Tag Length      |         Language Tag          \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   /* Check for invalid length - 18 bytes is the smallest v2 message. */
   if (buffer->end - buffer->start < 18)
      return SLP_ERROR_PARSE_ERROR;

   /* Parse header fields. */
   header->version = *buffer->curpos++;
   header->functionid = *buffer->curpos++;
   header->length = GetUINT24(&buffer->curpos);
   header->flags = GetUINT16(&buffer->curpos);
   header->encoding = 0; /* not used for SLPv2 */
   header->extoffset = GetUINT24(&buffer->curpos);
   header->xid = GetUINT16(&buffer->curpos);
   header->langtaglen = GetUINT16(&buffer->curpos);
   header->langtag = GetStrPtr(&buffer->curpos, header->langtaglen);

   /* Enforce language tag size limits. */
   if (buffer->curpos > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   /* Enforce function id range. */
   if (header->functionid < SLP_FUNCT_SRVRQST 
         || header->functionid > SLP_FUNCT_SAADVERT)
      return SLP_ERROR_PARSE_ERROR;

   /* Enforce reserved flags constraint. */
   if (header->flags & 0x1fff)
      return SLP_ERROR_PARSE_ERROR;

   /* Enforce extension offset limits. */
   if (buffer->start + header->extoffset > buffer->end)
      return SLP_ERROR_PARSE_ERROR;

   return 0;
}

/** Parse a wire buffer into an SLPv2 message descriptor.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 * @param[out] msg - The message object into which 
 *    @p buffer should be parsed.
 *
 * @return Zero on success, SLP_ERROR_PARSE_ERROR, or
 *    SLP_ERROR_INTERNAL_ERROR if out of memory. If SLPMessage
 *    is invalid then return is not successful.
 *
 * @remarks On success, pointers in the SLPMessage reference memory in
 *    the parsed SLPBuffer. If SLPBufferFree is called then the pointers
 *    in @p message will be invalidated.
 *
 * @remarks It is assumed that SLPMessageParseBuffer is calling this 
 *    routine and has already reset the message to accomodate new buffer
 *    data.
 */
int SLPv2MessageParseBuffer(SLPBuffer buffer, SLPMessage * msg)
{
   int result;

   /* parse the header first */
   result = SLPv2MessageParseHeader(buffer, &msg->header);
   if (result == 0)
   {
      /* switch on the function id to parse the body */
      switch (msg->header.functionid)
      {
         case SLP_FUNCT_SRVRQST:
            result = v2ParseSrvRqst(buffer, &msg->body.srvrqst);
            break;

         case SLP_FUNCT_SRVRPLY:
            result = v2ParseSrvRply(buffer, &msg->body.srvrply);
            break;

         case SLP_FUNCT_SRVREG:
            result = v2ParseSrvReg(buffer, &msg->body.srvreg);
            break;

         case SLP_FUNCT_SRVDEREG:
            result = v2ParseSrvDeReg(buffer, &msg->body.srvdereg);
            break;

         case SLP_FUNCT_SRVACK:
            result = v2ParseSrvAck(buffer, &msg->body.srvack);
            break;

         case SLP_FUNCT_ATTRRQST:
            result = v2ParseAttrRqst(buffer, &msg->body.attrrqst);
            break;

         case SLP_FUNCT_ATTRRPLY:
            result = v2ParseAttrRply(buffer, &msg->body.attrrply);
            break;

         case SLP_FUNCT_DAADVERT:
            result = v2ParseDAAdvert(buffer, &msg->body.daadvert);
            break;

         case SLP_FUNCT_SRVTYPERQST:
            result = v2ParseSrvTypeRqst(buffer, &msg->body.srvtyperqst);
            break;

         case SLP_FUNCT_SRVTYPERPLY:
            result = v2ParseSrvTypeRply(buffer, &msg->body.srvtyperply);
            break;

         case SLP_FUNCT_SAADVERT:
            result = v2ParseSAAdvert(buffer, &msg->body.saadvert);
            break;

         default:
            result = SLP_ERROR_MESSAGE_NOT_SUPPORTED;
      }
   }

   if (result == 0 && msg->header.extoffset)
      result = v2ParseExtension(buffer, msg);

   return result;
}

/*=========================================================================*/
