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

/** LDAPv3 search filter implementation.
 *
 * This files contains an implementation of LDAPv3 search filters for SLP 
 * as specified in RFC 2254.
 *
 * @file       slpd_predicate.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 *
 * @todo The current implementation reparses the predicate string every time 
 * it is evaluated. This implementation should be refactored to parse the 
 * predicate string once, build some sort of data structure out of that, 
 * and then compare the predicate DS with the attribute DS.
 *
 * @par Assumptions
 * @li If a tag specified in the query string does not exist, that's a FALSE
 * @li The "Undefined" value mentioned in the LDAPv3 RFC (2251) is synonymous 
 *     with "false".
 * @li The "Approx" operator matches as an equal operator. 
 * @li Variable matching on type is strict: ie, the right-hand-side of an 
 *     operator must evaluate to the same type as the tag referenced on the 
 *     left-hand-side. 
 *
 * @bug If trash follows the last operand to a binary argument, it will
 * be ignored if the operand is not evaluated due to short circuiting: 
 * ie, in "(&(first=*)(second<=3)trash)", "trash" will be silently 
 * ignored _if_ "(first=*)" is true.
 *
 * @bug Escaped '*' characters are treated as wildcards in the string 
 * equality operator: ie, in "(string=abc\2axyz)" will match the literal 
 * string "abc\2axyz" instead of "abc*xyz". 
 *
 * @bug No operations can be performed on opaque data types. 
 */

#include "slp_types.h"
#include "slp_debug.h"

#include "slpd_predicate.h"

#include "../libslpattr/libslpattr.h"
#include "../libslpattr/libslpattr_internal.h"

/* The character that is a wildcard. */
#define WILDCARD ('*')
#define BRACKET_OPEN '('
#define BRACKET_CLOSE ')'

#ifndef MIN
# define MIN(x,y) (x < y ? x : y)
#endif

/************************* <Lifted from slp_attr.c> ***********************/

/** Tests a character to see if it reserved (as defined in RFC 2608, p11). 
 */
#define IS_RESERVED(x) (((x) == '(' || (x) == ')' || (x) == ','   \
      || (x) == '\\' || (x) == '!' || (x) == '<' || (x) == '='    \
      || (x) == '>' || (x) == '~')                                \
      || ((((char)0x01 <= (x)) && ((char)0x1F >= (x)))            \
      || ((x) == (char)0x7F)))

/** Alias for IS_RESERVED.
 */
#define IS_INVALID_VALUE_CHAR(x) IS_RESERVED(x)

/** Tests a character to see if it's an invalid tag character.
 */
#define IS_INVALID_TAG_CHAR(x) (IS_RESERVED(x) || ((x) == '*')    \
      || ((x) == (char)0x0D) || ((x) == (char)0x0A)               \
      || ((x) == (char)0x09) || ((x) == '_'))

/** Tests a character to see if it's a valid tag character.
 */
#define IS_VALID_TAG_CHAR(x) (!IS_INVALID_TAG_CHAR(x))

/** Does a case insensitive substring match for needle in haystack.
 *
 * @param[in] haystack - The memory buffer to search in.
 * @param[in] needle - The string to search for.
 * @param[in] needle_len - The length of @p needle in bytes.
 *
 * @return A pointer to the start of the substring. NULL if the substring 
 *    is not found.
 *
 * @bug This implementation of substr isn't exactly blazingly fast...
 *
 * @internal
 */
/* static */ const char * substr(const char * haystack, const char * needle, 
      size_t needle_len)
{
   const char * hs_cur; 

   for (hs_cur = haystack; *hs_cur != 0; hs_cur++)
      if (strncasecmp(hs_cur, needle, needle_len) == 0)
         return hs_cur;

   return 0;
}

/** Result codes for the filter routines.
 */
typedef enum
{
   FR_UNSET,                  /*!< Placeholder. Used to detect errors. */
   FR_INTERNAL_SYSTEM_ERROR,  /*!< Internal error. */
   FR_PARSE_ERROR,            /*!< Parse error detected. */
   FR_MEMORY_ALLOC_FAILED,    /*!< Ran out of memory. */
   FR_EVAL_TRUE,              /*!< Expression successfully evaluated to true. */
   FR_EVAL_FALSE,             /*!< Expression successfully evaluated to false. */
} FilterResult;

/** Verifies and finds the length of an escaped string.
 *
 * @param[in] escaped - The string of escaped characters.
 * @param[in] len - The length of @p escaped in bytes.
 * @param[out] punescaped_len - The address of storage to return the 
 *    unescaped length of @p escaped.
 *
 * @return A boolean value; true if valid string, false if invalid. 
 *    If false, then @p punescaped_count will not be set.
 *
 * @internal
 */
static int escaped_verify(char * escaped, size_t len, size_t * punescaped_len) 
{
   size_t i;
   size_t unescaped_len;
   int seq_pos; /* Position in the current escape sequence. Set to zero when not in escape seq. */

   seq_pos = 0;

   for (i = unescaped_len = 0; i < len; i++)
   {
      /* Verify escape sequences. */
      if (seq_pos == 1 || seq_pos == 2)
      {
         if (!isxdigit((int) escaped[i]))
            return 0;

         if (seq_pos == 2)
            seq_pos = 0;
         else
            seq_pos++;
      }
      else
         unescaped_len++;


      /* Check for escape sequences. */
      if (escaped[i] == '\\')
         seq_pos = 1;
   }

   if (punescaped_len)
      *punescaped_len = unescaped_len;

   return 1;
}

/** Unescape the character represented by the two given hex values.
 *
 * @param[in] d1 - The first hexadecimal character.
 * @param[in] d2 - The second hexadecimal character.
 * @param[out] val - The address of storage for returning the value of 
 *    the character.
 *
 * @return A boolean value; true on successful conversion, false if one or 
 *    more of the hex digits are invalid.
 *
 * @internal
 */
