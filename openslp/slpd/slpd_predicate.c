/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_database.c                                            */
/*                                                                         */
/* Abstract:    This files contains an implementation of LDAPv3 search     */
/*              filters for SLP (as specified in RFC 2254).                */
/*                                                                         */
/* TODO: The current implementation reparses the predicate string every    */
/*       time it is evaluated. This implementation should be refactored to */
/*       parse the predicate string once, build some sort of data-         */
/*       structure out of that, and then compare the predicate DS with the */
/*       attribute DS.                                                     */
/*                                                                         */
/* Legal Mumbo-jumbo: Standard. See some other file.                       */
/*                                                                         */
/***************************************************************************/

#include "slpd.h"



/******************************************************************************
 *
 *                          PREDICATE STRUCTURE
 *
 *****************************************************************************/

#include <ctype.h>

/*=========================================================================*/
SLPError SLPDPredicateAlloc(const char *predicate_str, size_t len, SLPDPredicate *pred) 
/*                                                                         */
/* Create a predicate structure.                                           */
/*                                                                         */
/* predicate    (IN) the predicate string                                  */
/*                                                                         */
/* len          (IN) the length of the predicate string                    */
/*                                                                         */
/* pred         (IN) the predicate struct to populate                      */
/*                                                                         */
/* Returns:                                                                */
/*   SLP_OK if allocated properly.                                         */
/*   SLP_PARSE_ERROR if there is an error in the predicate string.         */
/*   SLP_MEMORY_ALLOC_FAILED if out of memory                              */
/*                                                                         */
/*=========================================================================*/
{
    const char *start_cur; /* Used to find first non-WS char in pred. */
    const char *end_cur; /* Used to find last non-WS char in pred. */

    char *new_pred; /* Temporary pointer for working with the under-construction predicate "object". */

    size_t real_len; /* The length of the elided whitespace. */

    /***** Elide start. *****/
    start_cur = predicate_str;
    while (((start_cur - predicate_str) < len) && (isspace(*start_cur)))
    {
        start_cur++;
    }

    /***** Elide end. *****/
    end_cur = predicate_str + len;
    while ((end_cur >= start_cur) && (isspace(*end_cur)))
    {
        end_cur--;
    }

    /***** Return if empty *****/
    if (end_cur < start_cur)
    {
        *pred = NULL; /* Empty string. */
        return SLP_OK;
    }

    /***** Copy *****/
    real_len = end_cur - start_cur;
    new_pred = (char *)malloc(real_len + 1);

    if (new_pred == NULL)
    {
        return SLP_MEMORY_ALLOC_FAILED;
    }

    /* Copy. */
    strncpy(new_pred, predicate_str, real_len);

    /* Null terminate. */
    new_pred[real_len] = 0;

    /* Set value. */
    *pred = (SLPDPredicate)new_pred;

    return SLP_OK;
}


void SLPDPredicateFree(SLPDPredicate *victim) 
{
    if (victim != NULL)
    {
        free(victim);
    }
    return;
}


#ifdef USE_PREDICATES

    #include <slp_logfile.h>

/*********
 *
 * Assumptions:
 *  - If a tag specified in the query string does not exist, that's a FALSE
 *  - The "Undefined" value mentioned in the LDAPv3 RFC (2251) is synonymous 
 *    with "false".
 *  - The "Approx" operator matches as an equal operator. 
 *  - Variable matching on type is strict: ie, the right-hand-side of an 
 *    operator must evaluate to the same type as the tag referenced on the 
 *    left-hand-side. 
 *
 * Known Bugs/TODO:
 *  - If trash follows the last operand to a binary argument, it will be 
 *    ignored if the operand is not evaluated due to short circuiting: 
 *    ie, in "(&(first=*)(second<=3)trash)", "trash" will be silently 
 *    ignored _if_ "(first=*)" is true.
 *  - Escaped '*' characters are treated as wildcards in the string equality 
 *    operator: ie, in "(string=abc\2axyz)" will match the literal string
 *    "abc\2axyz" instead of "abc*xyz". 
 *  - No operations can be performed on opaque data types. 
 *********/

    #define _GNU_SOURCE	/*FIXME <-- Seems somewhat hackish. */
    #include <fnmatch.h>
    #undef _GNU_SOURCE
    #include <stdlib.h>
    #include <stdio.h>
    #include <string.h>
    #include <assert.h>
    #include <unistd.h>

    #define BRACKET_OPEN '('
    #define BRACKET_CLOSE ')'

