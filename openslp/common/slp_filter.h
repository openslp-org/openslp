/*******************************************************************
 *  Description: encode/decode LDAP filters
 *
 *  Originated: 04-21-2001 
 *	Original Author: Mike Day - md@soft-hackle.net
 *  Project: 
 *
 *  $Header$
 *
 *  Copyright (C) Michael Day, 2001 
 *
 *  This program is free software; you can redistribute it and/or 
 *  modify it under the terms of the GNU General Public License 
 *  as published by the Free Software Foundation; either version 2 
 *  of the License, or (at your option) any later version. 
 *
 *  This program is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 *  GNU General Public License for more details. 
 *
 *  You should have received a copy of the GNU General Public License 
 *  along with this program; if not, write to the Free Software 
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. 

 *******************************************************************/
#ifndef SLP_FILTER_H_INCLUDED
#define SLP_FILTER_H_INCLUDED

#include "slp_attr.h"

typedef enum ldap_operator_types
{
    ldap_and = 259,    /* to match token values assigned in y_filter.h */
    ldap_or,
    ldap_not,
    expr_eq,
    expr_gt,
    expr_lt,
    expr_present,
    expr_approx
} ldapOperatorTypes;

typedef struct ldap_filter_struct_head
{
    struct ldap_filter_struct_head  *next;
    struct ldap_filter_struct_head  *prev;
    int isHead;
    int operator; 
} filterHead;

typedef struct ldap_filter_struct
{
    struct ldap_filter_struct *next;
    struct ldap_filter_struct *prev;
    int isHead;
    int operator; 
    int nestingLevel;
    int logical_value;
    filterHead children;
    lslpAttrList attrs;
} lslpLDAPFilter;

lslpLDAPFilter *lslpAllocFilter(int operator);
void lslpFreeFilter(lslpLDAPFilter *filter);
void lslpFreeFilterList(lslpLDAPFilter *head, int static_flag);
void lslpFreeFilterTree(lslpLDAPFilter *root);

/*prototypes */
lslpLDAPFilter *lslpDecodeLDAPFilter(const char *filter) ;

#endif /* _LSLPDEFS_INCLUDE */

