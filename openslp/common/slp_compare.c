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

/** Utility functions that deal with SLP strings and string-lists.
 *
 * These functions provide "case" and "no-case" versions of string matching
 * routines.
 *
 * @file       slp_compare.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeStrings
 */

#include "slp_types.h"
#include "slp_xmalloc.h"
#include "slp_compare.h"
#include "slp_net.h"
#include "slp_iface.h"

#ifdef HAVE_ICU
/* --- ICU header stuff - so we don't have to include their headers. --- */
#define u_strFromUTF8 u_strFromUTF8_3_2
#define u_strncasecmp u_strncasecmp_3_2

typedef int UErrorCode;
typedef uint16_t UChar;

UChar * u_strFromUTF8(UChar * dest, int32_t destCapacity,
      int32_t * pDestLength, const char * src, int32_t srcLength,
      UErrorCode * pErrorCode);

int32_t u_strncasecmp(const UChar * s1, const UChar * s2,
      int32_t n, uint32_t options);
/* --- End of ICU header stuff --- */
#endif	/* HAVE_ICU */

#ifndef _WIN32
# ifndef HAVE_STRNCASECMP
/** Case-insensitive, size-constrained, lexical comparison. 
 *
 * Compares a specified maximum number of characters of two strings for 
 * lexical equivalence in a case-insensitive manner.
 *
 * @param[in] s1 - The first string to be compared.
 * @param[in] s2 - The second string to be compared.
 * @param[in] len - The maximum number of characters to compare.
 * 
 * @return Zero if at least @p len characters of @p s1 are the same as
 *    the corresponding characters in @p s2 within the ASCII printable 
 *    range; a value less than zero if @p s1 is lexically less than
 *    @p s2; or a value greater than zero if @p s1 is lexically greater
 *    than @p s2.
 *
 * @internal
 */
int strncasecmp(const char * s1, const char * s2, size_t len)
{
   while (*s1 && (*s1 == *s2 || tolower(*s1) == tolower(*s2)))
   {
      len--;
      if (len == 0)
         return 0;
      s1++;
      s2++;
   }
   return len? (int)(*(unsigned char *)s1 - (int)*(unsigned char *)s2): 0;
}
# endif 

# ifndef HAVE_STRCASECMP
/** Case-insensitive lexical comparison. 
 *
 * Compares two strings for lexical equivalence in a case-insensitive 
 * manner.
 *
 * @param[in] s1 - The first string to be compared.
 * @param[in] s2 - The second string to be compared.
 * 
 * @return Zero if @p s1 is the same as @p s2 within the ASCII printable 
 *    range; a value less than zero if @p s1 is lexically less than @p s2;
 *    or a value greater than zero if @p s1 is lexically greater than 
 *    @p s2.
 *
 * @internal
 */
int strcasecmp(const char * s1, const char * s2)
{
   while (*s1 && (*s1 == *s2 || tolower(*s1) == tolower(*s2)))
      s1++, s2++;
   return (int)(*(unsigned char *)s1 - (int)*(unsigned char *)s2);
}
# endif 
#endif 

/** Convert a character to upper case using US ASCII rules. */
#define usaupr(c) (((c) & 0xC0) == 0x40? (c) & ~0x20: (c))

/** Determines if a specified character is a valid hexadecimal character.
 *
 * @param[in] c - The character to examine.
 *
 * @returns 1 if @p c is a valid hexadecimal character; 0 if not.
 */
static int ishex(int c)	
{
   c = usaupr(c); 
   return (c >= '0' && c <= '9') || ((c >= 'A' && c <= 'F')? 1: 0);
}

/** Converts a valid hexadecimal character into its binary equivalent.
 *
 * @param[in] c - The character to be converted.
 *
 * @returns The binary value of @p c.
 */
static int hex2bin(int c)
{
   c = usaupr(c); 
   return c - (c <= '9'? '0': 'A' - 10);
}

