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


#include "slpd.h"

#ifdef USE_PREDICATES   

/* The character that is a wildcard. */
#define WILDCARD ('*')
#define BRACKET_OPEN '('
#define BRACKET_CLOSE ')'

/************************* <Lifted from slp_attr.c> ***********************/

/* Tests a character to see if it reserved (as defined in RFC 2608, p11). */
#define IS_RESERVED(x) (((x) == '(' || (x) == ')' || (x) == ',' || (x) == '\\' || (x) == '!' || (x) == '<' || (x) == '=' || (x) == '>' || (x) == '~') || ((((char)0x01 <= (x)) && ((char)0x1F >= (x))) || ((x) == (char)0x7F)))


#define IS_INVALID_VALUE_CHAR(x) IS_RESERVED(x)

#define IS_INVALID_TAG_CHAR(x) (IS_RESERVED(x) || ((x) == '*') || ((x) == (char)0x0D) || ((x) == (char)0x0A) || ((x) == (char)0x09) || ((x) == '_'))

#define IS_VALID_TAG_CHAR(x) (!IS_INVALID_TAG_CHAR(x))

/************************* </Lifted from slp_attr.c> ***********************/


/*--------------------------------------------------------------------------*/
const char *substr(const char *haystack, 
                   const char *needle, 
                   size_t needle_len)
/* Does a case insensitive substring match for needle in haystack.          */
/*                                                                          */
/* Returns pointer to the start of the substring. NULL if the substring is  */
/* not found.                                                               */
/* FIXME This implementation isn't exactly blazingly fast...                */
/*--------------------------------------------------------------------------*/
{
    const char *hs_cur; 

    for (hs_cur = haystack; *hs_cur != 0; hs_cur++)
    {
        if (strncasecmp(hs_cur, needle, needle_len) == 0)
        {
            return hs_cur;
        }
    }

    return NULL;
}


/*--------------------------------------------------------------------------*/
int wildcard(const char *pattern, const char *string)
/* Returns: -1 error,  0 success,  1 failure                                */
/*--------------------------------------------------------------------------*/
{
    const char *cur_pattern; /* current position in the pattern. */
    const char *cur_string; /* Current position in the string. */

    cur_pattern = pattern;
    cur_string = string;

    /***** Is first char in pattern a wildcard? *****/
    if (*cur_pattern == WILDCARD)
    {
        /* Skip ahead. */
        cur_pattern++;
    }
    else
    {
        const char *old_pattern; 

        old_pattern = cur_pattern;
        /* Find first WC. */
        cur_pattern = strchr(cur_pattern, WILDCARD);

        if (cur_pattern == NULL)
        {
            /* No WC. Do basic comparision. */
            return strcasecmp(pattern, cur_string);
        }

        if (strncasecmp(cur_string, old_pattern, cur_pattern - old_pattern) != 0)
        {
            return 1; /* Initial anchored token does not match. */
        }

        /* Move string past its current position. */
        cur_string = cur_string + (cur_pattern - old_pattern); 
    }

    /* We are now guarenteed that cur_pattern starts with a WC. */

    /***** Process all wildcards up till end. *****/
    while (1)
    {
        const char *old_pattern; 
        const char *result;

        /*** Spin past WCs. ***/
        while (*cur_pattern == WILDCARD)
        {
            cur_pattern++;
        }

        if (*cur_pattern == 0)
        {
            /*** Pattern ends with WC. ***/
            return 0;
        }

        old_pattern = cur_pattern;
        /* Find first WC. */
        cur_pattern = strchr(cur_pattern, WILDCARD);

        if (cur_pattern == NULL)
        {
            /* No WC. Do basic comparision. */
            /* Here we check for a rear-anchored string. */
            const char  *end;
            size_t len;

            len = strlen(old_pattern);

            end = cur_string;
            while (*end != 0)
            {
                end++;
            }

            if ((end - cur_string) < len)
            {
                return 1; /* Too small to contain the anchored pattern. */
            }

            if (strcmp(end - len, old_pattern) == 0)
            {
                return 0;
            }
            return 1;

        }

        result = substr(cur_string, old_pattern, cur_pattern - old_pattern);
        if (result == NULL)
        {
            return 1; /* Couldn't find the pattern. */
        }

        /* Move string past its current position. */
        cur_string = result + (cur_pattern - old_pattern); 
    }
}