static int unescape_check(char d1, char d2, char * val) 
{
   if (!isxdigit((int) d1) || !isxdigit((int) d2))
      return 0;

   if ((d1 >= 'A') && (d1 <= 'F'))
      d1 = d1 - 'A' + 0x0A;
   else
      d1 = d1 - '0';

   if ((d2 >= 'A') && (d2 <= 'F'))
      d2 = d2 - 'A' + 0x0A;
   else
      d2 = d2 - '0';

   *val = d2 + (d1 * 0x10);
   return 1;
}

/** Compares two strings that (potentially) contain escaped characters.
 *
 * @param[in] escaped - The escaped string to compare.
 * @param[in] escaped_len - The length of @p escaped in bytes 
 *    (including esc chars).
 * @param[in] verbatim - The string to compare against (not escaped).
 * @param[in] verbatim_len - The length of @p verbatim in bytes.
 * @param[in] strict_len - If TRUE, the number of characters in @p verbatim 
 *    that match must be _exactly_ @p verbatim_len. If FALSE, at most 
 *    @p verbatim_len characters may match.
 * @param[out] punescaped_count - The number of unescaped characters. 
 *    Optional; can be NULL.
 *
 * @return A filter result object.
 *
 * @internal
 */
static FilterResult unescape_cmp(const char * escaped, size_t escaped_len,
      const char * verbatim, size_t verbatim_len, SLPBoolean strict_len,
      size_t * punescaped_count)
{
   char unesc; /* Holder for unescaped characters. */

   unsigned esc_i; /* Index into escaped. */
   unsigned ver_i; /* Index into verbatim. */

   int unescaped_count;

   unescaped_count = esc_i = ver_i = 0;

   /***** Compare every pair of characters. *****/
   while (1)
   {
      /**** Check for string end. ****/
      if (esc_i >= escaped_len)
      {
         if (punescaped_count != NULL)
            *punescaped_count = unescaped_count;

         if (strict_len == SLP_TRUE)
         {
            if (ver_i >= verbatim_len)
               return FR_EVAL_TRUE;
            else
               return FR_EVAL_FALSE;
         }

         return FR_EVAL_TRUE;
      }

      /**** Check for escaping ****/
      if (escaped[esc_i] == '\\')
      {
         /*** Check for truncated escape values. ***/
         if (esc_i + 2 >= escaped_len)
            return FR_PARSE_ERROR;
         /*** Unescape. ***/
         if (!unescape_check(escaped[esc_i + 1], escaped[esc_i + 2], &unesc))
            return FR_PARSE_ERROR;

         /*** Increment. ***/
         esc_i += 2;
      }
      else
         unesc = escaped[esc_i];

      if (unesc != verbatim[ver_i])        /* quick check for equality*/
      {
         if (!isascii(unesc)         /* case insensitive check */
               || !isalpha(unesc)
               || !isalpha(verbatim[ver_i])
               || tolower(unesc) != tolower(verbatim[ver_i]))
            return FR_EVAL_FALSE;
      }

      unescaped_count++;
      esc_i++;
      ver_i++;
   }
}

/** Locate an escaped sub-string.
 *
 * @param[in] haystack - The unescaped buffer to search.
 * @param[in] haystack_len - The length of @p haystack in bytes.
 * @param[in] needle - The escaped string to search for.
 * @param[in] needle_len - The length of @p needle.
 * @param[out] punescaped_len - The size of the unescaped needle. 
 *    Invalid if NULL is returned.
 *
 * @return A pointer to the sub-string if found, or NULL if not found.
 *
 * @internal
 */
static void * my_memmem(char * haystack, size_t haystack_len, char * needle,
      size_t needle_len, size_t * punescaped_len) 
{
   unsigned offset;
   size_t search_len;

   if (escaped_verify(haystack, haystack_len, &search_len) == 0)
      return 0;

   for (offset = 0; offset <= search_len; offset++)
   {
      FilterResult err;

      err = unescape_cmp(needle, needle_len, haystack + offset,
            haystack_len - offset, SLP_FALSE, punescaped_len);
      if (err == FR_EVAL_TRUE)
         return(void *) (haystack + offset);
      else if (err != FR_EVAL_FALSE)
         return 0;
   }

   return 0;
}

/** Internal wildcard pattern match.
 *
 * @param[in] pattern - The pattern to evaluate.
 * @param[in] pattern_len - The length of @p pattern in bytes.
 * @param[in] str - The string to test the pattern on.
 * @param[in] str_len - The length of @p str.
 *
 * @return -1 error, 0 success, 1 failure.
 *
 * @internal
 */
