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

/** Header file for LDAPv3 search filter parser.
 *
 * @file       slpd_predicate.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com), Richard Morrell
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#ifndef SLPD_PREDICATE_H_INCLUDED
#define SLPD_PREDICATE_H_INCLUDED

/*!@defgroup SlpdCodePredicate LDAPv3 Search Filter */

/*!@addtogroup SlpdCodePredicate
 * @ingroup SlpdCode
 * @{
 */

#include "slp_types.h"
#include "slpd.h"

/** The maximum recursion depth for the attribute and predicate parsers.
 */
#define SLPD_ATTR_RECURSION_DEPTH         50
#define SLPD_PREDICATE_RECURSION_DEPTH    50

/* The character that is a wildcard. */
#define WILDCARD ('*')

/** Result codes for the predicate parser.
 */
typedef enum
{
   PREDICATE_PARSE_OK = 0,          /*!< Success. */
   PREDICATE_PARSE_ERROR,           /*!< Parse failure */
   PREDICATE_PARSE_INTERNAL_ERROR   /*!< Failure unrelated to the parse - usually memory allocation */
} SLPDPredicateParseResult;

/* Predicate tree node types */
typedef enum {
   NODE_AND,
   NODE_OR,
   NODE_NOT,
   EQUAL,
   APPROX,
   GREATER,
   LESS,
   PRESENT
} SLPDPredicateTreeNodeType;

#define Operation SLPDPredicateTreeNodeType

/* Predicate tree node */
typedef struct __SLPDPredicateTreeNode
{
   SLPDPredicateTreeNodeType nodeType;
   struct __SLPDPredicateTreeNode *next;     /* next node in a combination */
   union {
      struct __SLPDPredicateLogicalBody
      {
         struct __SLPDPredicateTreeNode *first;
      } logical;
      struct __SLPDPredicateComparisonBody
      {
         size_t tag_len;
         char *tag_str;
         size_t value_len;
         char *value_str;
         char storage[2];
      } comparison;
   } nodeBody;
} SLPDPredicateTreeNode;

int SLPDPredicateTest(int version, size_t attrlistlen, 
      const char * attrlist, size_t predicatelen, 
      const char * predicate);

int SLPDFilterAttributes(size_t attrlistlen, const char * attrlist, 
      size_t taglistlen, const char * taglist, size_t * resultlen, 
      char ** result);

#if defined (ENABLE_SLPv1)
SLPDPredicateParseResult createPredicateParseTreev1(
   const char * start, const char ** end, SLPDPredicateTreeNode * *ppNode, int recursion_depth);
#endif
SLPDPredicateParseResult createPredicateParseTree(
   const char * start, const char ** end, SLPDPredicateTreeNode * *ppNode, int recursion_depth);

void freePredicateParseTree(SLPDPredicateTreeNode *pNode);

int SLPDPredicateTestTree(SLPDPredicateTreeNode *parseTree, 
      SLPAttributes slp_attr);

/*! @} */

#endif   /* SLPD_PREDICATE_H_INCLUDED */

/*=========================================================================*/
