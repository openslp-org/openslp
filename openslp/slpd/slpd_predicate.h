
/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_predicate.h                                           */
/*                                                                         */
/* Abstract:    This files contains an implementation of LDAPv3 search     */
/*              filters for SLP (as specified in RFC 2254).                */
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

#ifndef SLPD_PREDICATE_H_INCLUDED
#define SLPD_PREDICATE_H_INCLUDED

#define SLPD_ATTR_RECURSION_DEPTH   50   /* max recursion depth for attr   */
                                         /* parser                         */                                        
                                        
/*=========================================================================*/
int SLPDPredicateTest(int version,
                      int attrlistlen,
                      const char* attrlist,
                      int predicatelen,
                      const char* predicate);
/* Determine whether the specified attribute list satisfies                */
/* the specified predicate                                                 */
/*                                                                         */
/* version    (IN) SLP version of the predicate string (should always be   */
/*                 2 since we don't handle SLPv1 predicates yet)           */
/*                                                                         */
/* attrlistlen  (IN) length of attrlist                                    */
/*                                                                         */
/* attr         (IN) attribute list to test                                */
/*                                                                         */
/* predicatelen (IN) length of the predicate string                        */
/*                                                                         */
/* predicate    (IN) the predicate string                                  */
/*                                                                         */
/* Returns: Boolean value.  Zero of test fails.  Non-zero if test fails    */
/*          or if there is a parse error in the predicate string           */
/*=========================================================================*/



/*=========================================================================*/
int SLPDFilterAttributes(int attrlistlen,
                         const char* attrlist,
                         int taglistlen,
                         const char* taglist,
                         int* resultlen,
                         char** result);
/* Copies attributes from the specified attribute list to a result string  */
/* according to the taglist as described by section 10.4. of RFC 2608      */
/*                                                                         */
/* version    (IN) SLP version of the predicate string (should always be   */
/*                 2 since we don't handle SLPv1 predicates yet)           */
/*                                                                         */
/* attrlistlen  (IN) length of attrlist                                    */
/*                                                                         */
/* attr         (IN) attribute list to test                                */
/*                                                                         */
/* predicatelen (IN) length of the predicate string                        */
/*                                                                         */
/* predicate    (IN) the predicate string                                  */
/*                                                                         */
/* Returns: Zero on success.  Nonzero on failure                           */
/*=========================================================================*/
#endif 