static FilterResult wildcard_wc_str(char * pattern, size_t pattern_len, 
      char * str, size_t str_len)
{
   char * text_start; /* Pointer to the text following the WC(s) */
   size_t text_len; /* Length of the text. */
   char * found; /* The start of a string found in a search. */

   char * rem_start; /* Start of the remaining characters being searched for. */
   size_t rem_len; /* Length of the remaining charcters being searched for. */

   if (pattern_len == 0 && str_len == 0)
      return FR_EVAL_TRUE;

   if (pattern_len == 0)
      return FR_EVAL_FALSE;

   /***** Find text following wildcard *****/
   /**** Find end of WCs ****/
   text_start = pattern;
   text_len = 0;
   while (*text_start == WILDCARD)
   {
      text_start++;
      if (text_start == pattern + pattern_len)
      {
         /* No following text. Therefore we match. */
         return FR_EVAL_TRUE;
      }
   }

   /**** Find end of text. ****/
   found = memchr(text_start, WILDCARD, pattern_len - text_len);
   if (found == NULL)
   {
      /* No trailing WC. Set to end. */
      found = pattern + pattern_len;
   }
   text_len = found - text_start;

   /***** Look for the text in the source string *****/
   rem_start = str;
   rem_len = str_len;
   while (1)
   {
      int result;
      size_t match_len;

      found = my_memmem(rem_start, rem_len, text_start, text_len, &match_len);
      SLP_ASSERT(found == NULL
            || ((found >= rem_start) && (found <= rem_start + rem_len)));

      if (found == NULL)
      {
         /* Desired text is not in the string. */
         return FR_EVAL_FALSE;
      }

      rem_len = str_len - (found - str);
      rem_start = found + 1;

      /**** Make recursive call. ****/
      result = wildcard_wc_str(text_start + text_len,
                     (pattern + pattern_len) - (text_start + text_len),
                     found + text_len, rem_len - match_len);

      if (result != FR_EVAL_FALSE)
         return result;
   }
   /*NOTREACHED*/
   SLP_ASSERT(0);
}

/** Check a pattern against a string.
 *
 * Public interface to wildcard functionality.
 *
 * @param[in] pattern - The pattern to evaluate.
 * @param[in] pattern_len - The length of @p pattern in bytes.
 * @param[in] str - The string to test the pattern on.
 * @param[in] str_len - The length of @p str in bytes.
 *
 * @return -1 error, 0 success, 1 failure
 *
 * @remarks This is the interface to wildcard finding. It actually just 
 *    prepares for repeated calls to wildcard_wc_str. Note that this routine
 *    must be marked non-static so that it can be found by the predicate 
 *    test.
 *
 * @internal
 */
/* static */ FilterResult wildcard(const char * pattern, size_t pattern_len, 
      const char * str, size_t str_len)
{
   char * first_wc; /* Position of the first WC in the pattern. */
   size_t first_wc_len; /* Position of first_wc in pattern. */
   size_t unescaped_first_wc_len; /* Position of first_wc in unescaped version of pattern. */

   /***** Check text up to first WC *****/
   /**** Get text ****/
   first_wc = memchr(pattern, WILDCARD, pattern_len);
   if (first_wc == NULL)
   {
      /* No wc */
      return unescape_cmp(pattern, pattern_len, str, str_len, SLP_TRUE, NULL);
   }
   else
      first_wc_len = first_wc - pattern;

   /**** Compare. ****/
   unescaped_first_wc_len = first_wc_len;
   if (first_wc_len > 0)
   {
      FilterResult err;
      err = unescape_cmp(pattern, first_wc_len, str, str_len, SLP_FALSE,
                  &unescaped_first_wc_len);
      if (err != FR_EVAL_TRUE)
      {
         /* The leading text is different. */
         return err;
      }
   }

   /***** Start recursive call. *****/
   return wildcard_wc_str(first_wc, pattern_len - first_wc_len,
               (char *) str + unescaped_first_wc_len,
               str_len - unescaped_first_wc_len);
}

/** Tests a string to see if it is a boolean.
 *
 * @param[in] str - The string to be tested.
 * @param[in] str_len - The length of @p str in bytes.
 * @param[in] val - A boolean result; SLP_TRUE or SLP_FALSE.
 *
 * @return A boolean value; true on success, false on failure.
 *
 * @internal
 */
static int is_bool_string(const char * str, size_t str_len, SLPBoolean * val)
{
   if (str_len == 4 && memcmp(str, "true", 4) == 0) /* 4 -> length of string "true" */
   {
      *val = SLP_TRUE;
      return 1;
   }
   if (str_len == 5 && memcmp(str, "false", 5) == 0) /* 5 -> len of "false" */
   {
      *val = SLP_FALSE;
      return 1;
   }
   return 0;
}

/** Comparator operators.
 */
typedef enum
{
   EQUAL,
   APPROX,
   GREATER,
   LESS,
   PRESENT
} Operation;

/** Perform an integer operation.
 *
 * @param[in] slp_attr - 
 * @param[in] tag - 
 * @param[in] tag_len - The length of @p tag in bytes.
 * @param[in] rhs - 
 * @param[in] op - 
 *
 * @return A filter result object.
 *
 * @internal
 */
static FilterResult int_op(SLPAttributes slp_attr, char * tag, 
      size_t tag_len, char * rhs, Operation op)
{
   int rhs_val; /* The converted value of rhs. */
   char * end; /* Pointer to the end of op. */
   var_t * var; /* Pointer to the variable named by tag. */
   value_t * value; /* A value in var. */

   FilterResult result; /* Value to return. */

   SLP_ASSERT(op != PRESENT);

   result = FR_UNSET; /* For verification. */ /* TODO Only do this in debug. */

   /***** Verify and convert rhs. *****/
   rhs_val = strtol(rhs, &end, 10);

   if (*end != 0 && *end != BRACKET_CLOSE)
   {
      /* Trying to compare an int with a non-int. */
      return FR_EVAL_FALSE;
   }

   /***** Get variable. *****/
   var = attr_val_find_str((struct xx_SLPAttributes *) slp_attr, tag, tag_len);
   if (var == NULL)
      return FR_EVAL_FALSE;
   /**** Check type. ****/
   if (var->type != SLP_INTEGER)
      return FR_EVAL_FALSE;


   /***** Compare. *****/
   value = var->list;
   SLP_ASSERT(value != NULL);
   SLP_ASSERT(op != PRESENT); 
   switch (op)
   {
      case(EQUAL):
         result = FR_EVAL_FALSE;
         for (; value; value = value->next)
         {
            if (value->data.va_int == rhs_val)
            {
               result = FR_EVAL_TRUE;
               break;
            }
         }
         break;
      case(APPROX):
         SLP_ASSERT(0); /* TODO: Figure out how this works later. */
         result = FR_EVAL_FALSE; 
         break;
      case(GREATER):
         result = FR_EVAL_FALSE;
         for (; value; value = value->next)
         {
            if (value->data.va_int >= rhs_val)
            {
               result = FR_EVAL_TRUE;
               break;
            }
         }
         break;
      case(LESS):
         result = FR_EVAL_FALSE;
         for (; value; value = value->next)
         {
            if (value->data.va_int <= rhs_val)
            {
               result = FR_EVAL_TRUE;
               break;
            }
         }
         break;
      default:
         SLP_ASSERT(0);
   }

   SLP_ASSERT(result != FR_UNSET);
   return result;
}

