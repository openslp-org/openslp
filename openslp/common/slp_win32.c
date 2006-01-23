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
 *
 * Copyright (C) 1996-2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * INTERNET SOFTWARE CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *-------------------------------------------------------------------------*/

/** Missing Win32 functions.
 *
 * The functions in this file include routines to convert network addresses
 * from binary to presentation and from presentation to binary format. The
 * purpose of this file however is to provide a place to implement functions
 * that are common on Unix platforms, but which are missing on Win32 
 * platforms. Current versions of Windows (Win 2k, sp4 and higher) actually
 * do contain implementations of @p ntop and @p pton, so we may switch to 
 * these native versions shortly.
 *
 * @file       slp_win32.c
 * @author     Matthew Peterson, Paul Vixie, 
 *             John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#include "slp_net.h"

#define NS_INT16SZ      2
#define NS_INADDRSZ     4
#define NS_IN6ADDRSZ    16

#define snprintf _snprintf
#define strlcpy strncpy

/** Format an IPv4 address as a display string.
 *
 * @param[in] src - A binary buffer containing the IPv4 address 
 *    to be formatted.
 * @param[out] dst - A string buffer in which to store the 
 *    formatted address string.
 * @param[in] size - The size of @p dst.
 *
 * @returns A const pointer to @p dst.
 *
 * @note Uses no static variables.
 * @note Takes an unsigned char *, not an in_addr as input.
 *
 * @internal
 */
static const char * inet_ntop4(const uint8_t * src, char * dst, 
      size_t size)
{
   static const char * fmt = "%u.%u.%u.%u";

   char tmp[sizeof "255.255.255.255"];

   if ((size_t)snprintf(tmp, sizeof(tmp), fmt, src[0], src[1], 
         src[2], src[3]) >= size)
   {
      errno = ENOSPC;
      return 0;
   }
   strlcpy(dst, tmp, size);
   return dst;
}

/** Format an IPv6 address as a display string.
 *
 * @param[in] src - A binary buffer containing the IPv6 address 
 *    to be formatted.
 * @param[out] dst - A string buffer in which to store the
 *    formatted address string.
 * @param[in] size - The size of @p dst.
 *
 * @returns A const pointer to @p dst.
 *
 * @internal
 */
static const char * inet_ntop6(const uint8_t * src, char * dst, 
      size_t size)
{
   /** @note int32_t and int16_t need only be "at least" large enough
    * to contain a value of the specified size. On some systems, like
    * Crays, there is no such thing as an integer variable with 16 bits.
    * Keep this in mind if you think this function should have been coded
    * to use pointer overlays. All the world's not a VAX.
    */
   char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
   struct { int base, len; } best, cur;
   unsigned int words[NS_IN6ADDRSZ / NS_INT16SZ];
   int i;

   /* Preprocess:
    * Copy the input (bytewise) array into a wordwise array.
    * Find the longest run of 0x00's in src[] for :: shorthanding.
    */
   memset(words, '\0', sizeof words);
   for (i = 0; i < NS_IN6ADDRSZ; i++)
      words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
   best.base = cur.base = -1;
   best.len = cur.len = 0;
   for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) 
   {
      if (words[i] == 0) 
      {
         if (cur.base == -1)
            cur.base = i, cur.len = 1;
         else
            cur.len++;
      } 
      else 
      {
         if (cur.base != -1) 
         {
            if (best.base == -1 || cur.len > best.len)
               best = cur;
            cur.base = -1;
         }
      }
   }
   if (cur.base != -1) 
      if (best.base == -1 || cur.len > best.len)
         best = cur;
   if (best.base != -1 && best.len < 2)
      best.base = -1;

   /* Format the result. */
   tp = tmp;
   for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) 
   {
      /* Are we inside the best run of 0x00's? */
      if (best.base != -1 && i >= best.base && i < (best.base + best.len)) 
      {
         if (i == best.base)
            *tp++ = ':';
         continue;
      }
      /* Are we following an initial run of 0x00s or any real hex? */
      if (i != 0)
         *tp++ = ':';
      /* Is this address an encapsulated IPv4? */
      if (i == 6 && best.base == 0 
            && (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) 
      {
         if (!inet_ntop4(src+12, tp, sizeof tmp - (tp - tmp)))
            return 0;
         tp += strlen(tp);
         break;
      }
      snprintf(tp, tmp + sizeof tmp - tp, "%x", words[i]);
      tp += strlen(tp);
   }

   /* Was it a trailing run of 0x00's? */
   if (best.base != -1 && (best.base + best.len) == (NS_IN6ADDRSZ / NS_INT16SZ))
      *tp++ = ':';
   *tp++ = '\0';

   /* check for overflow, copy, and we're done. */
   if ((size_t)(tp - tmp) > size) 
   {
      errno = ENOSPC;
      return 0;
   }
   strlcpy(dst, tmp, size);
   return dst;
}

/** Convert a network format address to presentation format.
 *
 * @param[in] af - The address family (AF_INET or AF_INET6) of @p src.
 * @param[in] src - The binary IPv4 or IPv6 address to be formatted.
 * @param[out] dst - A string buffer in which to store the
 *    formatted address string.
 * @param[in] size - The size of @p dst.
 *
 * @returns A const pointer to @p dst on success; or NULL on failure, 
 *    and sets @a errno to EAFNOSUPPORT.
 */
const char * inet_ntop(int af, const void * src, char * dst, size_t size) 
{
   switch (af) 
   {
      case AF_INET:
         return (inet_ntop4(src, dst, size));
      case AF_INET6:
         return (inet_ntop6(src, dst, size));
   }
   errno = EAFNOSUPPORT;
   return 0;
}

