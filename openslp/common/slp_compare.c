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
/* Copyright (c) 1995, 1999  Caldera Systems, Inc.                         */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify it */
/* under the terms of the GNU Lesser General Public License as published   */
/* by the Free Software Foundation; either version 2.1 of the License, or  */
/* (at your option) any later version.                                     */
/*                                                                         */
/*     This program is distributed in the hope that it will be useful,     */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       */
/*     GNU Lesser General Public License for more details.                 */
/*                                                                         */
/*     You should have received a copy of the GNU Lesser General Public    */
/*     License along with this program; see the file COPYING.  If not,     */
/*     please obtain a copy from http://www.gnu.org/copyleft/lesser.html   */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*     Please submit patches to http://www.openslp.org                     */
/*                                                                         */
/***************************************************************************/

#include <string.h>

#include <slp_compare.h>

#ifdef WIN32
#define strncasecmp(string1, string2, n) strnicmp(string1, string2, n)
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
        return str1[str2len-1];
    }
    
    return str2[str1len-1];
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
    
    if (namingauthlen == 0xffff) /* match all naming authorities */ 
	return 0;		
    
    dot = memchr(srvtype,'.',srvtypelen);

    if (!namingauthlen)		/* IANA naming authority */
	return dot ? 1 : 0;	

    srvtypenalen = srvtypelen - (dot + 1 - srvtype);

    if (srvtypenalen != namingauthlen)
	return 1;
    
    if (strncasecmp(dot + 1, namingauth, namingauthlen) == 0)
	return 0;
    
    return 1;
}


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

    while(itemend < listend)
    {
        itembegin = itemend;

        /* seek to the end of the next list item */
        while(1)
        {
            if(itemend == listend || *itemend == ',')
            {
                if (*(itemend - 1) != '\\')
                {
                    break;
                }
            }

            itemend ++;
        }

        if(SLPCompareString(itemend - itembegin,
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
                if (*(itemend - 1) != '\\')
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