/** Perform a keyword operation.
 *
 * @param[in] slp_attr - 
 * @param[in] tag - 
 * @param[in] rhs - 
 * @param[in] op - 
 *
 * @return A filter result object.
 *
 * @internal
 */
static FilterResult keyw_op(SLPAttributes slp_attr, char * tag, char * rhs,
      Operation op)
{
   /* Note that the only test keywords are allowed to undergo is PRESENT, 
    * also note that PRESENT is handled by our calling function.
    *
    * We are therefore quite justified in saying:
    */
   (void)slp_attr;
   (void)tag;
   (void)rhs;
	(void)op;

   SLP_ASSERT(op != PRESENT);
   return FR_EVAL_FALSE;
}

/** Perform a boolean operation.
 *
 * @param[in] slp_attr - 
 * @param[in] tag - 
 * @param[in] tag_len - The length of @p tag in bytes.
 * @param[in] rhs - 
 * @param[in] rhs_len - The length of @p rhs in bytes.
 * @param[in] op - 
 *
 * @return A filter result object.
 *
 * @internal
 */
static FilterResult bool_op(SLPAttributes slp_attr, char * tag, 
      size_t tag_len, char * rhs, size_t rhs_len, Operation op)
{
   SLPBoolean rhs_val; /* The value of the rhs. */
   var_t * var;

   FilterResult result;

   SLP_ASSERT(op != PRESENT);

   result = FR_UNSET;

   /* Note that the only valid non-PRESENT operator is EQUAL. */
   if (op != EQUAL)
      return FR_EVAL_FALSE;

   /***** Get and check the rhs value. *****/
   if (!is_bool_string(rhs, rhs_len, &rhs_val))
      return FR_EVAL_FALSE;

   /***** Get the tag value. *****/
   var = attr_val_find_str((struct xx_SLPAttributes *) slp_attr, tag, tag_len);
   if (var == NULL)
      return FR_EVAL_FALSE;
   /**** Check type. ****/
   if (var->type != SLP_BOOLEAN)
      return FR_EVAL_FALSE;

   /***** Compare. *****/
   if (var->list->data.va_bool == rhs_val)
      result = FR_EVAL_TRUE;
   else
      result = FR_EVAL_FALSE;

   SLP_ASSERT(result != FR_UNSET);
   return result;
}

/** Perform a string operation.
 *
 * @param[in] slp_attr - 
 * @param[in] tag - 
 * @param[in] tag_len - The length of @p tag in bytes.
 * @param[in] rhs - 
 * @param[in] rhs_len - The length of @p rhs in bytes.
 * @param[in] op - 
 *
 * @return A filter result object.
 *
 * @internal
 */
static FilterResult str_op(SLPAttributes slp_attr, char * tag, 
      size_t tag_len, char * rhs, size_t rhs_len, Operation op)
{
   char * str_val; /* Converted value of rhs. */
   size_t str_len; /* Length of converted value. */

   var_t * var;

   SLP_ASSERT(op != PRESENT);

   /***** Verify rhs. *****/
   str_val = rhs;
   str_len = rhs_len;

   /***** Get tag value. *****/
   var = attr_val_find_str((struct xx_SLPAttributes *) slp_attr, tag, tag_len);
   if (var == NULL)
      return FR_EVAL_FALSE;
   /**** Check type. ****/
   if (var->type != SLP_STRING)
      return FR_EVAL_FALSE;


   /***** Compare. *****/
   SLP_ASSERT(op != PRESENT); 

   if (op == APPROX)
   {
      SLP_ASSERT(0); /* TODO: Figure out how this works later. */
   }
   else if (op == EQUAL)
   {
      int result;
      value_t * value;

      for (value = var->list; value; value = value->next)
      {
         result = wildcard(str_val, str_len, value->data.va_str,
                        value->unescaped_len);
         /* We only keep going if the test fails. Let caller handle other problems. */
         if (result != FR_EVAL_FALSE)
            return result;
      }
   }
   else
   {
      value_t * value;
      /* We know that the op must be comparative. */
      SLP_ASSERT(op == LESS || op == GREATER);

      for (value = var->list; value; value = value->next)
      {
         int result;

         result = memcmp(value->data.va_str, str_val,
                        MIN(str_len, value->unescaped_len));
         if ((result <= 0 && op == LESS) || (result >= 0 && op == GREATER))
            return FR_EVAL_TRUE;
      }
   }

   return FR_EVAL_FALSE;
}

/** Perform an opaque operation.
 *
 * @param[in] slp_attr - 
 * @param[in] tag - 
 * @param[in] tag_len - The length of @p tag in bytes.
 * @param[in] rhs - 
 * @param[in] rhs_len - The length of @p rhs in bytes.
 * @param[in] op - 
 *
 * @return A filter result object.
 *
 * @internal
 */
