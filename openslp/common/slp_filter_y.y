/*-------------------------------------------------------------------------
 * Copyright (C) 2000-2005 Mike Day <ncmike@ncultra.org>
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

/** Bison parser for encoding and decoding LDAP filters.
 *
 * @file       slp_filter_y.y
 * @author     Michael Day (ncmike@ncultra.org), 
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeFilter
 */

%{

#include "slp_client.h"

/* prototypes and globals go here */
  void filtererror(int8 *, ...);
  
  int32 filterlex(void);
  
int32 filterparse(void);
void filter_close_lexer(uint32 handle);
uint32 filter_init_lexer(int8 *s);

/* have a place to put attributes and the filter while the parser is working */
/* on them makes it easier to recover from parser errors - all the memory we  */
/* need to free is available from the list heads below.  */

/* listhead for reduced filters until the parser is finished */
 filterHead reducedFilters = { &reducedFilters, &reducedFilters, TRUE } ;
 int nesting_level;

%}

%name-prefix="filter" 
/* definitions for ytab.h */

%union {
  int32 filter_int;
  int8 *filter_string;
  lslpLDAPFilter *filter_filter;
}


%token<filter_int> L_PAREN 258 R_PAREN 259 OP_AND 260 OP_OR 261 OP_NOT 262 OP_EQU 263 OP_GT 264 OP_LT 265 OP_PRESENT 266 OP_APPROX 267
%token<filter_int> VAL_INT 270 VAL_BOOL 271  
%token<filter_string> OPERAND 280

/* typecast the non-terminals */

%type <filter_filter> filter filter_list expression
%type <filter_int> exp_operator filter_op filter_open filter_close
%%

/* grammar */

filter_list: filter 
         | filter_list filter 
         ;

filter: filter_open filter_op filter_list filter_close { 
            if(NULL != ($$ = lslpAllocFilter($2))) {
	      $$->nestingLevel = nesting_level;
	      if(! _LSLP_IS_EMPTY(&reducedFilters) ) { 
		lslpLDAPFilter *temp = (lslpLDAPFilter *)reducedFilters.next;
		while(! _LSLP_IS_HEAD(temp)) {
		  if(temp->nestingLevel == nesting_level + 1) {
		    lslpLDAPFilter *nest = temp;
		    temp = temp->next;
		    _LSLP_UNLINK(nest);
		    _LSLP_INSERT_BEFORE(nest, (lslpLDAPFilter *)&($$->children)) ;
		  } else {temp = temp->next; }
		}
		_LSLP_INSERT_BEFORE( (filterHead *)$$, &reducedFilters);
	      } else { lslpFreeFilter($$) ; $$ = NULL ; }
            }
         }
       
         | filter_open expression filter_close { 
	   $$ = $2;
	   if($2 != NULL) {
	     $2->nestingLevel = nesting_level;
	     _LSLP_INSERT_BEFORE((filterHead *)$2, &reducedFilters) ; 
	   }
	 }
         ;

filter_open: L_PAREN { nesting_level++; } 
         ;

filter_close: R_PAREN { nesting_level--; }
         ;

filter_op: OP_AND
         | OP_OR
         | OP_NOT
         { $$ = filterlval.filter_int; }

         ;

