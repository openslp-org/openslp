/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_string.c                                               */
/*                                                                         */
/* Abstract:    Various functions that deal with SLP strings and           */
/*              string-lists                                               */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*     Please submit patches to http://www.openslp.org                     */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/* Copyright (C) 2000 Caldera Systems, Inc                                 */
/* All rights reserved.                                                    */
/*                                                                         */
/* Redistribution and use in source and binary forms, with or without      */
/* modification, are permitted provided that the following conditions are  */
/* met:                                                                    */ 
/*                                                                         */
/*      Redistributions of source code must retain the above copyright     */
/*      notice, this list of conditions and the following disclaimer.      */
/*                                                                         */
/*      Redistributions in binary form must reproduce the above copyright  */
/*      notice, this list of conditions and the following disclaimer in    */
/*      the documentation and/or other materials provided with the         */
/*      distribution.                                                      */
/*                                                                         */
/*      Neither the name of Caldera Systems nor the names of its           */
/*      contributors may be used to endorse or promote products derived    */
/*      from this software without specific prior written permission.      */
/*                                                                         */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS     */
/* `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT      */
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   */
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CALDERA      */
/* SYSTEMS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, */
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON       */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    */
/*                                                                         */
/***************************************************************************/

#include <string.h>
#include <ctype.h>

#include "slp_compare.h"
#include "slp_net.h"


#ifdef _WIN32
#else
#ifndef HAVE_STRNCASECMP
int
strncasecmp(const char *s1, const char *s2, size_t len)
{
    while ( *s1 && (*s1 == *s2 || tolower(*s1) == tolower(*s2)) )
    {
        len--;
        if(len == 0) return 0;
        s1++;
        s2++;
    }
    return(int) *(unsigned char *)s1 - (int) *(unsigned char *)s2;
}
#endif
#ifndef HAVE_STRCASECMP
int
strcasecmp(const char *s1, const char *s2)
{
    while ( *s1 && (*s1 == *s2 || tolower(*s1) == tolower(*s2)) )
    {
        s1++;
        s2++;
    }
    return(int) *(unsigned char *)s1 - (int) *(unsigned char *)s2;
}
#endif
#endif 


/*=========================================================================*/
int SLPCompareString(int str1len,
                     const char* str1,
                     int str2len,
                     const char* str2)
/* Does a lexical string compare as described in RFC 2608 section 6.4.     */
/*                                                                         */
/* TODO: Handle the whole utf8 spec                                        */
/*                                                                         */
/* str1 -       pointer to string to be compared                           */
/*                                                                         */
/* str1len -    length of str1 in bytes                                    */
/*                                                                         */
/* str2 -       pointer to string to be compared                           */
/*                                                                         */
/* str2len -    length of str2 in bytes                                    */
/*                                                                         */
/* Returns -    zero if strings are equal. >0 if str1 is greater than str2 */
/*              <0 if s1 is less than str2                                 */
/*=========================================================================*/
{
    /* TODO: fold whitespace and handle escapes*/
    if(str1len == str2len)
    {
        return strncasecmp(str1,str2,str1len);
    }
    else if(str1len > str2len)
    {
        return -1;
    }

    return 1;
}


/*=========================================================================*/
int SLPCompareNamingAuth(int srvtypelen,
                         const char* srvtype,
                         int namingauthlen,
                         const char* namingauth)
/* Does srvtype match namingauth                                           */
/*                                                                         */
/* TODO: Handle the whole utf8 spec                                        */
/*                                                                         */
/* srvtype -        pointer to service type to be compared                 */
/*                                                                         */
/* srvtypelen -     length of srvtype in bytes                             */
/*                                                                         */
/* namingauth -     pointer to naming authority to be matched              */
/*                                                                         */
/* namingauthlen -  length of naming authority in bytes                    */
/*                                                                         */
/* Returns -    zero if srvtype matches the naming authority. Nonzero if   */
/*              it doesn't                                                 */
/*=========================================================================*/
{
    const char *dot;
    int srvtypenalen;

    if(namingauthlen == 0xffff) /* match all naming authorities */
        return 0;

    dot = memchr(srvtype,'.',srvtypelen);

    if(!namingauthlen)     /* IANA naming authority */
        return dot ? 1 : 0;

    srvtypenalen = srvtypelen - (dot + 1 - srvtype);

    if(srvtypenalen != namingauthlen)
        return 1;

    if(strncasecmp(dot + 1, namingauth, namingauthlen) == 0)
        return 0;

    return 1;
}