/* static */ FilterResult opaque_op(SLPAttributes slp_attr, char * tag, 
      size_t tag_len, char * rhs, size_t rhs_len, Operation op)
{
   char * str_val; /* Converted value of rhs. */
   size_t str_len; /* Length of converted value. */

   var_t * var;

   SLP_ASSERT(op != PRESENT);

   /***** Verify and convert rhs. *****/
   str_val = rhs;
   str_len = rhs_len;

   /***** Get tag value. *****/
   var = attr_val_find_str((struct xx_SLPAttributes *) slp_attr, tag, tag_len);
   if (var == NULL)
      return FR_EVAL_FALSE;
   /**** Check type. ****/
   if (var->type != SLP_OPAQUE)
      return FR_EVAL_FALSE;


   /***** Compare. *****/
   SLP_ASSERT(op != PRESENT); 

   if (op == APPROX)
   {
      SLP_ASSERT(0); /* TODO: Figure out how this works later. */
   }
   else if (op == EQUAL)
   {
      int result;
      value_t * value;

      for (value = var->list; value; value = value->next)
      {
         result = wildcard(str_val, str_len, value->data.va_str,
                        value->unescaped_len);
         /* We only keep going if the test fails. Let caller handle other problems. */
         if (result != FR_EVAL_FALSE)
            return result;
      }
   }
   else
   {
      value_t * value;
      /* We know that the op must be comparative. */
      SLP_ASSERT(op == LESS || op == GREATER);

      for (value = var->list; value; value = value->next)
      {
         int result;

         result = memcmp(value->data.va_str, str_val,
                        MIN(str_len, value->unescaped_len));
         if ((result <= 0 && op == LESS) || (result >= 0 && op == GREATER))
            return FR_EVAL_TRUE;
      }
   }

   return FR_EVAL_FALSE;
}

/** Tests a string to see if it consists wholly of numeric characters.
 *
 * @param[in] str - The string to test.
 *
 * @return A boolean value; true if string is an int, false if not.
 *
 * @internal
 */
/* static */ int is_int_string(const char * str)
{
   int i;

   for (i = 0; str[i] != 0; i++)
      if (!((!isdigit((int) str[i])) || str[i] == '-'))
         return 0;

   return 1;
}

/** Locate a sub-string in a string.
 *
 * @param[in] src - The string to search for @p to_find.
 * @param[in] src_len - The length in bytes of @p src.
 * @param[in] to_find - The sub-string to locate.
 *
 * @return A pointer to start of substring, or null.
 *
 * @internal
 */
/* static */ char * find_substr(char * src, size_t src_len, char * to_find)
{
   unsigned i;

   for (i = 0; i < src_len; i++)
   {
      if (src[i] == *to_find)
      {
         int old_i = i; /* Save the old start. */
         int find_index;
         for (find_index = 0;
               (to_find[find_index] != '\0')
               && (i < src_len)
               && (to_find[find_index] == src[i]);
               to_find++, i++)
         {
         }

         if (to_find[find_index] == '\0')
            return src + i;

         i = old_i; /* Restor the old start. */
      }
   }
   return 0;
}

/** Finds a bracket matched to this one. Returns 0 if there isn't one.
 *
 * @param[in] str - The string in which to look for the bracket.
 *
 * @return A pointer to the bracket character, or NULL if none is found.
 *
 * @internal
 */
static char * find_bracket_end(const char * str)
{
   int open_count; 
   const char * cur;

   if (*str != BRACKET_OPEN)
      return 0;

   open_count = 0;

   /***** Count brackets. *****/
   for (cur = str; *cur != '\0'; cur++)
   {
      SLP_ASSERT(open_count >= 0);

      /**** Check current character. ****/
      if (*cur == BRACKET_OPEN)
         open_count++;
      else if (*cur == BRACKET_CLOSE)
         open_count--;

      /**** Check if we've found bracket end. ****/
      if (open_count == 0)
         return(char *) cur;
   }

   return 0;
}       

/** Represents a string and its length.
 */
struct pair
{
   char * start;
   char * end;    /*!< Stop reading _BEFORE_ end. ie, at end-1. */
};

/*--------------------------------------------------------------------------*/

#if defined(ENABLE_SLPv1)
/** Generate a filtered list of attributes for SLPv1.
 *
 * @param[in] start - The start of the string to work in.
 * @param[out] end - The end of the string processed. After successful 
 *    processing (ie, no PARSE_ERRORs), this will be pointed at the 
 *    character following the last char in this level of the expression.
 * @param[in] slp_attr - The attributes handle to compare on.
 * @param[out] return_value - The result of the search. Only valid if 
 *    SLP_OK was returned.
 *
 * @return SLP_OK on successful search (ie, the search was do-able).
 *    SLP_PARSE_ERROR on search error. The end of the expression is
 *    returned through end.
 *
 * @note This attempt only implements one comparision operator (==).
 *
 * @internal
 */
