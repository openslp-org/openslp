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

/** Functions that convert between UTF-8 and other character encodings.
 *
 * @file       slp_utf8.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#include <sys/types.h>

#include "slp_message.h"
#include "slp_v1message.h"

typedef struct
{
   int cmask;
   int cval;
   int shift;
   long lmask;
   long lval;
} Tab;

static Tab tab[] =
{
   { 0x80, 0x00, 0 * 6,       0x7F,         0 }, /* 1 byte sequence */
   { 0xE0, 0xC0, 1 * 6,      0x7FF,      0x80 }, /* 2 byte sequence */
   { 0xF0, 0xE0, 2 * 6,     0xFFFF,     0x800 }, /* 3 byte sequence */
   { 0xF8, 0xF0, 3 * 6,   0x1FFFFF,   0x10000 }, /* 4 byte sequence */
   { 0xFC, 0xF8, 4 * 6,  0x3FFFFFF,  0x200000 }, /* 5 byte sequence */
   { 0xFE, 0xFC, 5 * 6, 0x7FFFFFFF, 0x4000000 }, /* 6 byte sequence */
   { 0,       0,     0,          0,         0 }  /* end of table    */
};

/** Convert a multi-byte UTF-8 character to a Unicode character.
 *
 * @param[out] p - The address of storage for a unicode character.
 * @param[in] s - A pointer to a multi-byte UTF-8 character.
 * @param[in] n - The number of bytes in @p s.
 *
 * @return The number of bytes converted from s, or -1 on error.
 *
 * @remarks Adapted from Ken Thompson's fss-utf.c. See 
 *    ftp://ftp.informatik.uni-erlangen.de/pub/doc/ISO/charsets/utf-8.c
 *
 * @internal
 */
static int utftouni(unsigned *p, const char *s, size_t n)
{
   long l;
   int c0, c;
   size_t nc;
   Tab *t;

   if (s == 0)
      return 0;

   nc = 0;
   if (n <= nc)
      return -1;
   c0 = *s & 0xff;
   l = c0;
   for (t = tab; t->cmask; t++)
   {
      nc++;
      if ((c0 & t->cmask) == t->cval)
      {
         l &= t->lmask;
         if (l < t->lval)
            return -1;
         *p = l;
         return (int)nc;
      }
      if (n <= nc)
         return -1;
      s++;
      c = (*s ^ 0x80) & 0xFF;
      if (c & 0xC0)
         return -1;
      l = (l << 6) | c;
   }
   return -1;
}

/** Convert a Unicode character to a multi-byte UTF-8 character.
 *
 * @param[out] s - The address of storage for multiple UTF-8 bytes.
 * @param[in] wc - The Unicode character to convert.
 *
 * @return The number of bytes stored in @p s, or -1 on error.
 *
 * @remarks Adapted from Ken Thompson's fss-utf.c. See 
 *    ftp://ftp.informatik.uni-erlangen.de/pub/doc/ISO/charsets/utf-8.c
 *
 * @note The space pointed to by @p s should be at least 6 bytes, which 
 *    is the maximum possible UTF-8 encoded character size.
 *
 * @internal
 */
static int unitoutf(char *s, unsigned wc)
{
   long l;
   int c, nc;
   Tab *t;

   if (s == 0)
      return 0;

   l = wc;
   nc = 0;
   for (t=tab; t->cmask; t++)
   {
      nc++;
      if (l <= t->lmask)
      {
         c = t->shift;
         *s = (char)(t->cval | ( l >> c ));
         while (c > 0)
         {
            c -= 6;
            s++;
            *s = (char)(0x80 | (( l >> c) & 0x3F));
         }
         return nc;
      }
   }
   return -1;
}