/*=========================================================================*/
int SLPCompareSrvType(int lsrvtypelen,
                      const char* lsrvtype,
                      int rsrvtypelen,
                      const char* rsrvtype)
/* Does lsrvtype = rsrvtype?                                               */
/*                                                                         */
/* TODO: Handle the whole utf8 spec                                        */
/*                                                                         */
/* lsrvtype -       pointer to string to be compared                       */
/*                                                                         */
/* lsrvtypelen -    length of str1 in bytes                                */
/*                                                                         */
/* rsrvtype -       pointer to string to be compared                       */
/*                                                                         */
/* rsrvtypelen -    length of str2 in bytes                                */
/*                                                                         */
/* Returns -    zero if srvtypes are equal. Nonzero if they are not        */
/*=========================================================================*/
{
    char* colon;

    /* Skip "service:" */
    if(strncasecmp(lsrvtype,"service:",lsrvtypelen > 8 ? 8 : lsrvtypelen) == 0)
    {
        lsrvtypelen = lsrvtypelen - 8;
        lsrvtype = lsrvtype + 8;
    }
    if(strncasecmp(rsrvtype,"service:",rsrvtypelen > 8 ? 8 : rsrvtypelen) == 0)
    {
        rsrvtypelen = rsrvtypelen - 8;
        rsrvtype = rsrvtype + 8;
    }

    if(memchr(lsrvtype,':',lsrvtypelen))
    {
        /* lsrvtype is uses concrete type so strings must be identical */
        if(lsrvtypelen == rsrvtypelen)
        {
            return strncasecmp(lsrvtype,rsrvtype,lsrvtypelen);
        }

        return 1;
    }

    colon = memchr(rsrvtype,':',rsrvtypelen);
    if(colon)
    {
        /* lsrvtype is abstract only and rsrvtype is concrete */
        if(lsrvtypelen == (colon - rsrvtype))
        {
            return strncasecmp(lsrvtype,rsrvtype,lsrvtypelen);
        }
        return 1;
    }

    /* lsrvtype and rsrvtype are  abstract only */
    if(lsrvtypelen == rsrvtypelen)
    {
        return strncasecmp(lsrvtype,rsrvtype,lsrvtypelen);
    }

    return 1;
}


/*=========================================================================*/
int SLPContainsStringList(int listlen, 
                          const char* list,
                          int stringlen,
                          const char* string) 
/* Checks a string-list for the occurence of a string                      */
/*                                                                         */
/* list -       pointer to the string-list to be checked                   */
/*                                                                         */
/* listlen -    length in bytes of the list to be checked                  */
/*                                                                         */
/* string -     pointer to a string to find in the string-list             */
/*                                                                         */
/* stringlen -  the length of the string in bytes                          */
/*                                                                         */
/* Returns -    zero if string is NOT contained in the list. non-zero if it*/
/*              is.                                                        */
/*=========================================================================*/
{
    char* listend = (char*)list + listlen;
    char* itembegin = (char*)list;
    char* itemend = itembegin;
    char ipv6Buffer[41]; /* must be at least 40 characters */

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
        if (strchr(itembegin, ':')) {
            SLPNetExpandIpv6Addr(itembegin, ipv6Buffer, sizeof(ipv6Buffer));
            if(SLPCompareString(itemend - itembegin,
                ipv6Buffer,
                stringlen,
                string) == 0) {
                    return 1;
                }
        }
        else {
            if(SLPCompareString(itemend - itembegin,
                            itembegin,
                            stringlen,
                            string) == 0)
            {
                return 1;
            }
        }
        itemend ++;    
    }

    return 0;
}


/*=========================================================================*/
int SLPIntersectStringList(int list1len,
                           const char* list1,
                           int list2len,
                           const char* list2)
