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
 * @ingroup    CommonCode
 */

#include <string.h>
#include <ctype.h>

#include "slp_compare.h"

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
int strncasecmp(const char *s1, const char *s2, size_t len)
{
   while (*s1 && (*s1 == *s2 || tolower(*s1) == tolower(*s2)))
   {
      len--;
      if (len == 0) return 0;
      s1++;
      s2++;
   }
   return (int) *(unsigned char *)s1 - (int) *(unsigned char *)s2;
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
int strcasecmp(const char *s1, const char *s2)
{
   while (*s1 && (*s1 == *s2 || tolower(*s1) == tolower(*s2)))
   {
      s1++;
      s2++;
   }
   return (int) *(unsigned char *)s1 - (int) *(unsigned char *)s2;
}
# endif
#endif 

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
int SLPCompareString(int str1len,
      const char* str1,
      int str2len,
      const char* str2)
{
   /* TODO: fold whitespace and handle escapes*/
   if (str1len == str2len)
   {
      if (str1len <= 0)
         return (0);
      return strncasecmp(str1,str2,str1len);
   }
   else if (str1len > str2len)
   {
      return -1;
   }

   return 1;
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
int SLPCompareNamingAuth(int srvtypelen,
      const char* srvtype,
      int namingauthlen,
      const char* namingauth)
{
   const char *dot;

   if (namingauthlen == 0xffff) /* match all naming authorities */
      return 0;

   /* Skip "service:" */
   if ((srvtypelen > 8) && (strncasecmp(srvtype,"service:",8) == 0))
   {
      srvtypelen -= 8;
      srvtype += 8;
   }
   /* stop search at colon after naming authority (if there is one) */
   dot = memchr(srvtype,':',srvtypelen);
   if (dot)
      srvtypelen = dot - srvtype;

   dot = memchr(srvtype,'.',srvtypelen);

   if (!namingauthlen)     /* IANA naming authority */
      return dot ? 1 : 0;

   if (dot)
   {
      int srvtypenalen = srvtypelen - (dot + 1 - srvtype);

      if (srvtypenalen != namingauthlen)
         return 1;

      if (strncasecmp(dot + 1, namingauth, namingauthlen) == 0)
         return 0;
   }

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
int SLPCompareSrvType(int lsrvtypelen,
      const char* lsrvtype,
      int rsrvtypelen,
      const char* rsrvtype)
{
   char* colon;

   /* Skip "service:" */
   if (strncasecmp(lsrvtype,"service:",lsrvtypelen > 8 ? 8 : lsrvtypelen) == 0)
   {
      lsrvtypelen = lsrvtypelen - 8;
      lsrvtype = lsrvtype + 8;
   }
   if (strncasecmp(rsrvtype,"service:",rsrvtypelen > 8 ? 8 : rsrvtypelen) == 0)
   {
      rsrvtypelen = rsrvtypelen - 8;
      rsrvtype = rsrvtype + 8;
   }

   if (memchr(lsrvtype,':',lsrvtypelen))
   {
      /* lsrvtype is uses concrete type so strings must be identical */
      if (lsrvtypelen == rsrvtypelen)
      {
         return strncasecmp(lsrvtype,rsrvtype,lsrvtypelen);
      }

      return 1;
   }

   colon = memchr(rsrvtype,':',rsrvtypelen);
   if (colon)
   {
      /* lsrvtype is abstract only and rsrvtype is concrete */
      if (lsrvtypelen == (colon - rsrvtype))
      {
         return strncasecmp(lsrvtype,rsrvtype,lsrvtypelen);
      }
      return 1;
   }

   /* lsrvtype and rsrvtype are  abstract only */
   if (lsrvtypelen == rsrvtypelen)
   {
      return strncasecmp(lsrvtype,rsrvtype,lsrvtypelen);
   }

   return 1;
}

/** Scan a string list for a string. 
 *
 * Determine if a specifed string list contains a specified string.
 * 
 * @param[in] list - A list of strings to search for @p string.
 * @param[in] listlen - The length of @p list in bytes.
 * @param[in] string - A string to locate in @p list.
 * @param[in] stringlen - The length of @p string in bytes.
 * 
 * @return Zero if @p string is found in @p list; non-zero if not. 
 *
 * @remarks The @p list parameter is a zero-terminated string consisting
 *    of a comma-separated list of sub-strings. This routine actually
 *    determines if a specified sub-string (@p string) matches one of
 *    the sub-strings in this list.
 */
int SLPContainsStringList(int listlen, 
      const char* list,
      int stringlen,
      const char* string)
{
   char* listend = (char*)list + listlen;
   char* itembegin = (char*)list;
   char* itemend = itembegin;

   while (itemend < listend)
   {
      itembegin = itemend;

      /* seek to the end of the next list item */
      while (1)
      {
         if (itemend == listend || *itemend == ',')
         {
            if (*(itemend - 1) != '\\')
            {
               break;
            }
         }

         itemend ++;
      }

      if (SLPCompareString(itemend - itembegin,
            itembegin,
            stringlen,
            string) == 0)
      {
         return 1;
      }

      itemend ++;
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
int SLPIntersectStringList(int list1len,
      const char* list1,
      int list2len,
      const char* list2)
{
   int result = 0;
   char* listend = (char*)list1 + list1len;
   char* itembegin = (char*)list1;
   char* itemend = itembegin;

   while (itemend < listend)
   {
      itembegin = itemend;

      /* seek to the end of the next list item */
      while (1)
      {
         if (itemend == listend || *itemend == ',')
         {
            if (*(itemend - 1) != '\\')
            {
               break;
            }
         }

         itemend ++;
      }

      if (SLPContainsStringList(list2len,
            list2,
            itemend - itembegin,
            itembegin))
      {
         result ++;
      }

      itemend ++;
   }

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
int SLPUnionStringList(int list1len,
      const char* list1,
      int list2len,
      const char* list2,
      int* unionlistlen,
      char * unionlist)
{
   char* listend = (char*)list2 + list2len;
   char* itembegin = (char*)list2;
   char* itemend = itembegin;
   int   itemlen;
   int   copiedlen;

   if (unionlist == 0 ||
         *unionlistlen == 0 ||
         *unionlistlen < list1len)
   {
      *unionlistlen = list1len + list2len + 1;
      return -1;
   }

   /* Copy list1 into the unionlist since it should not have any duplicates */
   memcpy(unionlist,list1,list1len);
   copiedlen = list1len;

   while (itemend < listend)
   {
      itembegin = itemend;

      /* seek to the end of the next list item */
      while (1)
      {
         if (itemend == listend || *itemend == ',')
         {
            if (*(itemend - 1) != '\\')
            {
               break;
            }
         }

         itemend ++;
      }

      itemlen = itemend - itembegin;
      if (SLPContainsStringList(list1len,
            list1,
            itemlen,
            itembegin) == 0)
      {
         if (copiedlen + itemlen + 1 > *unionlistlen)
         {

            *unionlistlen = list1len + list2len + 1;
            return -1;
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

      itemend ++;
   }

   *unionlistlen = copiedlen;

   return copiedlen;
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
int SLPSubsetStringList(int listlen,
      const char* list,
      int sublistlen,
      const char* sublist)
{
   /* count the items in sublist */
   int curpos;
   int sublistcount;

   if (sublistlen ==0 || listlen == 0)
   {
      return 0;
   }

   curpos = 0;
   sublistcount = 1;
   while (curpos < sublistlen)
   {
      if (sublist[curpos] == ',')
      {
         sublistcount ++;
      }
      curpos ++;
   }

   if (SLPIntersectStringList(listlen,
         list,
         sublistlen,
         sublist) == sublistcount)
   {
      return sublistcount;
   }

   return 0;
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
int SLPCheckServiceUrlSyntax(const char* srvurl,
      int srvurllen)
{
   /* TODO: Do we actually need to do something here to ensure correct
   * service-url syntax, or should we expect that it will be used
   * by smart developers who know that ambiguities could be encountered
   * if they don't?

   if(srvurllen < 8)
   {
   return 1;
   }

   if(strncasecmp(srvurl,"service:",8))
   {
   return 1;
   }        

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
int SLPCheckAttributeListSyntax(const char* attrlist,
      int attrlistlen)
{
   const char* slider;
   const char* end;

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
               {
                  return 0;
               }
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