static FilterResult filterv1(const char * start, const char ** end,
      SLPAttributes slp_attr, int recursion_depth)
{
   char * operator; /* Pointer to the operator substring. */
   const char * cur; /* Current working character. */
   const char * last_char; /* The last character in the working string. */
   FilterResult err = FR_UNSET; /* The result of an evaluation. */
   FilterResult stop_condition = FR_UNSET;

   if (recursion_depth <= 0)
      return FR_PARSE_ERROR;

   recursion_depth--;

   if (*start == 0)
   {
      *end = start;
      return FR_EVAL_TRUE;
   }

   if (*start != BRACKET_OPEN)
      return FR_PARSE_ERROR;

   /***** Get the current expression. *****/
   last_char = *end = find_bracket_end(start);
   if (*end == 0)
      return FR_PARSE_ERROR;
   (*end)++; /* Move the end pointer past the closing bracket. */

   /**** 
    * Check for the three legal characters that can follow an expression,
    * if the following character isn't one of those, the following character 
    * is trash.
    ****/
   if (!(**end == BRACKET_OPEN || **end == BRACKET_CLOSE || **end == '\0'))
      return FR_PARSE_ERROR;

   /***** check for boolean op. *****/
   cur = start;
   cur++;

   switch (*cur)
   {
      case('&'):
         /***** And. *****/
      case('|'):
         /***** Or. *****/
         {
            if (*cur == '&')
               stop_condition = FR_EVAL_FALSE;
            else if (*cur == '|')
               stop_condition = FR_EVAL_TRUE;

            cur++; /* Move past operator. */

            /*** Ensure that we have at least one operator. ***/
            if (*cur != BRACKET_OPEN || cur >= last_char)
               return FR_PARSE_ERROR;

            /*** Evaluate each operand. ***/
            /* NOTE: Due to the condition on the above "if", we are guarenteed that the first iteration of the loop is valid. */
            do
            {
               err = filterv1(cur, &cur, slp_attr, recursion_depth);
               /*** Propagate errors. ***/
               if (err != FR_EVAL_TRUE && err != FR_EVAL_FALSE)
                  return err;

               /*** Short circuit. ***/
               if (err == stop_condition)
                  return stop_condition;
            }
            while (*cur == BRACKET_OPEN && cur < last_char); 


            /*** If we ever get here, it means we've evaluated every operand without short circuiting -- meaning that the operation failed. ***/
            return(stop_condition
                  == FR_EVAL_TRUE
                  ? FR_EVAL_FALSE
                  : FR_EVAL_TRUE);
         }

      case('!'):
         /***** Not. *****/
         /**** Child. ****/
         cur++;
         err = filterv1(cur, &cur, slp_attr, recursion_depth);
         /*** Return errors. ***/
         if (err != FR_EVAL_TRUE && err != FR_EVAL_FALSE)
            return err;

         /*** Perform "not". ***/
         return(err == FR_EVAL_TRUE ? FR_EVAL_FALSE : FR_EVAL_TRUE);

      default:
         /***** Unknown operator. *****/
         ;
         /* We don't do anything here because this will catch the first character of every leaf predicate. */
   }

   /***** Check for leaf operator. *****/
   if (IS_VALID_TAG_CHAR(*cur))
   {
      Operation op;
      char * lhs, * rhs; /* The two operands. */
      char * val_start; /* The character after the operator. ie, the start of the rhs. */
      size_t lhs_len, rhs_len; /* Length of the lhs/rhs. */
      SLPType type;
      SLPError slp_err;

      /**** Demux leaf op. ****/
      /* Support only "==", we look for the equals 
       * sign, and then poke around on either side of that for the real 
       * value. 
       */
      operator = (char *) memchr(cur, '=', last_char - cur);
      if (operator == 0 || *(operator + 1) != '=')
      {
         /**** No search operator. ****/
         return FR_PARSE_ERROR;
      }

      /* The rhs always follows the operator. (This doesn't really make 
      sense for PRESENT ops, but ignore that). */
      val_start = operator + 2; 

      if (operator == cur)
      {
         /* Check that we can poke back one char. */
         return FR_PARSE_ERROR;
      }

      op = EQUAL;

      /***** Get operands. *****/
      /**** Left. ****/
      lhs_len = operator - cur;
      lhs = (char *) cur;

      /**** Right ****/
      rhs_len = last_char - val_start;
      rhs = val_start;

      /***** Do leaf operation. *****/
      /**** Check that tag exists. ****/
      slp_err = SLPAttrGetType_len(slp_attr, lhs, lhs_len, &type);
      if (slp_err == SLP_TAG_ERROR)
      {
         /* Tag  doesn't exist. */
         return FR_EVAL_FALSE;
      }
      else if (slp_err == SLP_OK)
      {
         /* Tag exists. */
         /**** Do operation. *****/
         if (op == PRESENT)
         {
            /*** Since the PRESENT operation is the same for all types, 
            do that now. ***/
            return FR_EVAL_TRUE;
         }
         else
         {
            /*** A type-specific operation. ***/
            switch (type)
            {
               case(SLP_BOOLEAN):
                  err = bool_op(slp_attr, lhs, lhs_len, rhs, rhs_len, op); 
                  break;
               case(SLP_INTEGER):
                  err = int_op(slp_attr, lhs, lhs_len, rhs, op); 
                  break;
               case(SLP_KEYWORD):
                  err = keyw_op(slp_attr, lhs, rhs, op); 
                  break;
               case(SLP_STRING):
                  err = str_op(slp_attr, lhs, lhs_len, rhs, rhs_len, op); 
                  break;
               case(SLP_OPAQUE):
                  SLP_ASSERT(0); /* Opaque is not yet supported. */
            }
         }
      }
      else
      {
         /* Some other tag-related error. */
         err = FR_INTERNAL_SYSTEM_ERROR;
      }

      SLP_ASSERT(err != FR_UNSET);
      return err;
   }

   /***** No operator. *****/
   return FR_PARSE_ERROR;
}
#endif

/** Generate a list of filtered attributes.
 *
 * @param[in] start - The start of the string to work in.
 * @param[out] end - The end of the string processed. After successful 
 *    processing (ie, no PARSE_ERRORs), this will be pointed at the 
 *    character following the last char in this level of the expression.
 * @param[in] slp_attr - The attributes handle to compare on.
 * @param[in] recursion_depth - The maximum depth to recurse during search.
 *
 * @return SLP_OK on successful search (ie, the search was do-able).
 *    SLP_PARSE_ERROR on search error. The end of the expression is
 *    returned through end.
 *
 * @internal
 */
