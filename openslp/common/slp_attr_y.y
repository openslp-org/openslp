/*******************************************************************
 *  Description: encode/decode attribute lists
 *
 *  Originated: 03-06-2000 
 *	Original Author: Mike Day - md@soft-hackle.net
 *
 *  Copyright (C) Michael Day, 2000-2001 
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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "slp_attr.h"
#include "slp_linkedlist.h"



/* prototypes and globals go here */

#ifndef FALSE
#define FALSE   0
#endif
#ifndef TRUE
#define TRUE    (!FALSE)
#endif

#define yymaxdepth      slp_attr_maxdepth
#define yyparse         slp_attr_parse
#define yylex           slp_attr_lex
#define yyerror         slp_attr_error
#define yylval          slp_attr_lval
#define yychar          slp_attr_char
#define yydebug         slp_attr_debug
#define yypact          slp_attr_pact  
#define yyr1            slp_attr_r1                    
#define yyr2            slp_attr_r2                    
#define yydef           slp_attr_def           
#define yychk           slp_attr_chk           
#define yypgo           slp_attr_pgo           
#define yyact           slp_attr_act           
#define yyexca          slp_attr_exca
#define yyerrflag       slp_attr_errflag
#define yynerrs         slp_attr_nerrs
#define yyps            slp_attr_ps
#define yypv            slp_attr_pv
#define yys             slp_attr_s
#define yy_yys          slp_attr_yys
#define yystate         slp_attr_state
#define yytmp           slp_attr_tmp
#define yyv             slp_attr_v
#define yy_yyv          slp_attr_yyv
#define yyval           slp_attr_val
#define yylloc          slp_attr_lloc
#define yyreds          slp_attr_reds
#define yytoks          slp_attr_toks
#define yylhs           slp_attr_yylhs
#define yylen           slp_attr_yylen
#define yydefred        slp_attr_yydefred
#define yydgoto         slp_attr_yydgoto
#define yysindex        slp_attr_yysindex
#define yyrindex        slp_attr_yyrindex
#define yygindex        slp_attr_yygindex
#define yytable         slp_attr_yytable
#define yycheck         slp_attr_yycheck
#define yyname          slp_attr_yyname
#define yyrule          slp_attr_yyrule

static int bt = TRUE;
static int bf = FALSE;
static SLPAttrList attrHead = {&attrHead, &attrHead, TRUE, NULL, head, {0L}};
static SLPAttrList inProcessAttr = {&inProcessAttr, &inProcessAttr, TRUE, NULL, head, {0L}};
static SLPAttrList inProcessTag = {&inProcessTag, &inProcessTag, TRUE, NULL, head, {0L}};

int slp_attr_parse(void);
void slp_attr_error(char *, ...);
int slp_attr_wrap(void);
int slp_attr_lex(void);
void slp_attr_close_lexer(unsigned int handle);
unsigned int slp_attr_init_lexer(char *s);


%}

/* definitions for ytab.h */

%union 
{
    int _i;
    char *_s;
    SLPAttrList *_atl;
}

%token<_i> _TRUE _FALSE _MULTIVAL _INT 
%token<_s> _ESCAPED _TAG _STRING 

/* typecast the non-terminals */

/* %type <_i> */
%type <_atl> attr_list attr attr_val_list attr_val

%%

attr_list: attr 
{
    while ( !SLP_IS_HEAD(inProcessAttr.next) )
    {
        $$ = inProcessAttr.next;
       SLP_UNLINK($$);
       SLP_INSERT_BEFORE($$, &attrHead);
    }
    /* all we really want to do here is link each attribute */
    /* to the global list head. */
}
| attr_list ',' attr 
{
    /* both of these non-terminals are really lists */
    /* ignore the first non-terminal */
    while ( !SLP_IS_HEAD(inProcessAttr.next) )
    {
        $$ = inProcessAttr.next;
       SLP_UNLINK($$);
       SLP_INSERT_BEFORE($$, &attrHead);
    }
};

attr: _TAG
{
    $$ =  SLPAllocAttr($1, tag, NULL, 0);
    if ( NULL != $$ )
    {
       SLP_INSERT_BEFORE($$, &inProcessAttr);
    }
}
| '(' _TAG ')' 	
{
    $$ =  SLPAllocAttr($2, tag, NULL, 0);
    if (NULL != $$)
    {
       SLP_INSERT_BEFORE($$, &inProcessAttr);
    }
}
| '(' _TAG '=' attr_val_list ')' 
{
    $$ = inProcessTag.next;
    while (!SLP_IS_HEAD($$))
    {
        $$->name = strdup($2); 
	SLP_UNLINK($$);
	SLP_INSERT_BEFORE($$, &inProcessAttr);
	$$ = inProcessTag.next;
    }
};

attr_val_list: attr_val 
{
    if(NULL != $1)
    {
       SLP_INSERT($1, &inProcessTag);
    }
}
| attr_val_list _MULTIVAL attr_val 
{
    if (NULL != $3)
    {
       SLP_INSERT_BEFORE($3, &inProcessTag);
    }
};

