/*******************************************************************
 *  Description: encode/decode LDAP filters
 *
 *  Originated: 04-21-2001 
 *	Original Author: Mike Day - md@soft-hackle.net
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

#define yymaxdepth    slp_filter_maxdepth
#define yyparse       slp_filter_parse
#define yylex         slp_filter_lex
#define yyerror       slp_filter_error
#define yylval        slp_filter_lval
#define yychar        slp_filter_char
#define yydebug       slp_filter_debug
#define yypact        slp_filter_pact  
#define yyr1          slp_filter_r1                    
#define yyr2          slp_filter_r2                    
#define yydef         slp_filter_def           
#define yychk         slp_filter_chk           
#define yypgo         slp_filter_pgo           
#define yyact         slp_filter_act           
#define yyexca        slp_filter_exca
#define yyerrflag     slp_filter_errflag
#define yynerrs       slp_filter_nerrs
#define yyps          slp_filter_ps
#define yypv          slp_filter_pv
#define yys           slp_filter_s
#define yy_yys        slp_filter_yys
#define yystate       slp_filter_state
#define yytmp         slp_filter_tmp
#define yyv           slp_filter_v
#define yy_yyv        slp_filter_yyv
#define yyval         slp_filter_val
#define yylloc        slp_filter_lloc
#define yyreds        slp_filter_reds
#define yytoks        slp_filter_toks
#define yylhs         slp_filter_yylhs
#define yylen         slp_filter_yylen
#define yydefred      slp_filter_yydefred
#define yydgoto       slp_filter_yydgoto
#define yysindex      slp_filter_yysindex
#define yyrindex      slp_filter_yyrindex
#define yygindex      slp_filter_yygindex
#define yytable       slp_filter_yytable
#define yycheck       slp_filter_yycheck
#define yyname        slp_filter_yyname
#define yyrule        slp_filter_yyrule

#include "slp_filter.h"
#include "slp_linkedlist.h"

#include <unistd.h>
#include <string.h>
#include <stdlib.h>


#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE   (!FALSE)
#endif

/* prototypes and globals go here */

void slp_filter_close_lexer(unsigned int handle);
unsigned int slp_filter_init_lexer(const char *s);
int slp_filter_parse(void);
int slp_filter_parse(void);
void slp_filter_error(char *, ...);
int slp_filter_lex(void);

/* have a place to put attributes and the filter while the parser is working */
/* on them makes it easier to recover from parser errors - all the memory we  */
/* need to free is available from the list heads below.  */

/* listhead for reduced filters until the parser is finished */
static filterHead reducedFilters = { &reducedFilters, &reducedFilters, TRUE } ;
static int nesting_level;

%}

/* definitions for ytab.h */

%union {
  int filter_int;
  char *filter_string;
  SLPLDAPFilter *filter_filter;
}


%token<filter_int> L_PAREN R_PAREN OP_AND OP_OR OP_NOT OP_EQU OP_GT OP_LT OP_PRESENT OP_APPROX
%token<filter_int> VAL_INT VAL_BOOL 
%token<filter_string> OPERAND 

/* typecast the non-terminals */

%type <filter_filter> filter filter_list expression
%type <filter_int> exp_operator filter_op filter_open filter_close
%%

/* grammar */

filter_list: filter | filter_list filter ;

filter: filter_open filter_op filter_list filter_close 
{ 
    if(NULL != ($$ = SLPAllocFilter($2)))
    {
        $$->nestingLevel = nesting_level;
        if(!SLP_IS_EMPTY(&reducedFilters) )
        {
            SLPLDAPFilter *temp = (SLPLDAPFilter *)reducedFilters.next;
            while(!SLP_IS_HEAD(temp))
            {
                if(temp->nestingLevel == nesting_level + 1)
                {
                    SLPLDAPFilter *nest = temp;
                    temp = temp->next;
                   SLP_UNLINK(nest);
                   SLP_INSERT_BEFORE(nest, (SLPLDAPFilter *)&($$->children)) ;
                }
                else
                {
                    temp = temp->next;
                }
            }
           SLP_INSERT_BEFORE( (filterHead *)$$, &reducedFilters);
        }
        else
        {
            SLPFreeFilter($$) ; $$ = NULL ;
        }
    }
}
| filter_open expression filter_close 
{ 
    $$ = $2;
    if($2 != NULL)
    {
        $2->nestingLevel = nesting_level;
       SLP_INSERT_BEFORE((filterHead *)$2, &reducedFilters) ; 
    }
};

filter_open: L_PAREN 
{ 
    nesting_level++; 
};

filter_close: R_PAREN 
{ 
    nesting_level--; 
};

filter_op: OP_AND | OP_OR | OP_NOT
{ 
    $$ = yylval.filter_int; 
};

expression: OPERAND OP_PRESENT 
{      /* presence test binds to single operand */
    if(NULL != ($$ = SLPAllocFilter(expr_present)))
    {
        SLPAttrList *attr = SLPAllocAttr($1, string, "*", (int)strlen("*") + 1);
        if(attr != NULL)
        {
           SLP_INSERT(attr, &($$->attrs));
        }
        else
        {
            SLPFreeFilter($$); $$ = NULL;
        }
    }
}     

