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
#include <unistd.h>
#include <string.h>

#include "slp_attr.h"
#include "slp_linkedlist.h"



/* prototypes and globals go here */

#ifndef FALSE
#define FALSE   0
#endif
#ifndef TRUE
#define TRUE    (!FALSE)
#endif

int attrparse(void);
unsigned int heapHandle;
int bt = TRUE, bf = FALSE;
void attr_close_lexer(unsigned int handle);
unsigned int attr_init_lexer(char *s);

lslpAttrList attrHead = {&attrHead, &attrHead, TRUE, NULL, head, {0L}};

lslpAttrList inProcessAttr = {&inProcessAttr, &inProcessAttr, TRUE, NULL, head, {0L}};

lslpAttrList inProcessTag = {&inProcessTag, &inProcessTag, TRUE, NULL, head, {0L}};


%}

/* definitions for ytab.h */

%union 
{
    int _i;
    char *_s;
    lslpAttrList *_atl;
}

%token<_i> _TRUE _FALSE _MULTIVAL _INT 
%token<_s> _ESCAPED _TAG _STRING 

/* typecast the non-terminals */

/* %type <_i> */
%type <_atl> attr_list attr attr_val_list attr_val

%%

attr_list: attr 
{
    while ( ! _LSLP_IS_HEAD(inProcessAttr.next) )
    {
        $$ = inProcessAttr.next;
        _LSLP_UNLINK($$);
        _LSLP_INSERT_BEFORE($$, &attrHead);
    }
    /* all we really want to do here is link each attribute */
    /* to the global list head. */
}
| attr_list ',' attr 
{
    /* both of these non-terminals are really lists */
    /* ignore the first non-terminal */
    while ( ! _LSLP_IS_HEAD(inProcessAttr.next) )
    {
        $$ = inProcessAttr.next;
        _LSLP_UNLINK($$);
        _LSLP_INSERT_BEFORE($$, &attrHead);
    }
};

attr: _TAG
{
    $$ =  lslpAllocAttr($1, tag, NULL, 0);
    if ( NULL != $$ )
    {
        _LSLP_INSERT_BEFORE($$, &inProcessAttr);
    }
}
| '(' _TAG ')' 	
{
    $$ =  lslpAllocAttr($2, tag, NULL, 0);
    if (NULL != $$)
    {
        _LSLP_INSERT_BEFORE($$, &inProcessAttr);
    }
}
| '(' _TAG '=' attr_val_list ')' 
{
    $$ = inProcessTag.next;
    while (! _LSLP_IS_HEAD($$))
    {
        $$->name = strdup($2); 
	_LSLP_UNLINK($$);
	_LSLP_INSERT_BEFORE($$, &inProcessAttr);
	$$ = inProcessTag.next;
    }
};

attr_val_list: attr_val 
{
    if(NULL != $1)
    {
        _LSLP_INSERT($1, &inProcessTag);
    }
}
| attr_val_list _MULTIVAL attr_val 
{
    if (NULL != $3)
    {
        _LSLP_INSERT_BEFORE($3, &inProcessTag);
    }
};

attr_val: _TRUE 
{
    $$ = lslpAllocAttr(NULL, bool, &bt, sizeof(int));
}
| _FALSE 
{
    $$ = lslpAllocAttr(NULL, bool, &bf, sizeof(int));
}
| _ESCAPED 
{ 
    /* treat it as a string because it is already encoded */
    $$ = lslpAllocAttr(NULL, string, $1, strlen($1) + 1);
}
| _STRING 
{    
    $$ = lslpAllocAttr(NULL, string, $1, strlen($1) + 1);
}
| _INT
{
    $$ = lslpAllocAttr(NULL, integer, &($1), sizeof(int));
};
 
       
%%


void _lslpInitInternalAttrList(void)
{
    attrHead.next = attrHead.prev = &attrHead;
    attrHead.isHead = TRUE;
    inProcessAttr.next =  inProcessAttr.prev = &inProcessAttr;
    inProcessAttr.isHead = TRUE;
    inProcessTag.next =  inProcessTag.prev = &inProcessTag;
    inProcessTag.isHead = TRUE;
    return;
}   

lslpAttrList *_lslpDecodeAttrString(char *s)
{
    unsigned int lexer = 0;
    lslpAttrList *temp = NULL;

    _lslpInitInternalAttrList();
    if ( s != NULL )
    {
        if ( NULL != (temp = lslpAllocAttrList()) )
        {
            if ((0 != (lexer = attr_init_lexer(s))) &&  attrparse() )
            {
                lslpFreeAttrList(temp);

                while ( ! _LSLP_IS_HEAD(inProcessTag.next) )
                {
                    temp = inProcessTag.next;
                    _LSLP_UNLINK(temp);
                    lslpFreeAttr(temp);
                }
                while ( ! _LSLP_IS_HEAD(inProcessAttr.next) )
                {
                    temp = inProcessAttr.next;
                    _LSLP_UNLINK(temp);
                    lslpFreeAttr(temp);
                }
                while ( ! _LSLP_IS_HEAD(attrHead.next) )
                {
                    temp = attrHead.next;
                    _LSLP_UNLINK(temp);
                    lslpFreeAttr(temp);
                }

                attr_close_lexer(lexer);
                
                return(NULL);
            }

            if ( ! _LSLP_IS_EMPTY(&attrHead) )
            {
                _LSLP_LINK_HEAD(temp, &attrHead);
            }

            if ( lexer != 0 )
            {
                attr_close_lexer(lexer);
            }
        }
    }

    return(temp);
}
