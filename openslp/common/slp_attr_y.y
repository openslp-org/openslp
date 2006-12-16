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

/** Bison parser input file for attribute parser.
 *
 * @file       slp_attr_y.y
 * @date       03-06-2000
 * @author     Matthew Peterson, Michael Day (md@soft-hackle.net), 
 *             John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeAttrs
 */

%{

#include "slp_types.h"
#include "slp_attr.h"
#include "slp_linkedlist.h"

/* prototypes and globals go here */

#ifndef FALSE
# define FALSE   0
#endif
#ifndef TRUE
# define TRUE    (!FALSE)
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
static SLPAttrList attrHead = {&attrHead, &attrHead, TRUE, 0, head, {0L}};
static SLPAttrList inProcessAttr = {&inProcessAttr, &inProcessAttr, TRUE, 0, head, {0L}};
static SLPAttrList inProcessTag = {&inProcessTag, &inProcessTag, TRUE, 0, head, {0L}};

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
   $$ =  SLPAllocAttr($1, tag, 0, 0);
   if ( 0 != $$ )
   {
      SLP_INSERT_BEFORE($$, &inProcessAttr);
   }
}
| '(' _TAG ')'    
{
   $$ =  SLPAllocAttr($2, tag, 0, 0);
   if (0 != $$)
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
   if(0 != $1)
   {
      SLP_INSERT($1, &inProcessTag);
   }
}
| attr_val_list _MULTIVAL attr_val 
{
   if (0 != $3)
   {
      SLP_INSERT_BEFORE($3, &inProcessTag);
   }
};

attr_val: _TRUE 
{
   $$ = SLPAllocAttr(0, boolean, &bt, sizeof(int));
}
| _FALSE 
{
   $$ = SLPAllocAttr(0, boolean, &bf, sizeof(int));
}
| _ESCAPED 
{ 
   /* treat it as a string because it is already encoded */
   $$ = SLPAllocAttr(0, string, $1, strlen($1) + 1);
}
| _STRING 
{    
   $$ = SLPAllocAttr(0, string, $1, strlen($1) + 1);
}
| _INT
{
   $$ = SLPAllocAttr(0, integer, &($1), sizeof(int));
};
       
%%

SLPAttrList *SLPAllocAttr(char *name, SLPTypes type, void *val, int len)
{
   SLPAttrList *attr;
   if ( 0 != (attr = (SLPAttrList *)calloc(1, sizeof(SLPAttrList))) )
   {
      if ( name != 0 )
      {
         if ( 0 == (attr->name = strdup(name)) )
         {
            free(attr);
            return(0);
         }
      }
      attr->type = type;
      if ( type == head ) /* listhead */
         return(attr);
      if ( val != 0 )
      {
         switch ( type )
         {
            case string:
               if ( 0 != (attr->val.stringVal = strdup((char *)val)) )
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
                  if ( 0 != attr->val.opaqueVal )
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
   if ( 0 != (temp = SLPAllocAttr(0, head, 0, 0)) )
   {
      temp->next = temp->prev = temp;
      temp->isHead = TRUE;  
   }
   return(temp);
}   

/* attr MUST be unlinked from its list ! */
void SLPFreeAttr(SLPAttrList *attr)
{
   if ( attr->name != 0 )
   {
      free(attr->name);
   }
   if ( attr->type == string && attr->val.stringVal != 0 )
   {
      free(attr->val.stringVal);
   }
   else if ( attr->type == opaque && attr->val.opaqueVal != 0 )
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
   SLPAttrList *temp = 0;

   SLPInitInternalAttrList();
   if ( s != 0 )
   {
      if ( 0 != (temp = SLPAllocAttrList()) )
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

            return 0;
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
   return temp;
}

/*=========================================================================*/
