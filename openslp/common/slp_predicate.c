#include <string.h>
#include <fnmatch.h>  

#include "slp_predicate.h"
#include "slp_filter.h"
#include "slp_linkedlist.h"


#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE   (!FALSE)
#endif
 
/* for those platforms without FNM_CASEFOLD */
#ifndef FNM_CASEFOLD
#define FNM_CASEFOLD 0
#endif

/* the usual */
#define SLP_MIN(a, b) ((a) < (b) ? (a) : (b))
#define SLP_MAX(a, b) ((a) > (b) ? (a) : (b))


int lslpEvaluateOperation(int compare_result, int operation)
{
    switch ( operation )
    {
        case expr_eq:
            if ( compare_result == 0 )     /*  a == b */
                return(TRUE);
            break;
        case expr_gt:
            if ( compare_result >= 0 )     /*  a >= b  */
                return(TRUE);
            break;

        case expr_lt:                 /* a <= b  */
            if ( compare_result <= 0 )
                return(TRUE);
            break;
        case expr_present:
        case expr_approx:
        default:
            return(TRUE);
            break;
    }

    return(FALSE);
}

/* evaluates attr values, not names */
int lslpEvaluateAttributes(const lslpAttrList *a, const lslpAttrList *b, int op)
{
    /* first ensure they are the same type  */
    if ( a->type == b->type )
    {
        switch ( a->type )
        {
            case string:
                return( lslpEvaluateOperation(fnmatch(a->val.stringVal, b->val.stringVal, FNM_CASEFOLD), op) );
            case integer:
                return( lslpEvaluateOperation( a->val.intVal - b->val.intVal, op));
            case tag:                   /* equivalent to a presence test  */
                return(TRUE);
            case boolean:
                if ( (a->val.boolVal != 0) && (b->val.boolVal != 0) )
                    return(TRUE);
                if ( (a->val.boolVal == 0) && (b->val.boolVal == 0) )
                    return(TRUE);
                break;
            case opaque:
                if ( ! memcmp((((char *)(a->val.opaqueVal)) + 4),
                              (((char *)(b->val.opaqueVal)) + 4),
                              SLP_MIN((*((int *)a->val.opaqueVal)), (*((int *)a->val.opaqueVal)))) );
                return(TRUE);
                break;
            default:
                break;
        }
    }
    return(FALSE);

}


/* filter is a filter tree, attrs is ptr to an attr listhead */
int lslpEvaluateFilterTree(lslpLDAPFilter *filter, const lslpAttrList *attrs)
{
    if ( ! _LSLP_IS_EMPTY( &(filter->children) ) )
    {
        lslpEvaluateFilterTree((lslpLDAPFilter *)filter->children.next, attrs);
    }
    
    if ( ! (_LSLP_IS_HEAD(filter->next)) && (! _LSLP_IS_EMPTY(filter->next)) )
    {
        lslpEvaluateFilterTree(filter->next, attrs);
    }


    if ( filter->operator == ldap_and || 
         filter->operator == ldap_or || 
         filter->operator == ldap_not )
    {
        /* evaluate ldap logical operators by evaluating filter->children as a list of filters */

        lslpLDAPFilter *child_list = (lslpLDAPFilter *)filter->children.next;
        
        /* initialize  the filter's logical value to true */
        if ( filter->operator == ldap_or )
        {
            filter->logical_value = FALSE;
        }
        else
        {
            filter->logical_value = TRUE;
        }

        while ( ! _LSLP_IS_HEAD(child_list) )
        {
            if ( child_list->logical_value == TRUE )
            {
                if ( filter->operator == ldap_or )
                {
                    filter->logical_value = TRUE;
                    break;
                }
                if ( filter->operator == ldap_not )
                {
                    filter->logical_value = FALSE;
                    break;
                }
                /* for an & operator keep going  */
            }
            else
            {
                /* child is false */
                if ( filter->operator == ldap_and )
                {
                    filter->logical_value = FALSE;
                    break;
                }
            }
            child_list = child_list->next;
        }
    }
    else
    {
        /* find the first matching attribute and set the logical value */
        filter->logical_value = FALSE;
        if ( ! _LSLP_IS_HEAD(filter->attrs.next) )
        {
            attrs = attrs->next;
            while ( (! _LSLP_IS_HEAD(attrs )) &&
                    ( FNM_NOMATCH == fnmatch(filter->attrs.next->name,
                                             attrs->name, FNM_CASEFOLD)) )
            {
                attrs = attrs->next ;
            }
            /* either we have traversed the list or found the first matching attribute */
            if ( ! _LSLP_IS_HEAD(attrs) )
            {
                /* we found the first matching attribute, now do the comparison */
                if ( filter->operator == expr_present || 
                     filter->operator == expr_approx )
                {
                    filter->logical_value = TRUE;
                }
                else
                {
                    filter->logical_value = lslpEvaluateAttributes(filter->attrs.next, 
                                                                   attrs, filter->operator );
                }
            }
        }
    }
    return(filter->logical_value);
}

/* a is an attribute list, while b is a string representation of an ldap filter */
int lslp_predicate_match(const lslpAttrList *attrlist,
                         const char *filter)
{
    int ccode;
    lslpLDAPFilter *ftree;
    if ( filter == NULL || ! strlen(filter) )
    {
        return(TRUE);               /*  no predicate - aways tests TRUE  */
    }

    if ( NULL != (ftree = lslpDecodeLDAPFilter(filter)) )
    {
        ccode = lslpEvaluateFilterTree(ftree, attrlist);
        lslpFreeFilterTree(ftree);
        return(ccode);
    }

    return(FALSE);
}