/** Unescape an SLP string in place.
 *
 * Replace escape sequences with corresponding character codes in a 
 * specified string.
 *
 * @param[in] len - The length in bytes of @p str.
 * @param[in,out] str - The string in which escape sequences should be 
 *    replaced with corresponding characters.
 *
 * @returns The new (shorter) length of @p str.
 *
 * @note Since only valid escapeable characters may be escaped, we will 
 *    assume that escaped characters are sequences of bytes within the 
 *    0x00 - 0x7F range (ascii subset of utf-8). 
 */
static int SLPUnescapeInPlace(size_t len, char * str)
{
   char * fp = str, * tp = str, * ep = str + len;
   while (fp < ep - 2)
   {
      char c = *fp++;
      if (c == '\\' && *fp && ishex(fp[0]) && ishex(fp[1]))
      {
         c = (char)(hex2bin(fp[0]) * 16 + hex2bin(fp[1]));
         fp += 2;
         len -= 2;
      }
      *tp++ = c;
   }
   return (int)len;
}

/** Fold internal white space within a string.
 *
 * Folds all internal white space to a single space character within a 
 * specified string. Modified the @p str parameter with the result and 
 * returns the new length of the string.
 *
 * @param[in] len - The length in bytes of @p str.
 * @param[in,out] str - The string from which extraneous white space 
 *    should be removed.
 *
 * @return The new (shorter) length of @p str.
 *
 * @note This routine assumes that leading and trailing white space have
 *    already been removed from @p str.
 */
static int SLPFoldWhiteSpace(size_t len, char * str)
{         
   char * p = str, * ep = str + len;
   while (p < ep)
   {
      if (isspace(*p))
      {
         char * ws2p = ++p;         /* Point ws2p to the second ws char. */
         while (isspace(*p))        /* Scan till we hit a non-ws char. */
            p++;
         len -= p - ws2p;           /* Reduce the length by extra ws. */
         memmove(ws2p, p, ep - p);  /* Overwrite the extra white space. */
      }
      p++;
   }
   return (int)len;
}

/** Lexical compare routine. 
 *
 * Performs a lexical string compare on two normalized UTF-8 strings as 
 * described in RFC 2608, section 6.4.
 * 
 * @param[in] str1 - A pointer to string to be compared.
 * @param[in] str2 - A pointer to string to be compared.
 * @param[in] length - The maximum length to compare in bytes.
 * 
 * @return Zero if @p str1 is equal to @p str2, less than zero if @p str1 
 *    is greater than @p str2, greater than zero if @p str1 is less than 
 *    @p str2.
 */
static int SLPCompareNormalizedString(const char * str1,
      const char * str2, size_t length)
{
#ifdef HAVE_ICU
   int result;
   UErrorCode uerr = 0;
   UChar * ustr1 = xmalloc((length + 1) * sizeof(UChar));
   UChar * ustr2 = xmalloc((length + 1) * sizeof(UChar));
   if (ustr1 && ustr2)
   {
      u_strFromUTF8(ustr1, (int32_t)length + 1, 0, str1, 
            (int32_t)length, &uerr);
      u_strFromUTF8(ustr2, (int32_t)length + 1, 0, str2, 
            (int32_t)length, &uerr);
   }
   if (ustr1 != 0 && ustr2 != 0 && uerr == 0)
      result = (int)u_strncasecmp(ustr1, ustr2, (int32_t)length, 0);
   else
      result = strncasecmp(str1, str2, length);
   xfree(ustr1);
   xfree(ustr2);
   return result;
#else
   return strncasecmp(str1, str2, length);
#endif /* HAVE_ICU */
}

/** Normalizes a string
 *
 * Normalizes a string by (optionally) removing leading and trailing white space,
 * folding internal white space, folding case (upper->lower), and unescaping
 * the string.
 * 
 * @param[in] len - The length of the string to be normalised, in bytes.
 * @param[in] srcstr - A pointer to the string to be normalised.
 * @param[in] dststr - A pointer to the buffer for the normalised string.
 * @param[in] trim - A flag to specify whether to trim leading and trailing space
 *                   completely (if non-zero) or just fold it (if zero).
 *
 * @return Size of normalised string in bytes.
 *
 * @remarks @p dststr may be the same as @p srcstr for "update in place".
 */