/* Calculates the number of common entries between two string-lists        */
/*                                                                         */
/* list1 -      pointer to the string-list to be checked                   */
/*                                                                         */
/* list1len -   length in bytes of the list to be checked                  */
/*                                                                         */
/* list2 -      pointer to the string-list to be checked                   */
/*                                                                         */
/* list2len -   length in bytes of the list to be checked                  */
/*                                                                         */
/* Returns -    The number of common entries.                              */
/*=========================================================================*/
{
    int result = 0;
    char* listend = (char*)list1 + list1len;
    char* itembegin = (char*)list1;
    char* itemend = itembegin;

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

        if(SLPContainsStringList(list2len,
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

/*=========================================================================*/
int SLPUnionStringList(int list1len,
                       const char* list1,
                       int list2len,
                       const char* list2,
                       int* unionlistlen,
                       char * unionlist)
/* Generate a string list that is a union of two string lists              */
/*                                                                         */
/* list1len -   length in bytes of list1                                   */
/*                                                                         */
/* list1 -      pointer to a string-list                                   */
/*                                                                         */
/* list2len -   length in bytes of list2                                   */
/*                                                                         */
/* list2 -      pointer to a string-list                                   */
/*                                                                         */
/* unionlistlen - pointer to the size in bytes of the unionlist buffer.    */
/*                also receives the size in bytes of the unionlist buffer  */
/*                on successful return.                                    */
/*                                                                         */
/* unionlist -  pointer to the buffer that will receive the union list.    */ 
/*                                                                         */
/*                                                                         */
/* Returns -    Length of the resulting union list or negative if          */
/*              unionlist is not big enough. If negative is returned       */
/*              *unionlist will be changed indicate the size of unionlist  */
/*              buffer needed                                              */
/*                                                                         */
/* Important: In order ensure that unionlist does not contain any          */
/*            duplicates, at least list1 must not have any duplicates.     */
/*            Also, for speed optimization if list1 and list2 are both     */
/*            with out duplicates, the larger list should be passed in     */
/*            as list1.                                                    */
/*                                                                         */
/* Note: A good size for unionlist (so that non-zero will never be         */
/*       returned) is list1len + list2len + 1                              */
/*=========================================================================*/
{
    char* listend = (char*)list2 + list2len;
    char* itembegin = (char*)list2;
    char* itemend = itembegin;
    int   itemlen;
    int   copiedlen;

    if(unionlist == 0 ||
       *unionlistlen == 0 ||
       *unionlistlen < list1len)
    {
        *unionlistlen = list1len + list2len + 1;
        return -1;
    }

    /* Copy list1 into the unionlist since it should not have any duplicates */
    memcpy(unionlist,list1,list1len);
    copiedlen = list1len;

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

        itemlen = itemend - itembegin;
        if(SLPContainsStringList(list1len,
                                 list1,
                                 itemlen,
                                 itembegin) == 0)
        {
            if(copiedlen + itemlen + 1 > *unionlistlen)
            {

                *unionlistlen = list1len + list2len + 1;
                return -1;
            }

            /* append a comma if not the first entry*/
            if(copiedlen)
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

/*=========================================================================*/
int SLPSubsetStringList(int listlen,
                        const char* list,
                        int sublistlen,
                        const char* sublist)
/* Test if sublist is a set of list                                        */
/*                                                                         */
/* list  -      pointer to the string-list to be checked                   */
/*                                                                         */
/* listlen -    length in bytes of the list to be checked                  */
/*                                                                         */
/* sublistlistlen -   pointer to the string-list to be checked             */
/*                                                                         */
/* sublist -   length in bytes of the list to be checked                   */
/*                                                                         */
/* Returns -    non-zero is sublist is a subset of list.  Zero otherwise   */
/*=========================================================================*/
{
    /* count the items in sublist */
    int curpos;
    int sublistcount;

    if(sublistlen ==0 || listlen == 0)
    {
        return 0;
    }

    curpos = 0;
    sublistcount = 1;
    while(curpos < sublistlen)
    {
        if(sublist[curpos] == ',')
        {
            sublistcount ++;
        }
        curpos ++;
    }

    if(SLPIntersectStringList(listlen,
                              list,
                              sublistlen,
                              sublist) == sublistcount)
    {
        return sublistcount;
    }

    return 0;
}


/*=========================================================================*/
int SLPCheckServiceUrlSyntax(const char* srvurl,
			     int srvurllen)
/* Test if a service url conforms to accepted syntax
 *
 * srvurl -     (IN) service url string to check
 *
 * srvurllen -  (IN) length of srvurl in bytes
 *
 * Returns - zero if srvurl has acceptable syntax, non-zero on failure
 *
 *=========================================================================*/
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
 
 
/*=========================================================================*/
int SLPCheckAttributeListSyntax(const char* attrlist,
				int attrlistlen)
/* Test if a service url conforms to accepted syntax
 *
 * attrlist -     (IN) attribute list string to check
 *
 * attrlistlen -  (IN) length of attrlist in bytes
 *
 * Returns - zero if srvurl has acceptable syntax, non-zero on failure
 *
 *=========================================================================*/
{
    const char* slider;
    const char* end;
   
    if(attrlistlen)
    {
        slider = attrlist;
        end = attrlist + attrlistlen;
        while(slider != end)
        {
            if(*slider == '(')
            {
	        while(slider != end)
	        {
	            if(*slider == '=')
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
