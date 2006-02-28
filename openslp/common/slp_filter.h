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
 *
 * Originated: 04-21-2001 
 * Copyright (C) Michael Day, 2001 
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either version 2 
 * of the License, or (at your option) any later version. 
 *
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details. 
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  
 * 02111-1307, USA. 
 *-------------------------------------------------------------------------*/

/** Header file for encoding and decoding LDAP filters.
 *
 * @file       slp_filter.h
 * @author     Matthew Peterson, Michael Day (md@soft-hackle.net), 
 *             John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeFilter
 */

#ifndef SLP_FILTER_H_INCLUDED
#define SLP_FILTER_H_INCLUDED

/*!@defgroup CommonCodeFilter LDAPv3 Filter
 * @ingroup CommonCode
 * @{
 */

#include "slp_attr.h"

/** LDAP operator types */
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

/** LDAP filter structure head pointer */
typedef struct ldap_filter_struct_head
{
   struct ldap_filter_struct_head  * next;
   struct ldap_filter_struct_head  * prev;
   int isHead;
   int operator; 
} filterHead;

/** LDAP filter structure */
typedef struct ldap_filter_struct
{
   struct ldap_filter_struct * next;
   struct ldap_filter_struct * prev;
   int isHead;
   int operator; 
   int nestingLevel;
   int logical_value;
   filterHead children;
   SLPAttrList attrs;
} SLPLDAPFilter;

SLPLDAPFilter * SLPAllocFilter(int operator);

void SLPFreeFilter(SLPLDAPFilter * filter);

void SLPFreeFilterList(SLPLDAPFilter * head, int static_flag);

void SLPFreeFilterTree(SLPLDAPFilter * root);

SLPLDAPFilter * SLPDecodeLDAPFilter(const char * filter);

/*! @} */

#endif   /* SLP_FILTER_H_INCLUDED */

/*=========================================================================*/