/** Converts a string IPv4 address to a binary address buffer.
 *
 * Works like inet_aton() but without all the hexadecimal and shorthand.
 *
 * @param[in] src - The string IPv4 address to be converted.
 * @param[out] dst - The address of a buffer to receive the binary 
 *    IPv4 address.
 *
 * @returns True (1) if @p src is a valid dotted quad, else False (0).
 *
 * @note Doesn't touch @p dst unless it's returning 1.
 *
 * @internal
 */
static int inet_pton4(const char * src, uint8_t * dst) 
{
   static const char digits[] = "0123456789";
   int saw_digit, octets, ch;
   uint8_t tmp[NS_INADDRSZ], * tp;

   saw_digit = 0;
   octets = 0;
   *(tp = tmp) = 0;
   while ((ch = *src++) != '\0') 
   {
      const char * pch;

      if ((pch = strchr(digits, ch)) != 0) 
      {
         unsigned int newc = *tp * 10 + (int)(pch - digits);

         if (saw_digit && *tp == 0)
            return 0;
         if (newc > 255)
            return 0;
         *tp = (unsigned char)newc;
         if (!saw_digit) 
         {
            if (++octets > 4)
               return 0;
            saw_digit = 1;
         }
      } 
      else if (ch == '.' && saw_digit) 
      {
         if (octets == 4)
            return 0;
         *++tp = 0;
         saw_digit = 0;
      } 
      else
         return 0;
   }
   if (octets < 4)
      return 0;
   memcpy(dst, tmp, NS_INADDRSZ);
   return 1;
}

/** Converts a string IPv6 address to a binary address buffer.
 *
 * @param[in] src - The string IPv6 address to be converted.
 * @param[out] dst - A pointer to a buffer in which to store the 
 *    converted binary IPv6 address.
 *
 * @returns True (1) if @p src is a valid [RFC1884 2.2] address; 
 *    False (0), if not.
 *
 * @note Doesn't touch @p dst unless it's returning True.
 * @note The sequence '::' in a full address is silently ignored.
 *
 * @internal
 */
static int inet_pton6(const char * src, uint8_t * dst) 
{
   static const char xdigits_l[] = "0123456789abcdef",

   xdigits_u[] = "0123456789ABCDEF";
   uint8_t tmp[NS_IN6ADDRSZ], * tp, * endp, * colonp;
   const char * xdigits, * curtok;
   int ch, saw_xdigit;
   unsigned int val;

   memset((tp = tmp), '\0', NS_IN6ADDRSZ);
   endp = tp + NS_IN6ADDRSZ;
   colonp = NULL;
   /* Leading :: requires some special handling. */
   if (*src == ':')
      if (*++src != ':')
         return 0;
   curtok = src;
   saw_xdigit = 0;
   val = 0;
   while ((ch = *src++) != '\0') 
   {
      const char * pch;

      if ((pch = strchr((xdigits = xdigits_l), ch)) == 0)
         pch = strchr((xdigits = xdigits_u), ch);
      if (pch != NULL) 
      {
         val <<= 4;
         val |= (pch - xdigits);
         if (val > 0xffff)
            return 0;
         saw_xdigit = 1;
         continue;
      }
      if (ch == ':') 
      {
         curtok = src;
         if (!saw_xdigit) 
         {
            if (colonp)
               return 0;
            colonp = tp;
            continue;
         }
         if (tp + NS_INT16SZ > endp)
            return 0;
         *tp++ = (unsigned char) (val >> 8) & 0xff;
         *tp++ = (unsigned char) val & 0xff;
         saw_xdigit = 0;
         val = 0;
         continue;
      }
      if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) 
            && inet_pton4(curtok, tp) > 0) 
      {
         tp += NS_INADDRSZ;
         saw_xdigit = 0;
         break;   /* '\0' was seen by inet_pton4(). */
      }
      return 0;
   }
   if (saw_xdigit) 
   {
      if (tp + NS_INT16SZ > endp)
         return 0;
      *tp++ = (unsigned char) (val >> 8) & 0xff;
      *tp++ = (unsigned char) val & 0xff;
   }
   if (colonp != 0)
   {
      /* Since some memmove()'s erroneously fail to handle
       * overlapping regions, we'll do the shift by hand.
       */
      const int n = (int)(tp - colonp);
      int i;

      for (i = 1; i <= n; i++) 
      {
         endp[- i] = colonp[n - i];
         colonp[n - i] = 0;
      }
      tp = endp;
   }
   if (tp != endp)
      return 0;
   memcpy(dst, tmp, NS_IN6ADDRSZ);
   return 1;
}

/** Converts a string IPv4 or IPv6 address to a binary address buffer.
 *
 * @param[in] af - The address family (AF_INET or AF_INET6) of @p src.
 * @param[in] src - The string address to be converted.
 * @param[out] dst - A pointer to a buffer in which to store the 
 *    converted binary address.
 *
 * @returns True (1) if the address was valid for the specified address 
 *    family; False (0) if the address wasn't valid (@p dst is untouched 
 *    in this case); Error (-1) if some other error occurred (@p dst is 
 *    untouched in this case, too).
 */
int inet_pton(int af, const char * src, void * dst) 
{
   switch (af) 
   {
      case AF_INET:
         return (inet_pton4(src, dst));
      case AF_INET6:
         return (inet_pton6(src, dst));
   }
   errno = EAFNOSUPPORT;
   return -1;
}

/*=========================================================================*/