/** Convert an SLPv1 encoded string to a UTF-8 string.
 *
 * Converts a SLPv1 encoded string to a UTF-8 character string in-place. 
 * If string does not have enough space to hold the encoded string we are 
 * dead.
 *
 * @param[in] encoding - The Unicode encoding of the string passed in.
 * @param[in,out] string - On entry, a pointer to SLPv1 encoded string; 
 *    on exit, a pointer to the converted UTF-8 string.
 * @param[in,out] len - On entry, the length of the SLPv1-encoded string
 *    in bytes; on exit, the length of UTF-8 string in bytes.
 *
 * @return Zero on success, SLP_ERROR_PARSE_ERROR, or 
 *    SLP_ERROR_INTERNAL_ERROR if out of memory. The @p string and @p len
 *    parameters are invalid if return is not successful.
 *
 * @note Even though this routine allows for conversion of 2 or 4-byte
 *    unicode characters to the ENTIRE utf-8 character encoding, only utf-8
 *    characters that will fit into the space occupied by the original 2- or
 *    4-byte unicode character are really allowed, as characters larger than 
 *    the original will overwrite the first one or more bytes of the next 
 *    character. However MOST utf-8 characters in common use today are two
 *    bytes or less in length. 
 */
int SLPv1AsUTF8(int encoding, char *string, int *len)
{
   int nc;
   unsigned uni;
   char utfchar[6];        /* UTF-8 chars are at most 6 bytes */
   char *utfstring = string, *unistring = string;

   if (encoding == SLP_CHAR_ASCII || encoding == SLP_CHAR_UTF8)
      return 0;

   if (encoding != SLP_CHAR_UNICODE16 && encoding != SLP_CHAR_UNICODE32)
      return SLP_ERROR_INTERNAL_ERROR;

   while (*len)
   {
      if (encoding == SLP_CHAR_UNICODE16)
      {
         uni = AsUINT16(unistring);
         unistring += 2;
         *len -= 2;
      }
      else
      {
         uni = AsUINT32(unistring);
         unistring += 4;
         *len -= 4;
      }
      if (*len < 0)
         return SLP_ERROR_INTERNAL_ERROR;

      nc = unitoutf(utfchar, uni);

      /* Take care not to overwrite. */
      if (nc < 0 || utfstring + nc > unistring)
         return SLP_ERROR_INTERNAL_ERROR;

      memcpy(utfstring, utfchar, nc);
      utfstring += nc;
   }
   *len = utfstring - string;
   return 0;
}

/** Converts a UTF-8 character string to an SLPv1-encoded string.
 *
 * string - (OUT) SLPv1 encoded string.
 * len    - (INOUT) IN - bytes available in string
 *                  OUT - bytes used up in string
 * encoding  - (IN) encoding of the string passed in
 * utfstring - (IN) pointer to UTF-8 string
 * utflen    - (IN) length of UTF-8 string
 *
 * @return Zero on success, SLP_ERROR_PARSE_ERROR, or
 *    SLP_ERROR_INTERNAL_ERROR if out of memory. The @p string and 
 *    @p len parameters are invalid if return is not successful.
 *
 * @remarks When called with @p string set to null, this routine returns
 *    the number of bytes needed in string.
 */
int SLPv1ToEncoding(char *string, int *len, int encoding, 
      const char *utfstring, int utflen)
{
   unsigned uni;
   int nc, total = 0;

   if (encoding == SLP_CHAR_ASCII || encoding == SLP_CHAR_UTF8)
   {
      if (*len < utflen)
         return SLP_ERROR_INTERNAL_ERROR;
      *len = utflen;
      if (string)
         memcpy(string, utfstring, utflen);
      return 0;
   }
   if (encoding != SLP_CHAR_UNICODE16 && encoding != SLP_CHAR_UNICODE32)
      return SLP_ERROR_INTERNAL_ERROR;
   while (utflen)
   {
      nc = utftouni(&uni, utfstring, utflen);
      utflen -= nc;
      if (nc < 0 || utflen < 0)
         return SLP_ERROR_INTERNAL_ERROR;
      utfstring += nc;
      if (encoding == SLP_CHAR_UNICODE16)
      {
         if (string)
         {
            ToUINT16(string, uni);
            string += 2;
         }
         total += 2;
      }
      else
      {
         if (string)
         {
            ToUINT32(string, uni);
            string += 4;
         }
         total += 4;
      }
      if (total > *len)
         return SLP_ERROR_INTERNAL_ERROR;
   }
   *len = total;
   return 0;
}

/*=========================================================================*/