/*--------------------------------------------------------------------------*/
int is_bool_string(const char *str, SLPBoolean *val)
/* Tests a string to see if it is a boolean. */
/*--------------------------------------------------------------------------*/
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


/*--------------------------------------------------------------------------*/
typedef enum
{
    EQUAL, APPROX, GREATER, LESS, PRESENT, SUBSTRING
} Operation;
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
SLPError int_op(SLPAttributes slp_attr, 
                char *tag, 
                char *rhs, 
                Operation op, 
                SLPBoolean *result)
/* Perform an integer operation.                                            */
/*--------------------------------------------------------------------------*/
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


/*--------------------------------------------------------------------------*/
SLPError keyw_op(SLPAttributes slp_attr, 
                 char *tag, 
                 char *rhs, 
                 Operation op, 
                 SLPBoolean *result)
/* Perform a keyword operation.                                             */
/*--------------------------------------------------------------------------*/
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



/*--------------------------------------------------------------------------*/
SLPError bool_op(SLPAttributes slp_attr, 
                 char *tag, 
                 char *rhs, 
                 Operation op, 
                 SLPBoolean *result)
/* Perform a boolean operation. */
/*--------------------------------------------------------------------------*/
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



/*--------------------------------------------------------------------------*/
SLPError str_op(SLPAttributes slp_attr, 
                char *tag, 
                char *rhs, 
                Operation op, 
                SLPBoolean *result)
/* Perform a string operation.                                              */
/*--------------------------------------------------------------------------*/
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
            if (wildcard(rhs_val, tag_vals[i]) == 0)
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


/*--------------------------------------------------------------------------*/
int is_int_string(const char *str)
/* Tests a string to see if it consists wholly of numeric characters.       */
/*--------------------------------------------------------------------------*/
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


/*--------------------------------------------------------------------------*/
char *find_substr(char *src, int src_len, char *to_find)
/* Returns ptr to start of substring, or null.                              */
/*--------------------------------------------------------------------------*/
{
    int i;

    for (i = 0; i < src_len; i++)
    {
        if (src[i] == *to_find)
        {
            int old_i = i; /* Save the old start. */
            int find_index;
            for (find_index = 0; (to_find[find_index] != '\0') && (i < src_len) && (to_find[find_index] == src[i]); to_find++, i++)
            {

            }

            if (to_find[find_index] == '\0')
            {
                return src + i;
            }

            i = old_i; /* Restor the old start. */
        }
    }
    return 0;
}



