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

/** Flex input file for LDAPv3 search filter.
 *
 * @file       slp_filter_l.l
 * @date       04-21-2001
 * @author     Matthew Peterson, Michael Day (md@soft-hackle.net), 
 *             John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeFilter
 */

%{

#undef YYLMAX
#define YYLMAX 2048

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
#define YY_NO_UNPUT

#include "slp_filter.h"
#include "slp_filter_y.h"
#include "slp_xmalloc.h"

static char buf[2052];

void slp_filter_error(char *, ...);

void slp_filter_close_lexer(unsigned int handle);

unsigned int slp_filter_init_lexer(const char *s);

%}

/* regex macros for the lexer */
hexdigit       [0-9a-fA-F]
reserved       [()\&|!=<>~\n] /* newline in reserved set for convinience */
not_reserved   [^()\&|!=<>~\n] 


/* special lexer states */
%x QUOTED_STRING

/* table size directives */

%option noyywrap
%%

"\""                          { BEGIN QUOTED_STRING; yyless(0); }
<QUOTED_STRING>[^()\&|!=<>~\n]+  {
                                 BEGIN INITIAL; 
                                 if(0 != (yylval.filter_string =  xstrdup(yytext)))
                                    return(OPERAND) ; 
                                 else 
                                    return(0L) ;
                              }


[ \t\v\f]*                    { ; }
"("                           { yylval.filter_int = L_PAREN; return L_PAREN ; }
")"                           { yylval.filter_int = R_PAREN; return R_PAREN ; }
"&"                           { yylval.filter_int = OP_AND; return OP_AND ; }
"|"                           { yylval.filter_int = OP_OR; return OP_OR ; }
"!"                           { yylval.filter_int = OP_NOT; return OP_NOT ; }
"="                           { yylval.filter_int = OP_EQU; return OP_EQU ; }
">="                          { yylval.filter_int = OP_GT; return OP_GT ; }
"<="                          { yylval.filter_int = OP_LT; return OP_LT ; }
"=*"                          { yylval.filter_int = OP_PRESENT; return OP_PRESENT ; }
"~="                          { yylval.filter_int = OP_APPROX; return OP_APPROX ;  }


[-+][0-9]+                  |
[-+]"0"[xX]{hexdigit}+      { 
                              yylval.filter_int = strtol(yytext, (char **) 0, 0) ;
                              return VAL_INT;
                            }

[0-9]+                      |
"0"[xX]{hexdigit}+          { 
                              yylval.filter_int = strtoul(yytext, (char **) 0, 0);
                              return VAL_INT;
                            }

[tT][rR][uU][eE]            { 
                               yylval.filter_int = 1; return VAL_BOOL; 
                            }


[fF][aA][lL][sS][eE]        { 
                               yylval.filter_int = 0; return VAL_BOOL; 
                            }


[^()\&|!=<>~ \t\v\f]+       {
                                 if(0 != (yylval.filter_string =  xstrdup(yytext)))
                                    return(OPERAND) ; 
                                 else 
                                    return(0L) ;
                            }

%%

void filter_close_lexer(unsigned int handle)
{
   yy_delete_buffer((YY_BUFFER_STATE)handle);
}

unsigned int slp_filter_init_lexer(const char * s)
{
   memset(&buf[0], 0x00, 2052);
   strncpy(&buf[0], s, 2048);
   return((uint)yy_scan_buffer(&buf[0], strlen(s) + 2));
}

void slp_filter_error(char *s, ...)
{
   return;
}   

/*=========================================================================*/
