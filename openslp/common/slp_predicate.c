#include <string.h>
#include <stdio.h>
#include <fnmatch.h> 

#include "slp_predicate.h"
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

#ifdef DEBUG
void dumpAttrList(int level, const SLPAttrList *attrs)
{
    int i;
    
    if (SLP_IS_EMPTY(attrs) )
    {
        return;
    }

    for ( i = 0; i <= level; i++ ) 
    {
        printf("\t");
    }

    attrs = attrs->next;
    while ( !SLP_IS_HEAD(attrs) )
    {
        switch ( attrs->type )
        {
            case string:
                printf("%s = %s (string) \n", attrs->name, attrs->val.stringVal);
                break;
            case integer:
                printf("%s = %lu (integer) \n", attrs->name, attrs->val.intVal);
                break;
            case boolean:
                printf("%s = %s (boolean) \n", 
                       attrs->name, 
                       (attrs->val.boolVal ? " TRUE " : " FALSE "));
                break;
            case opaque:
            case head:
            default: 
                printf("%s = %s\n", attrs->name, "illegal or unknown attribute type");
                break;
        }
        attrs = attrs->next;
    }
    return;
}

void printOperator(int op)
{
    switch ( op )
    {
        case ldap_or: printf(" OR ");
            break;
        case ldap_and: printf( " AND " );
            break;
        case ldap_not: printf(" NOT ");
            break;
        case expr_eq: printf(" EQUAL ");
            break;
        case expr_gt: printf(" GREATER THAN ");
            break;
        case expr_lt: printf(" LESS THAN ");
            break;
        case expr_present: printf(" PRESENT ");
            break;
        case expr_approx: printf(" APPROX ");
            break;
        case -1: printf(" list head ");
            break;
        default:
            printf(" unknown operator value %i ", op);
            break;
    }
    return; 
}

void dumpFilterTree( const SLPLDAPFilter *filter )
{
    int i; 
    
    for ( i = 0; i < filter->nestingLevel; i++ )
    {
        printf("\t");
    }
    
    printOperator(filter->operator);
    
    printf("%s (level %i) \n", 
           (filter->logical_value ? " TRUE " : " FALSE "), 
           filter->nestingLevel  );

    dumpAttrList(filter->nestingLevel, &(filter->attrs));
    
    if ( !SLP_IS_EMPTY( &(filter->children) ) )
    {
        dumpFilterTree((SLPLDAPFilter *)filter->children.next ) ;
    }

    if ( (!SLP_IS_HEAD(filter->next)) && (!SLP_IS_EMPTY(filter->next)) )
    {
        dumpFilterTree(filter->next);
    }

    return;
}
#endif /* DEBUG*/

int SLPEvaluateOperation(int compare_result, int operation)
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
int SLPEvaluateAttributes(const SLPAttrList *a, const SLPAttrList *b, int op)
{
    /* first ensure they are the same type  */
    if ( a->type == b->type )
    {
        switch ( a->type )
        {
            case string:
                return( SLPEvaluateOperation(fnmatch(a->val.stringVal, b->val.stringVal, FNM_CASEFOLD), op) );
            case integer:
                return( SLPEvaluateOperation( a->val.intVal - b->val.intVal, op));
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
int SLPEvaluateFilterTree(SLPLDAPFilter *filter, const SLPAttrList *attrs)
{
    if ( !SLP_IS_EMPTY( &(filter->children) ) )
    {
        SLPEvaluateFilterTree((SLPLDAPFilter *)filter->children.next, attrs);
    }
    
    if ( ! (SLP_IS_HEAD(filter->next)) && (!SLP_IS_EMPTY(filter->next)) )
    {
        SLPEvaluateFilterTree(filter->next, attrs);
    }


    if ( filter->operator == ldap_and || 
         filter->operator == ldap_or || 
         filter->operator == ldap_not )
    {
        /* evaluate ldap logical operators by evaluating filter->children as a list of filters */

        SLPLDAPFilter *child_list = (SLPLDAPFilter *)filter->children.next;
        
        /* initialize  the filter's logical value to true */
        if ( filter->operator == ldap_or )
        {
            filter->logical_value = FALSE;
        }
        else
        {
            filter->logical_value = TRUE;
        }

        while ( !SLP_IS_HEAD(child_list) )
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
        if ( !SLP_IS_HEAD(filter->attrs.next) )
        {
            attrs = attrs->next;
            while ( (!SLP_IS_HEAD(attrs )) &&
                    ( FNM_NOMATCH == fnmatch(filter->attrs.next->name,
                                             attrs->name, FNM_CASEFOLD)) )
            {
                attrs = attrs->next ;
            }
            /* either we have traversed the list or found the first matching attribute */
            if ( !SLP_IS_HEAD(attrs) )
            {
                /* we found the first matching attribute, now do the comparison */
                if ( filter->operator == expr_present || 
                     filter->operator == expr_approx )
                {
                    filter->logical_value = TRUE;
                }
                else
                {
                    filter->logical_value = SLPEvaluateAttributes(filter->attrs.next, 
                                                                   attrs, filter->operator );
                }
            }
        }
    }
    return(filter->logical_value);
}

/* a is an attribute list, while b is a string representation of an ldap filter */
int SLP_predicate_match(const SLPAttrList *attrlist,
                         const char *filter)
{
    int ccode;
    SLPLDAPFilter *ftree;
    if ( filter == NULL || ! strlen(filter) )
    {
        return(TRUE);  /*  no predicate - aways tests TRUE  */
    }

    if ( NULL != (ftree = SLPDecodeLDAPFilter(filter)) )
    {
        ccode = SLPEvaluateFilterTree(ftree, attrlist);
        SLPFreeFilterTree(ftree);
        return(ccode);
    }

    return(FALSE);
}