/************************* <Lifted from slp_attr.c> ***********************/

/* Tests a character to see if it reserved (as defined in RFC 2608, p11). */
    #define IS_RESERVED(x) \
    (((x) == '(' || (x) == ')' || (x) == ',' || (x) == '\\' || (x) == '!' || (x) == '<' \
    || (x) == '=' || (x) == '>' || (x) == '~') || \
    ((((char)0x01 <= (x)) && ((char)0x1F >= (x))) || ((x) == (char)0x7F)))


    #define IS_INVALID_VALUE_CHAR(x) \
    IS_RESERVED(x)

    #define IS_INVALID_TAG_CHAR(x) \
    (IS_RESERVED(x) \
    || ((x) == '*') || \
    ((x) == (char)0x0D) || ((x) == (char)0x0A) || ((x) == (char)0x09) || ((x) == '_'))

    #define IS_VALID_TAG_CHAR(x) !IS_INVALID_TAG_CHAR(x)

/************************* </Lifted from slp_attr.c> ***********************/

/* Tests a string to see if it is a boolean. */
int is_bool_string(const char *str, SLPBoolean *val)
{
    if (strcmp(str, "true") == 0)
    {
        *val = SLP_TRUE;
        return 1;
    }
    if (strcmp(str, "false") == 0)
    {
        *val = SLP_FALSE;
        return 1;
    }
    return 0;
}


typedef enum
{
    EQUAL, APPROX, GREATER, LESS, PRESENT, SUBSTRING
} Operation;

/* Perform an integer operation. */
SLPError int_op(SLPAttributes slp_attr, char *tag, char *rhs, Operation op, SLPBoolean *result)
{
    long rhs_val; /* The converted value of rhs. */
    char *end; /* Pointer to the end of op. */

    long *tag_vals; /* Array of ints. */
    size_t size; /* Length of tag_vals. */
    size_t i; /* Index into tag_vals. */

    SLPError slp_err;

    assert(op != PRESENT);

    *result = SLP_FALSE; /* Be pessamistic. */

    /***** Verify and convert rhs. *****/
    rhs_val = strtol(rhs, &end, 10);

    if (*end != '\0')
    {
        /* Trying to compare an int with a non-int. */
        *result = SLP_FALSE;
        return SLP_OK;
    }

    /***** Get tag value. *****/
    slp_err = SLPAttrGet_int(slp_attr, tag, &tag_vals, &size);

    if (slp_err != SLP_OK)
    {
        /* TODO A more careful examination of return values. */
        *result = SLP_FALSE;
        return SLP_OK;
    }

    /***** Compare. *****/
    assert(op != PRESENT); 
    switch (op)
    {
    case(EQUAL):
        for (i = 0; i < size; i++)
        {
            if (tag_vals[i] == rhs_val)
            {
                *result = SLP_TRUE;
                break;
            }
        }
        break;
    case(APPROX):
        assert(0); /* TODO: Figure out how this works later. */
    case(GREATER):
        for (i = 0; i < size; i++)
        {
            if (tag_vals[i] >= rhs_val)
            {
                *result = SLP_TRUE;
                break;
            }
        }
        break;
    case(LESS):
        for (i = 0; i < size; i++)
        {
            if (tag_vals[i] <= rhs_val)
            {
                *result = SLP_TRUE;
                break;
            }
        }
        break;
    case(SUBSTRING):
        *result = SLP_FALSE; /* Can't do a substring search on an int. */
    default:
        assert(0);
    }

    /***** Clean up. *****/
    free(tag_vals);

    return SLP_OK;
}