static FilterResult filter(const char * start, const char ** end,
      SLPAttributes slp_attr, int recursion_depth)
{
   char *operator; /* Pointer to the operator substring. */
   const char * cur; /* Current working character. */
   const char * last_char; /* The last character in the working string. */
   FilterResult err = FR_UNSET; /* The result of an evaluation. */
   FilterResult stop_condition = FR_UNSET;

   if (recursion_depth <= 0)
      return FR_PARSE_ERROR;

   recursion_depth--;

   if (*start != BRACKET_OPEN)
      return FR_PARSE_ERROR;

   /***** Get the current expression. *****/
   last_char = *end = find_bracket_end(start);
   if (*end == 0)
      return FR_PARSE_ERROR;
   (*end)++; /* Move the end pointer past the closing bracket. */

   /**** 
    * Check for the three legal characters that can follow an expression,
    * if the following character isn't one of those, the following character 
    * is trash.
    ****/
   if (!(**end == BRACKET_OPEN || **end == BRACKET_CLOSE || **end == '\0'))
      return FR_PARSE_ERROR;

   /***** check for boolean op. *****/
   cur = start;
   cur++;

   switch (*cur)
   {
      case('&'):
         /***** And. *****/
      case('|'):
         /***** Or. *****/
         {
            if (*cur == '&')
               stop_condition = FR_EVAL_FALSE;
            else if (*cur == '|')
               stop_condition = FR_EVAL_TRUE;

            cur++; /* Move past operator. */

            /*** Ensure that we have at least one operator. ***/
            if (*cur != BRACKET_OPEN || cur >= last_char)
               return FR_PARSE_ERROR;

            /*** Evaluate each operand. ***/
            /* NOTE: Due to the condition on the above "if", we are guarenteed that the first iteration of the loop is valid. */
            do
            {
               err = filter(cur, &cur, slp_attr, recursion_depth);
               /*** Propagate errors. ***/
               if (err != FR_EVAL_TRUE && err != FR_EVAL_FALSE)
                  return err;

               /*** Short circuit. ***/
               if (err == stop_condition)
                  return stop_condition;
            }
            while (*cur == BRACKET_OPEN && cur < last_char); 


            /*** If we ever get here, it means we've evaluated every operand without short circuiting -- meaning that the operation failed. ***/
            return(stop_condition
                  == FR_EVAL_TRUE
                  ? FR_EVAL_FALSE
                  : FR_EVAL_TRUE);
         }


      case('!'):
         /***** Not. *****/
         /**** Child. ****/
         cur++;
         err = filter(cur, &cur, slp_attr, recursion_depth);
         /*** Return errors. ***/
         if (err != FR_EVAL_TRUE && err != FR_EVAL_FALSE)
            return err;

         /*** Perform "not". ***/
         return(err == FR_EVAL_TRUE ? FR_EVAL_FALSE : FR_EVAL_TRUE);

      default:
         /***** Unknown operator. *****/
         ;
         /* We don't do anything here because this will catch the first character of every leaf predicate. */
   }

   /***** Check for leaf operator. *****/
   if (IS_VALID_TAG_CHAR(*cur))
   {
      Operation op;
      char * lhs, * rhs; /* The two operands. */
      char * val_start; /* The character after the operator. ie, the start of the rhs. */
      size_t lhs_len, rhs_len; /* Length of the lhs/rhs. */
      SLPType type;
      SLPError slp_err;

      /**** Demux leaf op. ****/
      /* Since all search operators contain a "=", we look for the equals 
       * sign, and then poke around on either side of that for the real 
       * value. 
       */
      operator = (char *) memchr(cur, '=', last_char - cur);
      if (operator == 0)
      {
         /**** No search operator. ****/
         return FR_PARSE_ERROR;
      }

      /* The rhs always follows the operator. (This doesn't really make 
      sense for PRESENT ops, but ignore that). */
      val_start = operator + 1; 

      /* Check for APPROX, GREATER, or LESS. Note that we shuffle the 
      operator pointer back to point at the start of the op. */
      if (operator == cur)
      {
         /* Check that we can poke back one char. */
         return FR_PARSE_ERROR;
      }

      switch (*(operator - 1))
      {
         case('~'):
            op = EQUAL; /* See Assumptions. */
            operator--;
            break;
         case('>'):
            op = GREATER;
            operator--;
            break;
         case('<'):
            op = LESS;
            operator--;
            break;
         default:
            /* No prefix to the '='. */
            /**** Check for PRESENT. ****/
            if ((operator == last_char - 2) && (*(operator + 1) == '*'))
               op = PRESENT;
            /**** It's none of the above: therefore it's EQUAL. ****/
            else
               op = EQUAL;
      }

      /***** Get operands. *****/
      /**** Left. ****/
      lhs_len = operator - cur;
      lhs = (char *) cur;

      /**** Right ****/
      rhs_len = last_char - val_start;
      rhs = val_start;

      /***** Do leaf operation. *****/
      /**** Check that tag exists. ****/
      slp_err = SLPAttrGetType_len(slp_attr, lhs, lhs_len, &type);
      if (slp_err == SLP_TAG_ERROR)
      {
         /* Tag  doesn't exist. */
         return FR_EVAL_FALSE;
      }
      else if (slp_err == SLP_OK)
      {
         /* Tag exists. */
         /**** Do operation. *****/
         if (op == PRESENT)
         {
            /*** Since the PRESENT operation is the same for all types, 
            do that now. ***/
            return FR_EVAL_TRUE;
         }
         else
         {
            /*** A type-specific operation. ***/
            switch (type)
            {
               case(SLP_BOOLEAN):
                  err = bool_op(slp_attr, lhs, lhs_len, rhs, rhs_len, op); 
                  break;
               case(SLP_INTEGER):
                  err = int_op(slp_attr, lhs, lhs_len, rhs, op); 
                  break;
               case(SLP_KEYWORD):
                  err = keyw_op(slp_attr, lhs, rhs, op); 
                  break;
               case(SLP_STRING):
                  err = str_op(slp_attr, lhs, lhs_len, rhs, rhs_len, op); 
                  break;
               case(SLP_OPAQUE):
                  SLP_ASSERT(0); /* Opaque is not yet supported. */
            }
         }
      }
      else
      {
         /* Some other tag-related error. */
         err = FR_INTERNAL_SYSTEM_ERROR;
      }

      SLP_ASSERT(err != FR_UNSET);
      return err;
   }

   /***** No operator. *****/
   return FR_PARSE_ERROR;
}

