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

#undef YYLMAX
#define YYLMAX 2048

//#include "../../lslp-common/lslp-common-defs.h"
#include "slp_filter_y.h"
#include "slp_xmalloc.h"
#include <stdarg.h>

#define filterless yyless
void filtererror(char *, ...);

/*
extern char *lslp_logPath;

#define yyerror(s) 	_lslpMsgOut(lslp_logPath, lslpPrintSem, LSLP_LOG_ERRORS, NULL, 0, (s), __FILE__, __LINE__);
*/
static int heapIndex;
static char heap[2052];
static char buf[2052];

static char *_lslp_strdup(char *s);
void filter_close_lexer(unsigned int handle);
uint filter_init_lexer(char *s);


%}

/* regex macros for the lexer */
hexdigit		[0-9a-fA-F]
reserved                [()\&|!=<>~\n] /* newline in reserved set for convinience */
not_reserved            [^()\&|!=<>~\n] 


/* special lexer states */
%x QUOTED_STRING

/* table size directives */

%option noyywrap
%%

"\""                          { BEGIN QUOTED_STRING; filterless(0); }
<QUOTED_STRING>[^()\&|!=<>~\n]+  {
                                 BEGIN INITIAL; 
                                 if(NULL != (filterlval.filter_string =  _lslp_strdup(yytext)))
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
                              filterlval.filter_int = strtol(yytext, (char **) 0, 0) ;
                              return VAL_INT;
                            }

[0-9]+                      |
"0"[xX]{hexdigit}+          { 
                              filterlval.filter_int = strtoul(yytext, (char **) 0, 0);
                              return VAL_INT;
                            }

[tT][rR][uU][eE]            { 
                               filterlval.filter_int = 1; return VAL_BOOL; 
                            }


[fF][aA][lL][sS][eE]        { 
                               filterlval.filter_int = 0; return VAL_BOOL; 
                            }


[^()\&|!=<>~ \t\v\f]+       {
                                 if(NULL != (filterlval.filter_string =  xstrdup(yytext)))
                                    return(OPERAND) ; 
                                 else 
                                    return(0L) ;
                            }


 
%%

void filter_close_lexer(uint handle)
{
  //assert(handle != 0);
  yy_delete_buffer((YY_BUFFER_STATE)handle);
}


uint filter_init_lexer(char *s)
{
  memset(&buf[0], 0x00, 2052);
  memset(&heap[0], 0x00, 2052);
  heapIndex = 0;
  strncpy(&buf[0], s, 2048);
  return((uint)yy_scan_buffer(&buf[0], strlen(s) + 2));
}

static char *xstrdup_strip_quotes(char *s)
{
  char *p = &heap[heapIndex];
  do { if(*s != '"') heap[heapIndex++] = *s; } while ((*s != 0x00) && (heapIndex < 2049) && (++s));
  return(p);
}


void filtererror(char *s, ...)
{
#ifdef TEST_HARNESS
	va_list va;
	va_start(va, s);
	printf(s, va);
#endif
	return;
}	

/*
int main(int argc, char *argv[])
{

}	

*/

