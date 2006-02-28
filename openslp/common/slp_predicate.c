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

/** Functions for predicate matching.
 *
 * This file also contains a few debug routines for dumping attribute lists
 * and filter trees to verify their formats.
 *
 * @file       slp_predicate.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodePred
 */

#include "slp_types.h"
#include "slp_predicate.h"
#include "slp_linkedlist.h"

#ifdef _WIN32
# define FNM_CASEFOLD 1
# define FNM_NOMATCH  1
/** Unix standard fnmatch routine.
 *
 * Performs a basic wildcard match between a pattern and a string.
 * This version doesn't do nearly what the GNU version does, but it's 
 * sufficient for our needs.
 *
 * @param[in] pattern - The pattern to match against.
 * @param[in] string - The string to match.
 * @param[in] flags - Control flags that change behaviour.
 *
 * @return Zero on success, or FNM_NOMATCH on mismatch.
 *
 * @internal
 */
static int fnmatch(const char * pattern, const char * string, int flags)
{
   while (*pattern && *string) 
   {
      switch (*pattern) 
      {
         case '*':
            for (pattern++; *string; string++)
               if (fnmatch(pattern, string, flags) == 0)
                  return 0;
            break;

         default:
            if (((*pattern ^ *string) 
                  & ~((flags & FNM_CASEFOLD)? 0x20: 0)) != 0)
               return FNM_NOMATCH;
            pattern++, string++;
      }
   }
   if (*string)
      return FNM_NOMATCH;
   while (*pattern)
      if (*pattern++ != '*')
         return FNM_NOMATCH;
   return 0;
}
#else
# include <fnmatch.h>
# ifndef FNM_CASEFOLD
#  define FNM_CASEFOLD 0   /*!< the fnmatch casefold flag - portability. */
# endif
#endif

/** More definitions for min and max - we should consolidate these ~~~jmc */
#define SLP_MIN(a, b) ((a) < (b) ? (a) : (b))
#define SLP_MAX(a, b) ((a) > (b) ? (a) : (b))

#ifdef DEBUG
/** Dump the attribute list to STDOUT.
 *
 * @param[in] level - The tab-indent level at which to print each string.
 * @param[in] attrs - The attribute list to be dumped.
 *
 * @remarks This routine is only used by debug code.
 *
 * @internal
 */
void dumpAttrList(int level, const SLPAttrList * attrs)
{
   int i;

   if (SLP_IS_EMPTY(attrs))
      return;

   for (i = 0; i <= level; i++)
      printf("\t");

   attrs = attrs->next;
   while (!SLP_IS_HEAD(attrs))
   {
      switch (attrs->type)
      {
         case string:
            printf("%s = %s (string) \n", attrs->name, attrs->val.stringVal);
            break;
         case integer:
            printf("%s = %lu (integer) \n", attrs->name, attrs->val.intVal);
            break;
         case boolean:
            printf("%s = %s (boolean) \n", attrs->name,
                  (attrs->val.boolVal ? " TRUE " : " FALSE "));
            break;
         case opaque:
         case head:
         default:
            printf("%s = %s\n", attrs->name,
                  "illegal or unknown attribute type");
            break;
      }
      attrs = attrs->next;
   }
   return;
}

/** Prints an LDAPv3 filter operator as a string.
 *
 * @param[in] op - The LDAPv3 filter operator code to be printed.
 *
 * @remarks This routine is only used by Debug code.
 *
 * @internal
 */
static void printOperator(int op)
{
   switch (op)
   {
      case ldap_or:
         printf(" OR ");
         break;
      case ldap_and:
         printf(" AND ");
         break;
      case ldap_not:
         printf(" NOT ");
         break;
      case expr_eq:
         printf(" EQUAL ");
         break;
      case expr_gt:
         printf(" GREATER THAN ");
         break;
      case expr_lt:
         printf(" LESS THAN ");
         break;
      case expr_present:
         printf(" PRESENT ");
         break;
      case expr_approx:
         printf(" APPROX ");
         break;
      case -1:
         printf(" list head ");
         break;
      default:
         printf(" unknown operator value %i ", op);
         break;
   }
   return;
}

/** Dumps the LDAPv3 filter tree to STDOUT.
 *
 * @param[in] filter - The filter to be dumped.
 *
 * @remarks This routine is only used by Debug code.
 *
 * @internal
 */
void dumpFilterTree(const SLPLDAPFilter * filter)
{
   int i; 

   for (i = 0; i < filter->nestingLevel; i++)
      printf("\t");

   printOperator(filter->operator);

   printf("%s (level %i) \n", (filter->logical_value ? " TRUE " : " FALSE "),
         filter->nestingLevel);

   dumpAttrList(filter->nestingLevel, &filter->attrs);

   if (!SLP_IS_EMPTY(&filter->children))
      dumpFilterTree((SLPLDAPFilter *) filter->children.next) ;

   if ((!SLP_IS_HEAD(filter->next)) && (!SLP_IS_EMPTY(filter->next)))
      dumpFilterTree(filter->next);

   return;
}
#endif   /* DEBUG */

/** Returns a boolean value based on a compare result and an operation.
 *
 * @param[in] compare_result - A value < 0, 0, or > 0.
 * @param[in] operation - The operation used to generate @p compare_result.
 *
 * @return A boolean value TRUE (1) or FALSE (0).
 *
 * @internal
 */