/* Perform a keyword operation. */
SLPError keyw_op(SLPAttributes slp_attr, char *tag, char *rhs, Operation op, SLPBoolean *result)
{
    /* Note that the only test keywords are allowed to undergo is PRESENT, 
     * also note that PRESENT is handled by our calling function.
     *
     * We are therefore quite justified in saying:
     */
    assert(op != PRESENT);
    *result = SLP_FALSE; /* Because any illegal ops simply fail. */
    return SLP_OK;
}


/* Perform a boolean operation. */
SLPError bool_op(SLPAttributes slp_attr, char *tag, char *rhs, Operation op, SLPBoolean *result)
{
    SLPBoolean rhs_val; /* The value of the rhs. */
    SLPBoolean tag_val; /* The value associated with the tag. */
    SLPError slp_err;

    assert(op != PRESENT);

    /* Note that the only valid non-PRESENT operator is EQUAL. */
    if (op != EQUAL)
    {
        *result = SLP_FALSE;
        return SLP_OK;
    }

    /***** Get and check the rhs value. *****/
    if (!is_bool_string(rhs, &rhs_val))
    {
        *result = SLP_FALSE;
        return SLP_OK;
    }

    /***** Get the tag value. *****/
    slp_err = SLPAttrGet_bool(slp_attr, tag, &tag_val);

    if (slp_err != SLP_OK)
    {
        *result = SLP_FALSE;
        return SLP_OK;
    }

    /***** Compare. *****/
    if (tag_val == rhs_val)
    {
        *result = SLP_TRUE; 
    }
    else
    {
        *result = SLP_FALSE;
    }

    return SLP_OK;
}



/* Perform a string operation. */
SLPError str_op(SLPAttributes slp_attr, char *tag, char *rhs, Operation op, SLPBoolean *result)
{
    char *rhs_val; /* Converted value of rhs. */

    char **tag_vals; /* Array of strings. */
    size_t size; /* Length of tag_vals. */
    size_t i; /* Index into tag_vals. */

    SLPError slp_err;

    assert(op != PRESENT);

    *result = SLP_FALSE; /* Be pessamistic. */

    /***** Verify and convert rhs. *****/
    /* TODO: Unescape. */
    rhs_val = strdup(rhs); /* Free()'d below. */

    /***** Get tag value. *****/
    slp_err = SLPAttrGet_str(slp_attr, tag, &tag_vals, &size);

    if (slp_err != SLP_OK)
    {
        /* TODO A more careful examination of return values. */
        *result = SLP_FALSE;
        return SLP_OK;
    }

    /***** Compare. *****/
    assert(op != PRESENT); 
    i = 0;
    switch (op)
    {
    case(LESS):
        for (i = 0; i < size; i++)
        {
            if (strcmp(tag_vals[i], rhs_val) <= 0)
            {
                *result = SLP_TRUE;
                break;
            }
            free(tag_vals[i]); /* Scorched earth. */
        }
        break;
    case(EQUAL):
        for (i = 0; i < size; i++)
        {
            if (fnmatch(rhs_val, tag_vals[i], FNM_CASEFOLD) == 0)
            {
                *result = SLP_TRUE;
                break;
            }
            free(tag_vals[i]); /* Scorched earth. */
        }
        break;
    case(APPROX):
        assert(0); /* TODO: Figure out how this works later. */
    case(GREATER):
        for (i = 0; i < size; i++)
        {
            if (strcmp(tag_vals[i], rhs_val) >= 0)
            {
                *result = SLP_TRUE;
                break;
            }
            free(tag_vals[i]); /* Scorched earth. */
        }
        break;
    case(SUBSTRING):
        *result = SLP_FALSE; /* Can't do a substring search on an int. */
        free(tag_vals[i]);
        break;
    default:
        assert(0);
    }

    /***** Clean up. *****/
    /* Note that the initializer is empty. This is because all of the above 
     * cases read until the first success, freeing as they go. Here, we free 
     * the remaining values.
     */
    for (; i < size; i++)
    {
        free(tag_vals[i]);
    }
    free(tag_vals);

    free(rhs_val);

    return SLP_OK;
}


/* Tests a string to see if it consists wholly of numeric characters. */
int is_int_string(const char *str)
{
    int i;

    for (i=0; str[i] != '\0'; i++)
    {
        if (!((!isdigit((int)str[i])) || str[i] == '-'))
        {
            return 0;
        }
    }

    return 1;
}

