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
/* Author(s):   Matthew Peterson                                           */
/*                                                                         */
/***************************************************************************/

#if(!defined SLP_STRING_H_INCLUDED)
#define SLP_STRING_H_INCLUDED


/*=========================================================================*/
int SLPStringCompare(int str1len,                                          
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
int SLPSrvTypeCompare(int srvtype1len,
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
int SLPStringListContains(int listlen,
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
int SLPStringListIntersect(int list1len,
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

#endif