size_t SLPNormalizeString(size_t len, const char * srcstr, char * dststr, int trim)
{
   char *upd = dststr;
   while (len > 0 && *srcstr)
   {
      if (isspace(*srcstr))
      {
         while (isspace(*srcstr) && len > 0)
         {
            ++srcstr, --len;
         }
         if (!trim || (upd != dststr && len > 0))
            /* Internal whitespace */
            *upd++ = ' ';
      }
      else if (*srcstr == '\\')
      {
         if (len < 3)
         {
            /* This indicates incorrect escaping, but just copy verbatim */
            *upd++ = *srcstr++;
            --len;
         }
         else
         {
            if (ishex(srcstr[1]) && ishex(srcstr[2]))
            {
               *upd++ = (char)(hex2bin(srcstr[0]) * 16 + hex2bin(srcstr[1]));
               len -= 3;
            }
            else
            {
               /* This indicates incorrect escaping, but just copy verbatim */
               *upd++ = *srcstr++;
               --len;
            }
         }
      }
      else
      {
         *upd++ = tolower(*srcstr++);
         --len;
      }
   }
   return upd - dststr;
}

/** Compares two non-normalized strings. 
 *
 * Normalizes two strings by removing leading and trailing white space,
 * folding internal white space and unescaping the strings first, and then
 * calling SLPCompareNormalizedString (as per RFC 2608, section 6.4).
 * 
 * @param[in] str1 - A pointer to string to be compared.
 * @param[in] str1len - The length of str1 in bytes.
 * @param[in] str2 - A pointer to string to be compared.
 * @param[in] str2len - The length of str2 in bytes.
 * 
 * @return Zero if @p str1 is equal to @p str2, less than zero if @p str1 
 *    is greater than @p str2, greater than zero if @p str1 is less than 
 *    @p str2.
 */
int SLPCompareString(size_t str1len, const char * str1, 
      size_t str2len, const char * str2)
{
   int result;
   char * cpy1, * cpy2;

   /* Remove leading white space. */
   while (str1len && isspace(*str1))
      str1++, str1len--;
   while (str2len && isspace(*str2))
      str2++, str2len--;

   /* Remove trailing white space. */
   while (str1len && isspace(str1[str1len - 1]))
      str1len--;
   while (str2len && isspace(str2[str2len - 1]))
      str2len--;

   /*A quick check for empty strings before we start xmemduping and xfreeing*/
   if (str1len == 0 || str2len == 0)
   {
      if(str1len == str2len)
         return 0;
      if(str1len < str2len)
         return -1;
      return 1;
   }

   /* Make modifiable copies. If either fails, compare original strings. */
   cpy1 = xmemdup(str1, str1len);
   cpy2 = xmemdup(str2, str2len);
   if (cpy1 != 0 && cpy2 != 0)
   {
      /* Unescape copies in place. */
      str1len = SLPUnescapeInPlace(str1len, cpy1);
      str2len = SLPUnescapeInPlace(str2len, cpy2);

      /* Fold white space in place. */
      str1len = SLPFoldWhiteSpace(str1len, cpy1);
      str2len = SLPFoldWhiteSpace(str2len, cpy2);

      /* Reset original pointers to modified copies. */
      str1 = cpy1;
      str2 = cpy2;
   }

   /* Comparison logic. */
   if (str1len == str2len)
      result = SLPCompareNormalizedString(str1, str2, str1len);
   else if (str1len > str2len)
      result = -1;
   else
      result = 1;

   xfree(cpy1);
   xfree(cpy2);

   return result;
}

/** Compare service type for matching naming authority. 
 *
 * Compares a service type string with a naming authority to determine 
 * if they refer to the same naming authority, as described in RFC 2608, 
 * section ??.
 * 
 * @param[in] srvtype - The service type to be compared.
 * @param[in] srvtypelen - The length of @p srvtype in bytes.
 * @param[in] namingauth - The naming authority to be matched.
 * @param[in] namingauthlen - The length of @p namingauth in bytes.
 * 
 * @return Zero if @p srvtype matches @p namingauth; non-zero if not.
 */