/* Returns ptr to start of substring, or null. */
char *find_substr(char *src, int src_len, char *to_find)
{
    int i;

    for (i = 0; i < src_len; i++)
    {
        if (src[i] == *to_find)
        {
            int old_i = i; /* Save the old start. */
            int find_index;
            for (find_index = 0; 
                (to_find[find_index] != '\0') && (i < src_len) && (to_find[find_index] == src[i]); to_find++, i++)
            {

            }

            if (to_find[find_index] == '\0')
            {
                return src + i;
            }

            i = old_i; /* Restor the old start. */
        }
    }
    return NULL;
}



/* Finds a bracket matched to this one. Returns NULL if there isn't one. */
char *find_bracket_end(const char *str)
{
    int open_count; 
    const char *cur;

    if (*str != BRACKET_OPEN)
    {
        return NULL;
    }

    open_count = 0;

    /***** Count brackets. *****/
    for (cur = str; *cur != '\0'; cur++)
    {
        assert(open_count >= 0);

        /**** Check current character. ****/
        if (*cur == BRACKET_OPEN)
        {
            open_count++;
        }
        else if (*cur == BRACKET_CLOSE)
        {
            open_count--;
        }

        /**** Check if we've found bracket end. ****/
        if (open_count == 0)
        {
            return(char *)cur;
        }
    }

    return NULL;
}       

/* Represents a string and its length. */
struct pair
{
    char *start;
    char *end; /* Stop reading _BEFORE_ end. ie, at end-1. */
};


/*
 * Returns: SLP_OK on successful search (ie, the search was do-able).
 *  0 on search error. 
 *
 *  The end of the expression is returned through end. 
 * 
 */