/*--------------------------------------------------------------------------*/
char *find_bracket_end(const char *str)
/* Finds a bracket matched to this one. Returns 0 if there isn't one.       */
/*--------------------------------------------------------------------------*/
{
    int open_count; 
    const char *cur;

    if (*str != BRACKET_OPEN)
    {
        return 0;
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

    return 0;
}       


/*--------------------------------------------------------------------------*/
struct pair
/* Represents a string and its length. */
{
    char *start;
    char *end; /* Stop reading _BEFORE_ end. ie, at end-1. */
};
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
SLPError filter(char *start, 
                char **end, 
                SLPAttributes slp_attr, 
                SLPBoolean *return_value, 
                int recursion_depth)
/* Parameters:                                                              */
/* start -- (IN) The start of the string to work in.                        */
/* end -- (OUT) The end of the string processed. After successful processing*/
/*              (ie, no PARSE_ERRORs), this will be pointed at the character*/
/*              following the last char in this level of the expression.    */
/* slp_attr -- (IN) The attributes handle to compare on.                    */
/* return_value -- (OUT) The result of the search. Only valid if SLP_OK was */
/*                       returned.                                          */
/*                                                                          */
/* Returns: SLP_OK on successful search (ie, the search was do-able).       */
/*          SLP_PARSE_ERROR on search error.  The end of the expression is  */
/*          returned through end.                                           */
/*--------------------------------------------------------------------------*/
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
    if (*end == 0)
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
	case('|'): /***** Or. *****/
		{
			SLPBoolean stop_condition; /* Stop when this value is returned. */
	
			if (*cur == '&') 
			{
				stop_condition = SLP_FALSE;
			}
			else if (*cur == '|')
			{
				stop_condition = SLP_TRUE;
			}
			
			cur++; /* Move past operator. */

			/*** Ensure that we have at least one operator. ***/
			if (*cur != BRACKET_OPEN) 
			{
				return SLP_PARSE_ERROR;
			}
			
			while(*cur == BRACKET_OPEN && cur < last_char) 
			{
	    	    err = filter(cur, &cur, slp_attr, &result, recursion_depth);
				/*** Propagate errors. ***/
	    	    if (err != SLP_OK)
    	    	{
	    	        return err;
	    	    }

				/*** Short circuit. ***/
				if (result == stop_condition) 
				{
					*return_value = stop_condition;
					return SLP_OK;
				}
			}
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

    default: /***** Unknown operator. *****/
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
        if (operator == 0)
        {
            /**** No search operator. ****/
            return SLP_PARSE_ERROR;
        }

        /* The rhs always follows the operator. (This doesn't really make 
        sense for PRESENT ops, but ignore that). */
        val_start = operator + 1; 
        
        /* Check for APPROX, GREATER, or LESS. Note that we shuffle the 
        operator pointer back to point at the start of the op. */
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
        if (lhs == 0)
        {
            return SLP_MEMORY_ALLOC_FAILED;
        }
        strncpy(lhs, cur, len);
        lhs[len] = '\0';

        /**** Right ****/
        len = last_char - val_start;
        rhs = (char *)malloc(len + 1);
        if (rhs == 0)
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
                /*** Since the PRESENT operation is the same for all types, 
                do that now. ***/
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
        if (rhs != 0)
        {
            free(rhs);
        }
        assert(err != SLP_NETWORK_INIT_FAILED);
        return err;
    }

    /***** No operator. *****/
    return SLP_PARSE_ERROR;
}


/*--------------------------------------------------------------------------*/
SLPError SLPAttrEvalPred(SLPAttributes slp_attr, 
                         SLPDPredicate predicate, 
                         SLPBoolean *result, 
                         int recursion_depth)

/* Test a predicate.                                                        */
/*                                                                          */
/* This is mostly a wrapper to filter(), but it does additional error       */
/* checking,  and puts error codes in terms of something a wee bit more     */
/* standard.                                                                */
/*                                                                          */
/* Parameters:                                                              */
/*                                                                          */
/*  recursion_depth -- dictates how many bracketed expressions are to be    */
/*  explored. It is decremented to zero, at which point parsing stops and   */
/*  a SLP_PARSE_ERROR is returned.                                          */
/*                                                                          */
/* Returns:                                                                 */
/*  SLP_OK -- If test was performed, the result (true or false) is returned */
/*      through the parameter result.                                       */
/*  SLP_PARSE_ERROR -- If there was a syntax error in the predicate, no     */
/*      result is returned through result.                                  */
/*  SLP_INTERNAL_SYSTEM_ERROR -- Something went wrong internally.           */
/*                                                                          */
/*--------------------------------------------------------------------------*/
{
    char *end; /* Pointer to the end of the parsed attribute string. */
    SLPError err;

    /***** An empty string is always true. *****/
    if (predicate == 0)
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
int SLPDTestPredicate(SLPDPredicate predicate, SLPAttributes attr) 
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
//	if (safe_pred == 0) {
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

/*=========================================================================*/
SLPError SLPDPredicateAlloc(const char *predicate_str,
                            size_t len,
                            SLPDPredicate *pred) 
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

    /* Temporary pointer for working with the under-construction predicate */
    /* "object".                                                           */
    char *new_pred;

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
        *pred = 0; /* Empty string. */
        return SLP_OK;
    }

    /***** Copy *****/
    real_len = end_cur - start_cur;
    new_pred = (char *)malloc(real_len + 1);

    if (new_pred == 0)
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


/*=========================================================================*/
void SLPDPredicateFree(SLPDPredicate *victim)
/* Free memory associated with the specified predicate                     */
/*                                                                         */
/* victim (IN) The predicate to free                                       */
/*                                                                         */
/* Returns: none                                                           */
/*=========================================================================*/
{
    if (victim != 0)
    {
        free(victim);
    }
    return;
}

#endif /* USE_PREDICATES */