int SLPCompareNamingAuth(size_t srvtypelen, const char * srvtype, 
      size_t namingauthlen, const char * namingauth)
{
   const char * dot;
   size_t srvtypenalen;

   if (namingauthlen == 0xffff)
      return 0;            /* match all naming authorities */

   dot = memchr(srvtype, '.', srvtypelen);
   if (!namingauthlen)
      return dot? 1: 0;    /* IANA naming authority */

   srvtypenalen = srvtypelen - (dot + 1 - srvtype);
   if (srvtypenalen != namingauthlen)
      return 1;

   if (SLPCompareNormalizedString(dot + 1, namingauth, namingauthlen) == 0)
      return 0;

   return 1;
}

/** Compare service types. 
 *
 * Determine if two service type strings refer to the same service type.
 * 
 * @param[in] lsrvtype - The first service type to be compared.
 * @param[in] lsrvtypelen - The length of @p lsrvtype in bytes.
 * @param[in] rsrvtype - The second service type to be compared.
 * @param[in] rsrvtypelen - The length of @p rsrvtype in bytes.
 * 
 * @return Zero if @p lsrvtype is the same as @p lsrvtype; non-zero 
 *    if they are different.
 */
int SLPCompareSrvType(size_t lsrvtypelen, const char * lsrvtype, 
      size_t rsrvtypelen, const char * rsrvtype)
{
   char * colon;

   /* Skip "service:" */
   if (strncasecmp(lsrvtype, "service:", lsrvtypelen > 8? 
         8: lsrvtypelen) == 0)
   {
      lsrvtypelen = lsrvtypelen - 8;
      lsrvtype = lsrvtype + 8;
   }
   if (strncasecmp(rsrvtype, "service:", rsrvtypelen > 8? 
         8: rsrvtypelen) == 0)
   {
      rsrvtypelen = rsrvtypelen - 8;
      rsrvtype = rsrvtype + 8;
   }
   if (memchr(lsrvtype, ':', lsrvtypelen))
   {
      /* lsrvtype is uses concrete type so strings must be identical. */
      if (lsrvtypelen == rsrvtypelen)
         return SLPCompareNormalizedString(lsrvtype, rsrvtype, lsrvtypelen);
      return 1;
   }
   colon = memchr(rsrvtype, ':', rsrvtypelen);
   if (colon)
   {
      /* lsrvtype is abstract only and rsrvtype is concrete. */
      if (lsrvtypelen == (size_t)(colon - rsrvtype))
         return SLPCompareNormalizedString(lsrvtype, rsrvtype, lsrvtypelen);
      return 1;
   }

   /* lsrvtype and rsrvtype are abstract only. */
   if (lsrvtypelen == rsrvtypelen)
      return SLPCompareNormalizedString(lsrvtype, rsrvtype, lsrvtypelen);
   return 1;
}

/** Scan a string list for a string. 
 *
 * Determine if a specified string list contains a specified string.
 * 
 * @param[in] list - A list of strings to search for @p string.
 * @param[in] listlen - The length of @p list in bytes.
 * @param[in] string - A string to locate in @p list.
 * @param[in] stringlen - The length of @p string in bytes.
 * 
 * @return Non-zero if @p string is found in @p list; zero if not.
 *
 * @remarks The @p list parameter is a zero-terminated string consisting
 *    of a comma-separated list of sub-strings. This routine actually
 *    determines if a specified sub-string (@p string) matches one of
 *    the sub-strings in this list.
 */
int SLPContainsStringList(size_t listlen, const char * list, size_t stringlen,
      const char * string)
{
   const char * listend = list + listlen;
   const char * itembegin = list;
   const char * itemend = itembegin;

   while (itemend < listend)
   {
      itembegin = itemend;

      /* Seek to the end of the next list item, break on commas. */
      while (itemend != listend && itemend[0] != ',')
         itemend++;

      if (SLPCompareString(itemend - itembegin, itembegin, 
            stringlen, string) == 0)
         return 1 + (itembegin - list);         /* 1-based index of the position of the string in the list */

      itemend++;
   }
   return 0;
}

/** Intersects two string lists. 
 *
 * Calculates the number of common entries between two string-lists.
 * 
 * @param[in] list1 - A pointer to the string-list to be checked
 * @param[in] list1len - The length in bytes of the list to be checked
 * @param[in] list2 - A pointer to the string-list to be checked
 * @param[in] list2len - The length in bytes of the list to be checked
 * 
 * @return The number of common entries between @p list1 and @p list2.
 */