SLPError filter(char *start, char **end, SLPAttributes slp_attr, SLPBoolean *return_value, int recursion_depth)
{
    char *operator; /* Pointer to the operator substring. */
    char *cur; /* Current working character. */
    char *last_char; /* The last character in the working string. */
    SLPError err = SLP_NETWORK_INIT_FAILED; /* Error code of a recursive call -- since nothing in here should ever go off-machine, the value is set to SLP_NETWORK_INIT_FAILED. The value of err must NOT be SLP_NETWORK_INIT_FAILED at exit. */
    SLPBoolean result;


    if (recursion_depth <= 0)
    {
        return SLP_PARSE_ERROR;
    }
    recursion_depth--;

    if (*start != BRACKET_OPEN)
    {
        return SLP_PARSE_ERROR;
    }

    /***** Get the current expression. *****/
    last_char = *end = find_bracket_end(start);
    if (*end == NULL)
    {
        return SLP_PARSE_ERROR;
    }
    (*end)++; /* Move the end pointer past the closing bracket. */

    /**** 
     * Check for the three legal characters that can follow an expression,
     * if the following character isn't one of those, the following character 
     * is trash.
     ****/
    if (!(**end == BRACKET_OPEN || **end == BRACKET_CLOSE || **end == '\0'))
    {
        return SLP_PARSE_ERROR;
    }

    /***** check for boolean op. *****/
    cur = start;
    cur++;

    switch (*cur)
    {
    case('&'): /***** And. *****/
        /**** Left child *****/
        cur++;
        err = filter(cur, &cur, slp_attr, &result, recursion_depth);
        if (err != SLP_OK)
        {
            return err;
        }

        /**** Do short circuiting... ****/
        if (result != SLP_TRUE)
        {
            *return_value = SLP_FALSE;
            return SLP_OK;
        }

        /**** Right child *****/
        err = filter(cur, &cur, slp_attr, &result, recursion_depth);
        if (err != SLP_OK)
        {
            return err;
        }

        *return_value = result;
        return SLP_OK;

    case('|'): /***** Or. *****/
        /**** Left child *****/
        cur++;
        err = filter(cur, &cur, slp_attr, &result, recursion_depth);
        if (err != SLP_OK)
        {
            return err;
        }

        /**** Do short circuiting... ****/
        if (result == SLP_TRUE)
        {
            *return_value = SLP_TRUE;
            return SLP_OK;
        }

        /**** Right child *****/
        err = filter(cur, &cur, slp_attr, &result, recursion_depth);
        if (err != SLP_OK)
        {
            return err;
        }

        *return_value = result;
        return SLP_OK;

    case('!'): /***** Not. *****/
        /**** Child. ****/
        cur++;
        err = filter(cur, &cur, slp_attr, &result, recursion_depth);
        if (err != SLP_OK)
        {
            return err;
        }

        *return_value = ( result == SLP_TRUE ? SLP_FALSE : SLP_TRUE );
        return SLP_OK;

    default: /***** Uh oh, could be an error... *****/
    }

    /***** Check for leaf operator. *****/
    if (IS_VALID_TAG_CHAR(*cur))
    {
        Operation op;
        char *lhs, *rhs; /* The two operands. */
        char *val_start; /* The character after the operator. ie, the start of the rhs. */
        int len;
        SLPType type;
        SLPError slp_err;

        /* Since all search operators contain a "=", we look for the equals 
         * sign, and then poke around on either side of that for the real 
         * value. 
         */
        operator = (char *)memchr(cur, '=', last_char - cur);
        if (operator == NULL)
        {
            /**** No search operator. ****/
            return SLP_PARSE_ERROR;
        }

        val_start = operator + 1; /* The rhs always follows the operator. (This doesn't really make sense for PRESENT ops, but ignore that). */
        /**** Check for APPROX, GREATER, or LESS. Note that we shuffle the operator pointer back to point at the start of the op. ****/
        if (operator == cur)
        {/* Check that we can poke back one char. */
            return SLP_PARSE_ERROR;
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
        default: /* No prefix to the '='. */
            /**** Check for PRESENT. ****/
            if ((operator == last_char - 2) && (*(operator+1) == '*'))
            {
                op = PRESENT;
            }
            /**** It's none of the above: therefore it's EQUAL. ****/
            else
            {
                op = EQUAL;
            }
        }


        /***** Get operands. *****/
        /**** Left. ****/
        len = operator - cur;
        lhs = (char *)malloc(len + 1);
        if (lhs == NULL)
        {
            return SLP_MEMORY_ALLOC_FAILED;
        }
        strncpy(lhs, cur, len);
        lhs[len] = '\0';

        /**** Right ****/
        len = last_char - val_start;
        rhs = (char *)malloc(len + 1);
        if (rhs == NULL)
        {
            return SLP_MEMORY_ALLOC_FAILED;
        }
        strncpy(rhs, val_start, len);
        rhs[len] = '\0';

        /***** Do leaf operation. *****/
        /**** Check that tag exists. ****/
        slp_err = SLPAttrGetType(slp_attr, lhs, &type);
        if (slp_err == SLP_TAG_ERROR)
        { /* Tag  doesn't exist. */
            *return_value = SLP_FALSE;
            err = SLP_OK;
        }
        else if (slp_err == SLP_OK)
        { /* Tag exists. */
            /**** Do operation. *****/
            if (op == PRESENT)
            {
                /*** Since the PRESENT operation is the same for all types, do that now. ***/
                *return_value = SLP_TRUE;
                err = SLP_OK;
            }
            else
            {
                /*** A type-specific operation. ***/
                switch (type)
                {
                case(SLP_BOOLEAN):
                    err = bool_op(slp_attr, lhs, rhs, op, return_value); 
                    break;
                case(SLP_INTEGER):
                    err = int_op(slp_attr, lhs, rhs, op, return_value); 
                    break;
                case(SLP_KEYWORD):
                    err = keyw_op(slp_attr, lhs, rhs, op, return_value); 
                    break;
                case(SLP_STRING):
                    err = str_op(slp_attr, lhs, rhs, op, return_value); 
                    break;
                case(SLP_OPAQUE):
                    assert(0);
                }
            }
        }
        else
        { /* Some other tag-related error. */
            err = SLP_INTERNAL_SYSTEM_ERROR;
        }

        /***** Clean up. *****/
        free(lhs);
        if (rhs != NULL)
        {
            free(rhs);
        }
        assert(err != SLP_NETWORK_INIT_FAILED);
        return err;
    }

    /***** No operator. *****/
    return SLP_PARSE_ERROR;
}


