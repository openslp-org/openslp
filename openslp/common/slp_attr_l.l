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
 * Copyright (C) Michael Day, 1999-2001 
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

/** Lex input file for attribute parser.
 *
 * @file       slp_attr_l.l
 * @date       03-06-2000
 * @author     Matthew Peterson, Michael Day (md@soft-hackle.net), 
 *             John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

%{

#undef YYLMAX
#define YYLMAX 2048

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
#define YY_NO_UNPUT

#include "slp_attr.h"
#include "slp_attr_y.h"
#include "slp_xmalloc.h"

static char buf[2052];

void slp_attr_error(char *, ...);

int slp_attr_wrap(void);

int slp_attr_lex(void);

void slp_attr_close_lexer(unsigned int handle);

uint slp_attr_init_lexer(char *s);


%}

/* {}	*/
digit			[0-9]
hexdigit		[0-9a-fA-F]
escaped			("\\"([a-fA-F0-9]{2}))
reserved		[(),\\!<=>~\x00-\x1f]
res_tag			[(),\\!<>~\x00-\x1f*_\r\n\t]
res_val			[*_\r\n\t]
tag				[^(),\\!<=>~\x00-\x1f]
val				[^(),\\!<>~\x00-\x1f*_\r\n\t]

 /* 
   this lexer cycles through three states. in the initial state, 
   it is looking strictly for attribute tags. If, in the initial 
   state it finds a '(' it anticipates an attribute-value pair 
   and changes to ATTRIBUTE state .

   in the ATTRIBUTE state is is looking for either a closing ')'
   or a '='. If it sees a closing ')' it changes to the initial state. 
   if it sees an '=' it changes to the VALUE state and looks for 
   the attribute value. After seeing the attribute value, it looks
   for a either a ')' or a ','. If it sees a ')' it closes the attribute
   and returns to the initial state. 

   if, during the VALUE state the lexer sees a ',' it decides it is 
   looking at a multi-valued attribute and returns a special _MULTIVAL
   token to the parser. The parser uses this to construct additional
   attribute-value pairs for each value in the multi-val statement. 
 */ 
%option noyywrap
%x ATTR
%x VALUE
%%

[ \t\v\f]*              { ; }

"," 			{ yylval._i = *yytext; return(yylval._i); }
"("			{ BEGIN ATTR; yylval._i = *yytext; return(yylval._i); }
{tag}+			{   
            if (yyleng > 0) {
               if(NULL != (yylval._s = xstrdup(yytext)))
                                 return(_TAG);
                              else
                                 return(0L);
                           }
                        }

 /* anything else in the initial state is an error */
.				{yyerror("error in lexer - initial state\n");}

<ATTR>[ \t\v\f]*        { ; }
<ATTR>"="	        {BEGIN VALUE; yylval._i = *yytext; return(yylval._i);}
<ATTR>{tag}+	        {   
                          if (yyleng > 0 ) {
                             if(NULL != (yylval._s = xstrdup(yytext)))
                                return(_TAG);
                             else
                                return(0L);
                           }
                        }

 /* make it legal to have just an attr tag enclosed in parens */
<ATTR>")"		{BEGIN INITIAL; yylval._i = *yytext; return(yylval._i); }

 /* anything else in the attribute state is an error */
<ATTR>.			{yyerror("error in lexer - attribute state\n");}

<VALUE>[ \t\v\f]*       { ; }
<VALUE>","  		{yylval._i = *yytext; return(_MULTIVAL);}

<VALUE>[-+][0-9]+             |
<VALUE>[-+]"0"[xX]{hexdigit}+ { yylval._i = strtol(yytext, (char **) 0, 0); return _INT ;};

<VALUE>[0-9]+                 |
<VALUE>"0"[xX]{hexdigit}+     { yylval._i = strtoul(yytext, (char **)0, 0); return _INT; };


<VALUE>[tT][rR][uU][eE] {return(_TRUE);}
<VALUE>[fF][aA][lL][sS][eE]  {return(_FALSE);}

<VALUE>{val}+	        {	
                           if (yyleng > 0) {
                              if(NULL != (yylval._s = xstrdup(yytext)))
                                 return(_STRING);
                              else
                                 return(0L);
                           }
                        }		

<VALUE>{escaped}+       {	
                           if (yyleng > 0) {
                              if(NULL != (yylval._s = xstrdup(yytext)))
                                 return(_ESCAPED);
                              else
                                 return(0L);
                           }
                        }


<VALUE>")"	        {BEGIN INITIAL; yylval._i = *yytext; return(yylval._i);}

 /* anything else in the value state is an error */
<VALUE>.		{yyerror("error in lexer - value state\n");}

%%

unsigned int slp_attr_init_lexer(char *s)
{
    memset(&buf[0], 0x00, 2052);
    strncpy(&buf[0], s, 2052);
    return((uint)yy_scan_buffer(&buf[0], strlen(s) + 2));
}

void attr_close_lexer(uint handle)
{
    // assert(handle != 0);
    yy_delete_buffer((YY_BUFFER_STATE)handle);
}


void yyerror(char *s, ...)
{
    return;
}   

/*=========================================================================*/