int SLPIntersectStringList(size_t list1len, const char * list1,
      size_t list2len, const char * list2)
{
   int result = 0;
   const char * listend = list1 + list1len;
   const char * itembegin = list1;
   const char * itemend = itembegin;

   while (itemend < listend)
   {
      itembegin = itemend;

      /* Seek to the end of the next list item, break on commas. */
      while (itemend != listend && itemend[0] != ',')
         itemend++;

      if (SLPContainsStringList(list2len, list2, 
            itemend - itembegin, itembegin))
         result++;

      itemend++;
   }
   return result;
}

/** Intersects two string lists, and removes the common entries from
 * the second list. 
 *
 * @param[in] list1len - The length in bytes of the list to be checked
 * @param[in] list1 - A pointer to the string-list to be checked
 * @param[in] list2len - The length in bytes of the list to be checked
 *     and updated
 * @param[in] list2 - A pointer to the string-list to be checked and
 *     updated
 * 
 * @return The number of common entries between @p list1 and @p list2.
 */
int SLPIntersectRemoveStringList(int list1len,
                                 const char* list1,
                                 int* list2len,
                                 char* list2)
{
    int result = 0;
    int pos;
    char* listend = (char*)list1 + list1len;
    char* itembegin = (char*)list1;
    char* itemend = itembegin;
    char* list2end = (char*)list2+(*list2len);

    while(itemend < listend)
    {
        itembegin = itemend;

        /* seek to the end of the next list item */
        while(1)
        {
            if(itemend == listend || *itemend == ',')
            {
                if(*(itemend - 1) != '\\')
                {
                    break;
                }
            }

            itemend ++;
        }

        if ((pos = SLPContainsStringList(*list2len,
                                       list2,
                                       itemend - itembegin,
                                       itembegin)) != 0)
        {
            /* String found in the list at position pos (1-based) */
            /* Remove it from list2                               */
            char* dest = list2+(pos-1);
            char* src = dest+(itemend-itembegin);

            result++;

            if (src < list2end)
            {
                if (*src == ',')
                    ++src;
            }
            while (src < list2end)
            {
                *dest++ = *src++;
            }
            list2end = dest;
        }

        itemend ++;    
    }

    *list2len = list2end-(char *)list2;

    return result;
}

/** Take the union of two string lists. 
 *
 * Generates a string list that contains all unique strings within 
 * two specified string lists.
 * 
 * @param[in] list1 - A pointer to the first string-list.
 * @param[in] list1len - The length in bytes of @p list1.
 * @param[in] list2 - A pointer to the second string-list.
 * @param[in] list2len - The length in bytes of @p list2.
 * @param[out] unionlist - A pointer to a buffer that will receive
 *    the union of @p list1 and @p list2.
 * @param[in,out] unionlistlen - A pointer to the size in bytes of the 
 *    @p unionlist buffer on entry; also receives the number of bytes 
 * written to the @p unionlist buffer on success.
 * 
 * @return The length of the resulting union list, or a negative value 
 *    if @p unionlist is not large enough. If a negative value is returned
 *    @p *unionlist will return the required size of @p unionlist.
 * 
 * @remarks In order ensure that @p unionlist does not contain duplicates, 
 *    @p list1 must not have any duplicates. Also, as a speed optimization, 
 *    if neither @p list1 nor @p list2 contain internal duplicates, the 
 *    larger list should be passed in as @p list1.
 *
 * @remarks To avoid buffer overflow errors pass @p list1len + 
 *    @p list2len + 1 as the value for @p unionlistlen.
 */
