/*
 *  Abstract: Does wildcard comparison of strings. 
 */

#include <string.h>


#ifdef WIN32
    #define strncasecmp(String1, String2, Num) strnicmp(String1, String2, Num)
    #define strcasecmp(String1, String2, Num) stricmp(String1, String2, Num)
#endif

/* The character that is a wildcard. */
#define WILDCARD ('*')

/* Does a case insensitive substring match for needle in haystack. 
 * 
 * Returns pointer to the start of the substring. NULL if the substring is 
 * not found. 
 * FIXME This implementation isn't exactly blazingly fast...
 */
const char *substr(const char *haystack, const char *needle, size_t needle_len)
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


/* Returns
 * -1 error
 * 0 success
 * 1 failure
 */
int wildcard(const char *pattern, const char *string)
{
    int i;
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
            const char *result, *end;
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
