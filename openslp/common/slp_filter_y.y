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

%{

#include "../../lslp-common/lslp-common-defs.h"

/* prototypes and globals go here */


#ifndef TEST_HARNESS
SUBALLOC_HANDLE *filter_sa;
#ifndef _init_heap
#ifdef DEBUG_ALLOC

#define _init_heap(x)  filter_sa = InitializeProcessHeap((x));
#define malloc(x) filter_sa->malloc((x), (uint32)filter_sa, __FILE__, __LINE__)
#define calloc(x, y) filter_sa->calloc((x), (y), (uint32)filter_sa, __FILE__, __LINE__)
#define realloc(x, y) filter_sa->realloc((x), (y), (uint32)filter_sa, __FILE__, __LINE__)
#define strdup(x) filter_sa->strdup((x), (uint32)filter_sa, __FILE__, __LINE__)
#define wcsdup(x) filter_sa->wcsdup((x), (uint32)filter_sa, __FILE__, __LINE__)
#define _de_init() filter_sa->de_init((int32)filter_sa)

#else

#define _init_heap(x)  filter_sa = InitializeProcessHeap((x));
#define malloc(x) filter_sa->malloc((x), (uint32)filter_sa)
#define calloc(x, y) filter_sa->calloc((x), (y), (uint32)filter_sa)
#define realloc(x, y) filter_sa->realloc((x), (y), (uint32)filter_sa)
#define strdup(x) filter_sa->strdup((x), (uint32)filter_sa)
#define wcsdup(x) filter_sa->wcsdup((x), (uint32)filter_sa)
#define _de_init() filter_sa->de_init((int32)filter_sa)

#endif

#endif

#else
#define vs_free(x) free(x)
#endif

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

/* definitions for ytab.h */

%union {
  int32 filter_int;
  int8 *filter_string;
  lslpLDAPFilter *filter_filter;
}


%token<filter_int> L_PAREN R_PAREN OP_AND OP_OR OP_NOT OP_EQU OP_GT OP_LT OP_PRESENT OP_APPROX
%token<filter_int> VAL_INT VAL_BOOL 
%token<filter_string> OPERAND 

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
	     lslpAttrList *attr = lslpAllocAttr($1, bool, &($3), sizeof($3));
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



#ifdef TEST_HARNESS

lslpAttrList *lslpAllocAttr(int8 *name, int8 type, void *val, int16 len)
{	
  lslpAttrList *attr;
  if (NULL != (attr = (lslpAttrList *)calloc(1, sizeof(lslpAttrList))))
    {
      if (name != NULL)
	{
	  if (NULL == (attr->name = strdup(name)))
	    {
	      free(attr);
	      return(NULL);
	    }
	}
      attr->type = type;
      if (type == head)	/* listhead */
	return(attr);
      if (val != NULL)
	{
	  switch (type)	    {
	    case string:
	      if ( NULL != (attr->val.stringVal = strdup((uint8 *)val)))
		return(attr);
	      break;
	    case integer:
	      attr->val.intVal = *(uint32 *)val;
	      break;
	    case bool:
	      attr->val.boolVal = *(BOOL *)val;
	      break;
	    default:
	      break;
	    }
	}
    }
  return(attr);
}	

lslpAttrList *lslpAllocAttrList(void)
{
  lslpAttrList *temp;
  if (NULL != (temp = lslpAllocAttr(NULL, head, NULL, 0)))
    {
      temp->next = temp->prev = temp;
      temp->isHead = TRUE;	
    }
  return(temp);
}	

/* attr MUST be unlinked from its list ! */
void lslpFreeAttr(lslpAttrList *attr)
{
  assert(attr != NULL);
  if (attr->name != NULL)
    free(attr->name);
  if (attr->type == string && attr->val.stringVal != NULL)
    free(attr->val.stringVal);
  else if (attr->type == opaque && attr->val.opaqueVal != NULL)
    free(attr->val.opaqueVal);
  free(attr);
}	

void lslpFreeAttrList(lslpAttrList *list, BOOL staticFlag)
{
  lslpAttrList *temp;

  assert(list != NULL);
  assert(_LSLP_IS_HEAD(list));
  while(! (_LSLP_IS_EMPTY(list)))
    {
      temp = list->next;
      _LSLP_UNLINK(temp);
      lslpFreeAttr(temp);
    }
  if(staticFlag == TRUE)
    lslpFreeAttr(list);
  return;
	
}	

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
      filter->operator = operator;
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

void dumpAttrList(int level, lslpAttrList *attrs)
{
  int i;
  if(_LSLP_IS_EMPTY(attrs)) { return; }
  for(i = 0; i <= level; i++) printf("\t");
  attrs = attrs->next;
  while( ! _LSLP_IS_HEAD(attrs)) {
    switch(attrs->type) {
    case string:
      printf("%s = %s (string) \n", attrs->name, attrs->val.stringVal);
      break;
    case integer:
      printf("%s = %i (integer) \n", attrs->name, attrs->val.intVal);
      break;
    case bool:
      printf("%s = %s (boolean) \n", attrs->name, (attrs->val.boolVal ? " TRUE " : " FALSE "));
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
  switch(op) {
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

void dumpFilterTree( lslpLDAPFilter *filter )
{
  int i; 
  assert(! _LSLP_IS_HEAD(filter));
  for(i = 0; i < filter->nestingLevel; i++)
    printf("\t");
  printOperator(filter->operator);
  printf("%s (level %i) \n", (filter->logical_value ? " TRUE " : " FALSE "), filter->nestingLevel  );
  dumpAttrList(filter->nestingLevel, &(filter->attrs));
  if( ! _LSLP_IS_EMPTY( &(filter->children) ) ) {
    dumpFilterTree((lslpLDAPFilter *)filter->children.next ) ;
  }
  if( (! _LSLP_IS_HEAD(filter->next)) && (! _LSLP_IS_EMPTY(filter->next))) 
    dumpFilterTree(filter->next);
  return;
}


#endif /* test harness */
/* real code for the next few functions ! */





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
#ifndef TEST_HARNESS
  extern LSLP_SEM_T filterParseSem;
#endif
  lslpLDAPFilter *temp = NULL;
  uint32 lexer = 0;
  int32 ccode = LSLP_WAIT_OK;
  assert(filter != NULL && strlen(filter));
#ifndef TEST_HARNESS
  _LSLP_WAIT_SEM(filterParseSem, 10000, &ccode );
  assert(ccode == LSLP_WAIT_OK);
#endif
  if(ccode == LSLP_WAIT_OK) {
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
#ifndef TEST_HARNESS
    _LSLP_SIGNAL_SEM(filterParseSem);
#endif
  }
  return(temp);	
}


#ifdef TEST_HARNESS
/* test harness */

#define NUMBER_FILTERS 7
uint8 *filters[] = 
{
	"(one=1)",
	"(one = 01)(two = 0x02)(four = TRUE)(five = +0x05)(six = +06)",
	"(&(nothing =*)(anotherthing = nothing*))",	
	"(one = 1)(two= 2)(!(five = 5))",
	" (!         (|        (& (one = 0xfe )( a = bee )) (three = -45 ) ) )    ",
 	"(& (one = food) (&(two = nothing) (two = another)) (three = -067) (|(a=* )(c = d)(e = f)) (!(a=*) (&(b=*)(c=FALSE)(d=*))   )  )",
	"(&(one >= 2) (three <= 4))",
	"ftp://ftp.isi.edu/in-notes/rfc1700.txt",
	"ftp://ftp.isi.edu/in-notes/rfc1700.txt",
	"error  this is really a #### bad url",
	"", ""
};



int32 main(int32 argc, int8 *argv[]) 
{
  int i;
  lslpLDAPFilter *filter;

  for(i = 0; i < NUMBER_FILTERS; i++) {
    
    printf("input filter: %s\n", filters[i]);
    if(NULL != (filter = _lslpDecodeLDAPFilter(filters[i]))) {
      dumpFilterTree( filter );
      lslpFreeFilter( filter );
    }
  }
  return(0);		
}	

#endif