int SLPUnionStringList(size_t list1len, const char * list1, size_t list2len, 
      const char * list2, size_t * unionlistlen, char * unionlist)
{
   char * listend = (char *)list2 + list2len;
   char * itembegin = (char *)list2;
   char * itemend = itembegin;
   size_t itemlen;
   size_t copiedlen;

   if (unionlist == 0 || *unionlistlen == 0 || *unionlistlen < list1len)
   {
      *unionlistlen = list1len + list2len + 1;
      return  -1;
   }

   /* Copy list1 into the unionlist since it should not have any duplicates */
   memcpy(unionlist, list1, list1len);
   copiedlen = list1len;

   while (itemend < listend)
   {
      itembegin = itemend;

      /* seek to the end of the next list item */
      while (1)
      {
         if (itemend == listend || *itemend == ',')
            if (*(itemend - 1) != '\\')
               break;
         itemend++;
      }

      itemlen = itemend - itembegin;
      if (SLPContainsStringList(list1len, list1, itemlen, itembegin) == 0)
      {
         if (copiedlen + itemlen + 1 > *unionlistlen)
         {
            *unionlistlen = list1len + list2len + 1;
            return  -1;
         }

         /* append a comma if not the first entry*/
         if (copiedlen)
         {
            unionlist[copiedlen] = ',';
            copiedlen++;
         }
         memcpy(unionlist + copiedlen, itembegin, itemlen);
         copiedlen += itemlen;
      }
      itemend++;
   }
   *unionlistlen = copiedlen;
   return (int)copiedlen;
}

/** Test if a list is a proper sub-set of another list. 
 *
 * Determines if @p sublist is a proper sub-set of @p list.
 * 
 * @param[in] list - The list to compare @p sublist against.
 * @param[in] listlen - The length of @p list in bytes.
 * @param[in] sublist - The sub-list to be compared against @p list.
 * @param[in] sublistlen - The length of @p sublist in bytes.
 * 
 * @return A Boolean value; true if @p sublist is a proper subset of
 *    @p list; false if not.
 */
int SLPSubsetStringList(size_t listlen, const char * list, 
      size_t sublistlen, const char * sublist)
{
   unsigned curpos;
   int sublistcount;

   /* Quick check for empty lists. Note that an empty sub-list is not 
    * considered a proper subset of any value of list.
    */
   if (sublistlen == 0 || listlen == 0)
      return 0;

   /* Count the items in sublist. */
   curpos = 0;
   sublistcount = 1;
   while (curpos < sublistlen)
   {
      if (sublist[curpos] == ',')
         sublistcount++;
      curpos++;
   }

   /* Intersect the lists, return 1 if proper subset, 0 if not. */
   return SLPIntersectStringList(listlen, list, sublistlen, 
         sublist) == sublistcount? 1 : 0;
}

/** Test URL conformance. 
 *
 * Test if a service URL conforms to accepted syntax as described in 
 * RFC 2608, section ??.
 * 
 * @param[in] srvurl     The service url string to check.
 * @param[in] srvurllen  The length of @p srvurl in bytes.
 * 
 * @return Zero if @p srvurl has acceptable syntax, non-zero on failure.
 */
int SLPCheckServiceUrlSyntax(const char * srvurl, size_t srvurllen)
{
   (void)srvurl;
   (void)srvurllen;

   /*!@todo Do we actually need to do something here to ensure correct
    * service-url syntax, or should we expect that it will be used
    * by smart developers who know that ambiguities could be encountered
    * if they don't?

   if (srvurllen < 8)
      return 1;
   if (strncasecmp(srvurl, "service:",8))
      return 1;
   return 0;

    */
   return 0;
}

/** Test URL conformance. 
 *
 * Test if a service URL conforms to accepted syntax as described in 
 * RFC 2608, section ??.
 * 
 * @param[in] attrlist - The attribute list string to check.
 * @param[in] attrlistlen - The length of @p attrlist in bytes.
 * 
 * @return Zero if @p srvurl has acceptable syntax, non-zero on failure.
 */
int SLPCheckAttributeListSyntax(const char * attrlist, size_t attrlistlen)
{
   const char * slider;
   const char * end;

   if (attrlistlen)
   {
      slider = attrlist;
      end = attrlist + attrlistlen;
      while (slider != end)
      {
         if (*slider == '(')
         {
            while (slider != end)
            {
               if (*slider == '=')
                  return 0;
               slider++;
            }
            return 1;
         }
         slider++;
      }
   }
   return 0;
}

/*=========================================================================*/