/** Determine whether an attribute list satisfies a predicate.
 *
 * @param[in] version - The SLP version of the predicate string 
 *    (should always be 2 since we don't handle SLPv1 predicates yet).
 * @param[in] attrlistlen - The length of @p attrlist in bytes.
 * @param[in] attrlist - The attribute list to test.
 * @param[in] predicatelen - The length of @p predicate in bytes.
 * @param[in] predicate - The predicate string.
 *
 * @return A Boolean value; false if test fails. Non-zero if test fails
 *    or if there is a parse error in the predicate string.
 */
int SLPDPredicateTest(int version, size_t attrlistlen, 
      const char * attrlist, size_t predicatelen, const char * predicate)
{
   SLPAttributes attr;
   const char * end; /* Pointer to the end of the parsed attribute string. */
   FilterResult err;
   char attrnull;
   char prednull;
   int result = 0;

   /* An NULL or empty string is always true. */
   if (predicate == 0 || *(char *) predicate == 0)
      return 1;

   /* Attempt v1 too. */
   if (version != 2 
#if defined(ENABLE_SLPv1)        
       && version != 1
#endif
      )
      return 1;

   /* TRICKY: Temporarily NULL terminate the attribute list and the predicate 
    * string. We can do this because there is room in the corresponding SLPv2 
    * SRVREG SRVRQST messages. Basically we are squashing the authcount and 
    * the spi string length. Don't worry, we fix things up later and it is 
    * MUCH faster than a malloc for a new buffer 1 byte longer!
    */
   prednull = predicate[predicatelen];
   ((char *) predicate)[predicatelen] = 0;
   attrnull = attrlist[attrlistlen];
   ((char *) attrlist)[attrlistlen] = 0;

   /* Generate an SLPAttr from the comma delimited list */
   if (SLPAttrAlloc("en", NULL, SLP_FALSE, &attr) == 0)
   {
      if (SLPAttrFreshen(attr, attrlist) == 0)
      {
         switch (version)
         {
#if defined(ENABLE_SLPv1)
            case 1:
               err = filterv1(predicate, &end, attr, SLPD_ATTR_RECURSION_DEPTH);
               break;
#endif
            default:
               err = filter(predicate, &end, attr, SLPD_ATTR_RECURSION_DEPTH);
               break;
         }

         /* Check for trailing trash data. */
         if ((err == FR_EVAL_TRUE || err == FR_EVAL_FALSE) && *end != 0)
            result = 0;
         else if (err == FR_EVAL_TRUE)
            result = 1;
      }

      SLPAttrFree(attr);
   }

   /* Un null terminate */
   ((char *)predicate)[predicatelen] = prednull;
   ((char *)attrlist)[attrlistlen] = attrnull;

   return result;
}

/** Copies attributes from a list to a string.
 *
 * Copies attributes from the specified attribute list to a result string
 * according to the taglist as described by section 10.4. of RFC 2608
 *
 * @param[in] attrlistlen - The length of @p attrlist in bytes.
 * @param[in] attrlist - The attribute list to test.
 * @param[in] taglistlen - The length of @p taglist in bytes.
 * @param[in] taglist - 
 * @param[out] resultlen - The address of storage for the length of the 
 *    returned @p result parameter.
 * @param[out] result - The address of storage for the returned result 
 *    string.
 *
 * @return Zero on success, or a non-zero value on failure.
 */
int SLPDFilterAttributes(int attrlistlen, const char * attrlist,
      size_t taglistlen, const char * taglist, size_t * resultlen, 
      char ** result)
{
   SLPAttributes attr;
   FilterResult err;
   char attrnull;
   char tagnull;

   *result = 0;
   *resultlen = 0;

   /* TRICKY: Temporarily NULL-terminate the attribute list and the tag 
    * string. We can do this because there is room in the corresponding 
    * SLPv2 SRVREG and ATTRRQST messages. Basically we are squashing the 
    * authcount and the spi string length. Don't worry, we fix things up 
    * later and it is MUCH faster than a malloc for a new buffer 1 byte
    * longer!
    */
   tagnull = taglist[taglistlen];
   ((char *)taglist)[taglistlen] = 0;
   attrnull = attrlist[attrlistlen];
   ((char *)attrlist)[attrlistlen] = 0;

   /* Generate an SLPAttr from the comma delimited list */
   err = 1;
   if (SLPAttrAlloc("en", NULL, SLP_FALSE, &attr) == 0)
   {
      if (SLPAttrFreshen(attr, attrlist) == 0)
         err = SLPAttrSerialize(attr, taglist, result, *resultlen, resultlen,
                     SLP_FALSE);

      SLPAttrFree(attr);
   }

   /* SLPAttrSerialize counts the NULL terminator which we don't care about*/
   if (*resultlen)
      *resultlen -= 1;

   /* Un-null terminate */
   ((char *)taglist)[taglistlen] = tagnull;
   ((char *)attrlist)[attrlistlen] = attrnull;

   return(*resultlen == 0);
}

/*=========================================================================*/