/* Test a predicate. 
 *
 * This is mostly a wrapper to filter(), but it does additional error 
 * checking,  and puts error codes in terms of something a wee bit more 
 * standard. 
 *
 * Parameters:
 * 
 *  recursion_depth -- dictates how many bracketed expressions are to be 
 *      explored. It is decremented to zero, at which point parsing stops and 
 *      a SLP_PARSE_ERROR is returned. 
 *
 * Returns:
 *  SLP_OK -- If test was performed, the result (true or false) is returned 
 *      through the parameter result.
 *  SLP_PARSE_ERROR -- If there was a syntax error in the predicate, no 
 *      result is returned through result.
 *  SLP_INTERNAL_SYSTEM_ERROR -- Something went wrong internally.
 *  
 */
SLPError SLPAttrEvalPred(SLPAttributes slp_attr, SLPDPredicate predicate, SLPBoolean *result, int recursion_depth)
{
    char *end; /* Pointer to the end of the parsed attribute string. */
    SLPError err;

    /***** An empty string is always true. *****/
    if (predicate == NULL)
    {
        return SLP_OK;
    }

    err = filter((char *)predicate, &end, slp_attr, result, recursion_depth);

    /***** Check for trailing trash data. *****/
    if (err == SLP_OK && *end != '\0')
    {
        return SLP_PARSE_ERROR;
    }

    return err;
}


/*=========================================================================*/
int SLPTestPredicate(SLPDPredicate predicate, SLPAttributes attr) 
/*                                                                         */
/* Determine whether the specified attribute list satisfies                */
/* the specified predicate                                                 */
/*                                                                         */
/* predicatelen (IN) the length of the predicate string                    */
/*                                                                         */
/* predicate    (IN) the predicate string                                  */
/*                                                                         */
/* attr         (IN) attribute list to test                                */
/*                                                                         */
/* Returns: Zero if there is a match, a positive value if there was not a  */
/*          match, and a negative value if there was a parse error in the  */
/*          predicate string.                                              */
/*=========================================================================*/
{
    SLPError err;
    SLPBoolean result = SLP_FALSE;

    err = SLPAttrEvalPred(attr, predicate, &result, 50);
    switch (err)
    {
    case(SLP_OK):
        if (result == SLP_TRUE)
        {
            return 0;
        }
        return 1;

    case(SLP_INTERNAL_SYSTEM_ERROR):
        SLPLog("The predicate string \"%s\" caused an internal error\n", (char *)predicate);
    case(SLP_PARSE_ERROR):
        return -1;

    default:
        SLPLog("SLPAttrEvalPred() returned an out of range value (%d) when evaluating \"%s\"\n", err, (char *)predicate);
    }

    return -1;
//	SLPBoolean result;
//	SLPError err;
//	char *safe_pred; /* FIXME Hack. We copy the contents of the predicate into a buffer and null-terminate it. */
//
//	/* Empty predicates are always true. */
//	if (predicatelen == 0) {
//		return 0;
//	}
//	/* TODO elide. */
//	
//	safe_pred = (char *)malloc(predicatelen + 1);
//	if (safe_pred == NULL) {
//		return -1;
//	}
//	memcpy(safe_pred, predicate, predicatelen);
//	safe_pred[predicatelen] = 0;
//	
//	err = SLPAttrEvalPred(attr, safe_pred, &result, 50);
//
//	free(safe_pred);
//
//	if (err == SLP_OK) {
//		if (result == SLP_TRUE) {
//			return 0;
//		}
//		return 1;
//	}
//	
//	return -1;
}



#else /* USE_PREDICATES */


/* See below for function definition. We ONLY use this if predicates are 
 * disabled. I'm operating on the assumption that the compiler is smart 
 * enough to optimize this out. 
 */
int SLPTestPredicate(SLPDPredicate predicate, SLPAttributes attr) 
{
    return 0;
}
#endif /* USE_PREDICATES */