expression: OPERAND OP_PRESENT {      /* presence test binds to single operand */
             if(NULL != ($$ = lslpAllocFilter(expr_present))) {
	       lslpAttrList *attr = lslpAllocAttr($1, string, "*", (int16)strlen("*") + 1);
	       if(attr != NULL) {
		 _LSLP_INSERT(attr, &($$->attrs));
	       } else { lslpFreeFilter($$); $$ = NULL; }
	     }
         }     

         | OPERAND exp_operator VAL_INT  {  /* must be an int or a bool */
	   /* remember to touch up the token values to match the enum in lslp.h */
	   if(NULL != ($$ = lslpAllocFilter($2))) {
	     lslpAttrList *attr = lslpAllocAttr($1, integer, &($3), sizeof($3));
	     if(attr != NULL) {
	       _LSLP_INSERT(attr, &($$->attrs));
	     } else { lslpFreeFilter($$); $$ = NULL ; } 
	   }
	 }

         | OPERAND exp_operator VAL_BOOL  {  /* must be an int or a bool */
	   /* remember to touch up the token values to match the enum in lslp.h */
	   if(NULL != ($$ = lslpAllocFilter($2))) {
	     lslpAttrList *attr = lslpAllocAttr($1, bool_type, &($3), sizeof($3));
	     if(attr != NULL) {
	       _LSLP_INSERT(attr, &($$->attrs));
	     } else { lslpFreeFilter($$); $$ = NULL ; } 
	   }
	 }

         | OPERAND exp_operator OPERAND  {   /* both operands are strings */
	   if(NULL != ($$ = lslpAllocFilter($2))) {
	     lslpAttrList *attr = lslpAllocAttr($1, string, $3, (int16)strlen($3) + 1 );
	     if(attr != NULL) {
	       _LSLP_INSERT(attr, &($$->attrs));
	     } else { lslpFreeFilter($$); $$ = NULL ; } 
	   }
	 }

         ;

exp_operator: OP_EQU
         | OP_GT
         | OP_LT
         | OP_APPROX
         { $$ = filterlval.filter_int; }
         ;

%% 


lslpLDAPFilter *lslpAllocFilter(int operator)
{
  lslpLDAPFilter *filter = (lslpLDAPFilter *)calloc(1, sizeof(lslpLDAPFilter));
  if(filter  != NULL) {
    filter->next = filter->prev = filter;
    if(operator == head) {
      filter->isHead = TRUE;
    } else {
      filter->children.next = filter->children.prev = &(filter->children);
      filter->children.isHead = 1;
      filter->attrs.next = filter->attrs.prev = &(filter->attrs);
      filter->attrs.isHead = 1;
      filter->ldap_operator = operator;
    }
  }
  return(filter);
}


void lslpFreeFilter(lslpLDAPFilter *filter)
{
  if(filter->children.next != NULL) {
    while(! (_LSLP_IS_EMPTY((lslpLDAPFilter *)&(filter->children)))) {
      lslpLDAPFilter *child = (lslpLDAPFilter *)filter->children.next;
      _LSLP_UNLINK(child);
      lslpFreeFilter(child);
    }
  }
  if(filter->attrs.next != NULL) {
    while(! (_LSLP_IS_EMPTY(&(filter->attrs)))) {
      lslpAttrList *attrs = filter->attrs.next;
      _LSLP_UNLINK(attrs);
      lslpFreeAttr(attrs);
    }
  }
}

void lslpFreeFilterList(lslpLDAPFilter *head, BOOL static_flag)
{
  assert((head != NULL) && (_LSLP_IS_HEAD(head)));
  while(! (_LSLP_IS_EMPTY(head))) {
    lslpLDAPFilter *temp = head->next;
    _LSLP_UNLINK(temp);
    lslpFreeFilter(temp);
  }
  
  if( static_flag == TRUE)
    lslpFreeFilter(head);
  return;
}

void lslpInitFilterList(void )
{
  reducedFilters.next = reducedFilters.prev = &reducedFilters;
  reducedFilters.isHead = TRUE;
  return;
}

void lslpCleanUpFilterList(void)
{
  lslpFreeFilterList( (lslpLDAPFilter *)&reducedFilters, FALSE);
}

lslpLDAPFilter *_lslpDecodeLDAPFilter(int8 *filter)
{

  lslpLDAPFilter *temp = NULL;
  uint32 lexer = 0;
  assert(filter != NULL && strlen(filter));
  lslpInitFilterList();
  nesting_level = 1;
  if(0 != (lexer = filter_init_lexer(filter))) {
    if(filterparse()) { lslpCleanUpFilterList(); }
    filter_close_lexer(lexer);
  }
  if (! _LSLP_IS_EMPTY(&reducedFilters)) {
    if(NULL != (temp  = lslpAllocFilter(ldap_and))) {
      _LSLP_LINK_HEAD(&(temp->children), &reducedFilters);
    }
  }
  lslpCleanUpFilterList();
  
  return(temp);	
}