| OPERAND exp_operator VAL_INT  
{  /* must be an int or a bool */
    /* remember to touch up the token values to match the enum in SLP.h */
    if(NULL != ($$ = SLPAllocFilter($2)))
    {
        SLPAttrList *attr = SLPAllocAttr($1, integer, &($3), sizeof($3));
        if(attr != NULL)
        {
           SLP_INSERT(attr, &($$->attrs));
        }
        else
        {
            SLPFreeFilter($$); $$ = NULL ;
        } 
    }
}
| OPERAND exp_operator VAL_BOOL  
{  /* must be an int or a bool */
    /* remember to touch up the token values to match the enum in SLP.h */
    if(NULL != ($$ = SLPAllocFilter($2)))
    {
        SLPAttrList *attr = SLPAllocAttr($1, boolean, &($3), sizeof($3));
        if(attr != NULL)
        {
           SLP_INSERT(attr, &($$->attrs));
        }
        else
        {
            SLPFreeFilter($$); $$ = NULL ;
        } 
    }
}
| OPERAND exp_operator OPERAND  
{   /* both operands are strings */
    if(NULL != ($$ = SLPAllocFilter($2)))
    {
        SLPAttrList *attr = SLPAllocAttr($1, string, $3, (int)strlen($3) + 1 );
        if(attr != NULL)
        {
           SLP_INSERT(attr, &($$->attrs));
        }
        else
        {
            SLPFreeFilter($$); $$ = NULL ;
        } 
    }
};

exp_operator: OP_EQU | OP_GT | OP_LT | OP_APPROX
{ 
    $$ = yylval.filter_int; 
};

%% 



SLPLDAPFilter *SLPAllocFilter(int operator)
{
    SLPLDAPFilter *filter = (SLPLDAPFilter *)calloc(1, sizeof(SLPLDAPFilter));
    if ( filter  != NULL )
    {
        filter->next = filter->prev = filter;
        if ( operator == head )
        {
            filter->isHead = TRUE;
        }
        else
        {
            filter->children.next = filter->children.prev = &(filter->children);
            filter->children.isHead = 1;
            filter->attrs.next = filter->attrs.prev = &(filter->attrs);
            filter->attrs.isHead = 1;
            filter->operator = operator;
        }
    }
    return(filter);
}


void SLPFreeFilter(SLPLDAPFilter *filter)
{
    if ( filter->children.next != NULL )
    {
        while ( ! (SLP_IS_EMPTY((SLPLDAPFilter *)&(filter->children))) )
        {
            SLPLDAPFilter *child = (SLPLDAPFilter *)filter->children.next;
           SLP_UNLINK(child);
            SLPFreeFilter(child);
        }
    }
    if ( filter->attrs.next != NULL )
    {
        while ( ! (SLP_IS_EMPTY(&(filter->attrs))) )
        {
            SLPAttrList *attrs = filter->attrs.next;
           SLP_UNLINK(attrs);
            SLPFreeAttr(attrs);
        }
    }
}

void SLPFreeFilterList(SLPLDAPFilter *head, int static_flag)
{
    while ( ! (SLP_IS_EMPTY(head)) )
    {
        SLPLDAPFilter *temp = head->next;
       SLP_UNLINK(temp);
        SLPFreeFilter(temp);
    }

    if ( static_flag == TRUE )
        SLPFreeFilter(head);
    return;
}

void SLPFreeFilterTree(SLPLDAPFilter *root)
{
    if ( !SLP_IS_EMPTY( &(root->children) ) )
    {
        SLPFreeFilterTree((SLPLDAPFilter *)root->children.next);
    }
    if ( ! (SLP_IS_HEAD(root->next)) && (!SLP_IS_EMPTY(root->next)) )
    {
        SLPFreeFilterTree(root->next);
    }

    if ( root->attrs.next != NULL )
    {
        while ( ! (SLP_IS_EMPTY(&(root->attrs))) )
        {
            SLPAttrList *attrs = root->attrs.next;
           SLP_UNLINK(attrs);
            SLPFreeAttr(attrs);
        }
    }
}

void SLPInitFilterList(void )
{
    reducedFilters.next = reducedFilters.prev = &reducedFilters;
    reducedFilters.isHead = TRUE;
    return;
}

void SLPCleanUpFilterList(void)
{
    SLPFreeFilterList( (SLPLDAPFilter *)&reducedFilters, FALSE);
}

SLPLDAPFilter *SLPDecodeLDAPFilter(const char *filter)
{
    SLPLDAPFilter *temp = NULL;
    unsigned int lexer = 0;

    SLPInitFilterList();
    nesting_level = 1;
    if ( 0 != (lexer = slp_filter_init_lexer(filter)) )
    {
        if ( slp_filter_parse() )
        {
            SLPCleanUpFilterList();
        }
        slp_filter_close_lexer(lexer);
    }
    if ( !SLP_IS_EMPTY(&reducedFilters) )
    {
        if ( NULL != (temp  = SLPAllocFilter(ldap_and)) )
        {
           SLP_LINK_HEAD(&(temp->children), &reducedFilters);
        }
    }
    SLPCleanUpFilterList();


    return(temp);
}
