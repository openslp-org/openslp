/*******************************************************************
 *  Description: encode/decode attribute lists
 *
 *  Originated: 03-06-2000 
 *	Original Author: Mike Day - md@soft-hackle.net
 *  Project: 
 *
 *  $Header$
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

#undef YYLMAX
#define YYLMAX 2048

#include "slp_attr_y.h"
#include "slp_xmalloc.h"

static int heapIndex;
static char heap[2052];
static char buf[2052];
void attrerror(char *, ...);
int attrwrap(void);
int attrlex(void);
void attr_close_lexer(unsigned int handle);
uint attr_init_lexer(char *s);


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

"," 			{ attrlval._i = *yytext; return(attrlval._i); }
"("			{ BEGIN ATTR; attrlval._i = *yytext; return(attrlval._i); }
{tag}+			{   
 			   if (yyleng > 0) {
			      if(NULL != (attrlval._s = xstrdup(yytext)))
                                 return(_TAG);
                              else
                                 return(0L);
                           }
                        }

 /* anything else in the initial state is an error */
.				{attrerror("error in lexer - initial state\n");}

<ATTR>[ \t\v\f]*        { ; }
<ATTR>"="	        {BEGIN VALUE; attrlval._i = *yytext; return(attrlval._i);}
<ATTR>{tag}+	        {   
                          if (yyleng > 0 ) {
                             if(NULL != (attrlval._s = xstrdup(yytext)))
                                return(_TAG);
                             else
                                return(0L);
                           }
                        }

 /* make it legal to have just an attr tag enclosed in parens */
<ATTR>")"		{BEGIN INITIAL; attrlval._i = *yytext; return(attrlval._i); }

 /* anything else in the attribute state is an error */
<ATTR>.			{attrerror("error in lexer - attribute state\n");}

<VALUE>[ \t\v\f]*       { ; }
<VALUE>","  		{attrlval._i = *yytext; return(_MULTIVAL);}

<VALUE>[-+][0-9]+             |
<VALUE>[-+]"0"[xX]{hexdigit}+ { attrlval._i = strtol(yytext, (char **) 0, 0); return _INT ;};

<VALUE>[0-9]+                 |
<VALUE>"0"[xX]{hexdigit}+     { attrlval._i = strtoul(yytext, (char **)0, 0); return _INT; };


<VALUE>[tT][rR][uU][eE] {return(_TRUE);}
<VALUE>[fF][aA][lL][sS][eE]  {return(_FALSE);}

<VALUE>{val}+	        {	
                           if (yyleng > 0) {
                              if(NULL != (attrlval._s = xstrdup(yytext)))
                                 return(_STRING);
                              else
                                 return(0L);
                           }
                        }		

<VALUE>{escaped}+       {	
                           if (yyleng > 0) {
                              if(NULL != (attrlval._s = xstrdup(yytext)))
                                 return(_ESCAPED);
                              else
                                 return(0L);
                           }
                        }


<VALUE>")"	        {BEGIN INITIAL; attrlval._i = *yytext; return(attrlval._i);}

 /* anything else in the value state is an error */
<VALUE>.		{attrerror("error in lexer - value state\n");}

%%

void attr_close_lexer(uint handle)
{
 // assert(handle != 0);
  yy_delete_buffer((YY_BUFFER_STATE)handle);
}

unsigned int attr_init_lexer(char *s)
{
  memset(&buf[0], 0x00, 2052);
  memset(&heap[0], 0x00, 2052);
  heapIndex = 0;
  strncpy(&buf[0], s, 2052);
  return((uint)yy_scan_buffer(&buf[0], strlen(s) + 2));
}


void attrerror(char *s, ...)
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
