/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slp_utf8.h                                                 */
/*                                                                         */
/* Abstract:    Do conversions between UTF-8 and other character encodings */
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

#ifndef SLP_UTF8_INCLUDED
#define SLP_UTF8_INCLUDED

#ifndef INT_MAX
#include <limits.h>
#endif

/*=========================================================================*/
int SLPv1ToEncoding(char *string, int *len, int encoding, 
                    const char *utfstring, int utflen); 
/* Converts a UTF-8 character string to a SLPv1 encoded string.            */
/* When called with string set to null returns number of bytes needed      */
/* in string.                                                              */
/*                                                                         */
/* string - (OUT) SLPv1 encoded string.                                    */
/*                                                                         */
/* len    - (INOUT) IN - bytes available in string                         */
/*                  OUT - bytes used up in string                          */
/*                                                                         */
/* encoding  - (IN) encoding of the string passed in                       */
/*                                                                         */
/* utfstring - (IN) pointer to UTF-8 string                                */
/*                                                                         */
/* utflen    - (IN) length of UTF-8 string                                 */
/*                                                                         */
/* Returns  - Zero on success, SLP_ERROR_PARSE_ERROR, or                   */
/*            SLP_ERROR_INTERNAL_ERROR if out of memory.  string and len   */
/*            invalid if return is not successful.                         */
/*                                                                         */
/*=========================================================================*/


/*=========================================================================*/
int SLPv1AsUTF8(int encoding, char *string, int *len);
/* Converts a SLPv1 encoded string to a UTF-8 character string in          */
/* place. If string does not have enough space to hold the encoded string  */
/* we are dead.                                                            */
/*                                                                         */
/* encoding - (IN) unicode encoding of the string passed in                */
/*                                                                         */
/* string   - (INOUT) IN - pointer to SLPv1 encoded string                 */
/*                    OUT - pointer to converted UTF-8 string.             */
/*                                                                         */
/* len      - (INOUT) IN - length of SLPv1 encoded string (in bytes)       */
/*                    OUT - length of UTF-8 string (in bytes)              */
/*                                                                         */
/* Returns  - Zero on success, SLP_ERROR_PARSE_ERROR, or                   */
/*            SLP_ERROR_INTERNAL_ERROR if out of memory.  string and len   */
/*            invalid if return is not successful.                         */
/*                                                                         */
/*=========================================================================*/

#endif
