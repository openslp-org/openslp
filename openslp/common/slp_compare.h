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
                       char * unionlist); 
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


/*=========================================================================*/
int SLPSubsetStringList(int listlen,
                        const char* list,
                        int sublistlen,
                        const char* sublist);
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

#endif