static int SLPEvaluateOperation(int compare_result, int operation)
{
   switch (operation)
   {
      case expr_eq:
         if (compare_result == 0)   /*  a == b */
            return 1;
         break;

      case expr_gt:
         if (compare_result >= 0)   /*  a >= b  */
            return 1;
         break;

      case expr_lt:
         /* a <= b  */
         if (compare_result <= 0)
            return 1;
         break;

      case expr_present:
      case expr_approx:
      default:
         return 1;
         break;
   }
   return 0;
}

/** Evaluates attribute values.
 *
 * @param[in] a - The first value to evaluate.
 * @param[in] b - The second value to evaluate.
 * @param[in] op - The LDAPv3 filter operation to use in the evaluation.
 *
 * @return A boolean false (0) or true (1) based on the comparison 
 *    results. If the compare operation is true, returns 1.
 *
 * @internal
 */
static int SLPEvaluateAttributes(const SLPAttrList * a, 
      const SLPAttrList * b, int op)
{
   /* first ensure they are the same type  */
   if (a->type == b->type)
      switch (a->type)
      {
         case string:
            return SLPEvaluateOperation(fnmatch(a->val.stringVal,
                  b->val.stringVal, FNM_CASEFOLD), op);

         case integer:
            return SLPEvaluateOperation(a->val.intVal - b->val.intVal, op);

         case tag:
            /* equivalent to a presence test  */
            return 1;

         case boolean:
            if ((a->val.boolVal != 0) && (b->val.boolVal != 0))
               return 1;
            if ((a->val.boolVal == 0) && (b->val.boolVal == 0))
               return 1;
            break;

         case opaque:
            if (!memcmp((((char *) (a->val.opaqueVal)) + 4),
                  (((char *)(b->val.opaqueVal)) + 4),
                  SLP_MIN((*((int *)a->val.opaqueVal)),
                        (*((int *)a->val.opaqueVal)))))
               ;
            return 1;

         default:
            break;
      }
   return 0;
}

/** Evaluates an entire LDAPv3 filter tree.
 *
 * @param[in,out] filter - The LDAPv3 filter tree to evaluate.
 * @param[in] attrs - The attribute list to apply @p filter to.
 *
 * @return A boolean false (0) or true (1) based on the comparison 
 *    results. If the compare operation is true, returns 1.
 *
 * @remarks On exit, @p filter has been updated with per-node evaluation
 *    values.
 *
 * @internal
 */
static int SLPEvaluateFilterTree(SLPLDAPFilter * filter, 
      const SLPAttrList * attrs)
{
   if (!SLP_IS_EMPTY(&filter->children))
      SLPEvaluateFilterTree((SLPLDAPFilter *) filter->children.next, attrs);

   if (!SLP_IS_HEAD(filter->next) && !SLP_IS_EMPTY(filter->next))
      SLPEvaluateFilterTree(filter->next, attrs);


   if (filter->operator == ldap_and
         || filter->operator == ldap_or
         || filter->operator == ldap_not)
   {
      /* evaluate ldap logical operators by evaluating filter->children as 
         *  a list of filters 
         */
      SLPLDAPFilter * child_list = (SLPLDAPFilter *) filter->children.next;

      /* initialize  the filter's logical value to true */
      if (filter->operator == ldap_or)
         filter->logical_value = 0;
      else
         filter->logical_value = 1;

      while (!SLP_IS_HEAD(child_list))
      {
         if (child_list->logical_value == 1)
         {
            if (filter->operator == ldap_or)
            {
               filter->logical_value = 1;
               break;
            }
            if (filter->operator == ldap_not)
            {
               filter->logical_value = 0;
               break;
            }
            /* for an & operator keep going  */
         }
         else
         {
            /* child is false */
            if (filter->operator == ldap_and)
            {
               filter->logical_value = 0;
               break;
            }
         }
         child_list = child_list->next;
      }
   }
   else
   {
      /* find the first matching attribute and set the logical value */
      filter->logical_value = 0;
      if (!SLP_IS_HEAD(filter->attrs.next))
      {
         attrs = attrs->next;
         while (!SLP_IS_HEAD(attrs)
               && fnmatch(filter->attrs.next->name, attrs->name, FNM_CASEFOLD)
                     == FNM_NOMATCH)
            attrs = attrs->next ;

         /* either we have traversed the list or found the first matching attribute */
         if (!SLP_IS_HEAD(attrs))
         {
            /* we found the first matching attribute, now do the comparison */
            if (filter->operator == expr_present
                  || filter->operator == expr_approx)
               filter->logical_value = 1;
            else
               filter->logical_value = SLPEvaluateAttributes(
                     filter->attrs.next, attrs, filter->operator);
         }
      }
   }
   return filter->logical_value;
}

/** Match attribute in a list to a specified filter.
 *
 * @param[in] attrlist - The attribute list to apply @p filter to.
 * @param[in] filter - A string representation of an LDAPv3 filter.
 *
 * @return A boolean false (0) or true (1) based on the comparison 
 *    results. If the compare operation is true, returns 1.
 */
int SLP_predicate_match(const SLPAttrList * attrlist, const char * filter)
{
   int ccode;
   SLPLDAPFilter * ftree;

   if (filter == 0 || !strlen(filter))
      return 1;   /* no predicate - aways tests TRUE */

   if ((ftree = SLPDecodeLDAPFilter(filter)) != 0)
   {
      ccode = SLPEvaluateFilterTree(ftree, attrlist);
      SLPFreeFilterTree(ftree);
      return ccode;
   }
   return 0;
}

/*=========================================================================*/
