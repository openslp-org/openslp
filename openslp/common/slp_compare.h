/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_string.h                                               */
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

#if(!defined SLP_STRING_H_INCLUDED)
#define SLP_STRING_H_INCLUDED


/*=========================================================================*/
int SLPCompareString(int str1len,                                          
                     const char* str1,
                     int str2len,
                     const char* str2);
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


/*=========================================================================*/
int SLPCompareNamingAuth(int srvtypelen,
			 const char* srvtype,
			 int namingauthlen,
			 const char* namingauth);
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

int SLPCompareSrvType(int srvtype1len,
                      const char* srvtype1,
                      int srvtype2len,
                      const char* srvtype2);
/* Compares two service type strings                                       */
/*                                                                         */
/* TODO: Handle the whole utf8 spec                                        */
/*                                                                         */
/* srvtype1 -       pointer to string to be compared                       */
/*                                                                         */
/* srvtype1len -    length of str1 in bytes                                */
/*                                                                         */
/* srvtype2 -       pointer to string to be compared                       */
/*                                                                         */
/* srvtype2len -    length of str2 in bytes                                */
/*                                                                         */
/* Returns -    zero if srvtypes are equal. >0 if str1 is greater than str2 */
/*              <0 if s1 is less than str2                                 */
/*=========================================================================*/


/*=========================================================================*/
int SLPContainsStringList(int listlen,
                          const char* list, 
                          int stringlen,
                          const char* string);
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


/*=========================================================================*/
int SLPIntersectStringList(int list1len,
                           const char* list1,
                           int list2len,
                           const char* list2);
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


/*=========================================================================*/
int SLPUnionStringList(int list1len,
                       const char* list1,
                       int list2len,
                       const char* list2,
                       int* unionlistlen,
                       char* unionlist);
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
/*                on successful return                                     */
/*                                                                         */
/* unionlist -  pointer to buffer that will receive the union list         */
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
#endif