attr_val: _TRUE 
{
    $$ = SLPAllocAttr(NULL, boolean, &bt, sizeof(int));
}
| _FALSE 
{
    $$ = SLPAllocAttr(NULL, boolean, &bf, sizeof(int));
}
| _ESCAPED 
{ 
    /* treat it as a string because it is already encoded */
    $$ = SLPAllocAttr(NULL, string, $1, strlen($1) + 1);
}
| _STRING 
{    
    $$ = SLPAllocAttr(NULL, string, $1, strlen($1) + 1);
}
| _INT
{
    $$ = SLPAllocAttr(NULL, integer, &($1), sizeof(int));
};
 
       
%%

SLPAttrList *SLPAllocAttr(char *name, SLPTypes type, void *val, int len)
{
    SLPAttrList *attr;
    if ( NULL != (attr = (SLPAttrList *)calloc(1, sizeof(SLPAttrList))) )
    {
        if ( name != NULL )
        {
            if ( NULL == (attr->name = strdup(name)) )
            {
                free(attr);
                return(NULL);
            }
        }
        attr->type = type;
        if ( type == head ) /* listhead */
            return(attr);
        if ( val != NULL )
        {
            switch ( type )
            {
                case string:
                    if ( NULL != (attr->val.stringVal = strdup((unsigned char *)val)) )
                        return(attr);
                    break;
                case integer:
                    attr->val.intVal = *(unsigned int *)val;
                    break;
                case boolean:
                    attr->val.boolVal = *(int *)val;
                    break;
                case opaque:
                    #if 0
                    if ( len > 0 )
                    {
                        int encLen;
                        opq_EncodeOpaque(val, len, (char **)&(attr->val.opaqueVal), &encLen);
                        if ( NULL != attr->val.opaqueVal )
                        {
                            /* first two bytes contain length of attribute */
                           SLP_SETSHORT(((char *)attr->val.opaqueVal), encLen, 0 );
                        }
                    }
                    #endif
                    break;
                default:
                    break;
            }
        }
    }
    return(attr);
}   

SLPAttrList *SLPAllocAttrList(void)
{
    SLPAttrList *temp;
    if ( NULL != (temp = SLPAllocAttr(NULL, head, NULL, 0)) )
    {
        temp->next = temp->prev = temp;
        temp->isHead = TRUE;  
    }
    return(temp);
}   

/* attr MUST be unlinked from its list ! */
void SLPFreeAttr(SLPAttrList *attr)
{
    if ( attr->name != NULL )
    {
        free(attr->name);
    }
    if ( attr->type == string && attr->val.stringVal != NULL )
    {
        free(attr->val.stringVal);
    }
    else if ( attr->type == opaque && attr->val.opaqueVal != NULL )
    {
        free(attr->val.opaqueVal);
    }
    
    free(attr);
}   

void SLPFreeAttrList(SLPAttrList *list, int staticFlag)
{
    SLPAttrList *temp;

    while ( ! (SLP_IS_EMPTY(list)) )
    {
        temp = list->next;
       SLP_UNLINK(temp);
        SLPFreeAttr(temp);
    }
    if ( staticFlag == TRUE )
    {
        SLPFreeAttr(list);
    }
    
    return;
}       

void SLPInitInternalAttrList(void)
{
    attrHead.next = attrHead.prev = &attrHead;
    attrHead.isHead = TRUE;
    inProcessAttr.next =  inProcessAttr.prev = &inProcessAttr;
    inProcessAttr.isHead = TRUE;
    inProcessTag.next =  inProcessTag.prev = &inProcessTag;
    inProcessTag.isHead = TRUE;
    return;
}   

SLPAttrList *_SLPDecodeAttrString(char *s)
{
    unsigned int lexer = 0;
    SLPAttrList *temp = NULL;

   SLPInitInternalAttrList();
    if ( s != NULL )
    {
        if ( NULL != (temp = SLPAllocAttrList()) )
        {
            if ((0 != (lexer = slp_attr_init_lexer(s))) &&  yyparse() )
            {
                SLPFreeAttrList(temp,0);

                while ( !SLP_IS_HEAD(inProcessTag.next) )
                {
                    temp = inProcessTag.next;
                   SLP_UNLINK(temp);
                    SLPFreeAttr(temp);
                }
                while ( !SLP_IS_HEAD(inProcessAttr.next) )
                {
                    temp = inProcessAttr.next;
                   SLP_UNLINK(temp);
                    SLPFreeAttr(temp);
                }
                while ( !SLP_IS_HEAD(attrHead.next) )
                {
                    temp = attrHead.next;
                   SLP_UNLINK(temp);
                    SLPFreeAttr(temp);
                }

                slp_attr_close_lexer(lexer);
                
                return(NULL);
            }

            if ( !SLP_IS_EMPTY(&attrHead) )
            {
               SLP_LINK_HEAD(temp, &attrHead);
            }

            if ( lexer != 0 )
            {
                slp_attr_close_lexer(lexer);
            }
        }
    }

    return(temp);
}
