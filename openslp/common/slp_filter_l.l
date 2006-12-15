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

/** Lexer for encoding and decoding LDAP filters.
 *
 * @file       slp_filter_l.l
 * @author     Michael Day (ncmike@ncultra.org), 
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeFilter
 */

%{

#undef YYLMAX
#define YYLMAX 2048
#define YY_NEVER_INTERACTIVE 1
#undef YY_INPUT
#define YY_INPUT(b, r, m) abort()
#undef ECHO
#define ECHO  filtererror(yytext)
#define exit(i) abort()
#include "slp_client.h"
#include "y_filter.h"
#include <stdarg.h>

#define filterless yyless
void filtererror(int8 *, ...);

/*
extern char *lslp_logPath;

#define yyerror(s) 	_lslpMsgOut(lslp_logPath, lslpPrintSem, LSLP_LOG_ERRORS, NULL, 0, (s), __FILE__, __LINE__);
*/
static int16 heapIndex;
static int8 heap[2052];
static int8 buf[2052];

static int8 *_lslp_strdup(int8 *s);
void filter_close_lexer(uint32 handle);
uint32 filter_init_lexer(int8 *s);


%}

/* regex macros for the lexer */
hexdigit		[0-9a-fA-F]
reserved                [()\&|!=<>~\n] /* newline in reserved set for convinience */
not_reserved            [^()\&|!=<>~\n] 


/* special lexer states */
%x QUOTED_STRING

/* table size directives */

%option noyywrap prefix="filter"
%%

"\""                          { BEGIN QUOTED_STRING; filterless(0); }
<QUOTED_STRING>[^()\&|!=<>~\n]+  {
                                 BEGIN INITIAL; 
                                 if(NULL != (filterlval.filter_string =  _lslp_strdup(filtertext)))
                                    return(OPERAND) ; 
                                 else 
                                    return(0L) ;
                              }


[ \t\v\f]*                    { ; }
"("                           { filterlval.filter_int = L_PAREN; return L_PAREN ; }
")"                           { filterlval.filter_int = R_PAREN; return R_PAREN ; }
"&"                           { filterlval.filter_int = OP_AND; return OP_AND ; }
"|"                           { filterlval.filter_int = OP_OR; return OP_OR ; }
"!"                           { filterlval.filter_int = OP_NOT; return OP_NOT ; }
"="                           { filterlval.filter_int = OP_EQU; return OP_EQU ; }
">="                          { filterlval.filter_int = OP_GT; return OP_GT ; }
"<="                          { filterlval.filter_int = OP_LT; return OP_LT ; }
"=*"                          { filterlval.filter_int = OP_PRESENT; return OP_PRESENT ; }
"~="                          { filterlval.filter_int = OP_APPROX; return OP_APPROX ;  }


[-+][0-9]+                  |
[-+]"0"[xX]{hexdigit}+      { 
                              filterlval.filter_int = strtol(filtertext, (char **) 0, 0) ;
                              return VAL_INT;
                            }

[0-9]+                      |
"0"[xX]{hexdigit}+          { 
                              filterlval.filter_int = strtoul(filtertext, (char **) 0, 0);
                              return VAL_INT;
                            }

[tT][rR][uU][eE]            { 
                               filterlval.filter_int = 1; return VAL_BOOL; 
                            }


[fF][aA][lL][sS][eE]        { 
                               filterlval.filter_int = 0; return VAL_BOOL; 
                            }


[^()\&|!=<>~ \t\v\f]+       {
                                 if(NULL != (filterlval.filter_string =  _lslp_strdup(filtertext)))
                                    return(OPERAND) ; 
                                 else 
                                    return(0L) ;
                            }


 
%%

void filter_close_lexer(uint32 handle)
{
  assert(handle != 0);
  filter_delete_buffer((YY_BUFFER_STATE)handle);
}


uint32 filter_init_lexer(int8 *s)
{
  memset(&buf[0], 0x00, 2052);
  memset(&heap[0], 0x00, 2052);
  heapIndex = 0;
  strncpy(&buf[0], s, 2048);
  return((uint32)filter_scan_buffer(&buf[0], strlen(s) + 2));
}


static int8 *_lslp_strdup(int8 *s)
{
  int8 *p = &heap[heapIndex];
  do { heap[heapIndex++] = *s; } while ((*s != 0x00) && (heapIndex < 2049) && (++s));
  return(p);
}

/*****
static int8 *_lslp_strdup_strip_quotes(int8 *s)
{
  int8 *p = &heap[heapIndex];
  do { if(*s != '"') heap[heapIndex++] = *s; } while ((*s != 0x00) && (heapIndex < 2049) && (++s));
  return(p);
}

******/

void filtererror(int8 *s, ...)
{
	return;
}	

/*
int main(int argc, char *argv[])
{

}	

*/

