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

/** Bison parser for encoding and decoding LDAP attributes.
 *
 * @file       slp_attr_y.y
 * @author     Michael Day (ncmike@ncultra.org), 
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeFilter
 */

%{
#include "slp_client.h"
void attrerror(int8 *, ...);
int32 attrwrap(void);
int32 attrlex(void);   
int32 attrparse(void);
BOOL bt = TRUE, bf = FALSE;
void attr_close_lexer(uint32 handle);
uint32 attr_init_lexer(int8 *s);

lslpAttrList attrHead = 
{
	&attrHead, &attrHead, TRUE
};

lslpAttrList inProcessAttr = 
{
	&inProcessAttr, &inProcessAttr, TRUE
};

lslpAttrList inProcessTag = 
{
	&inProcessTag, &inProcessTag, TRUE
};


%}

/* definitions for ytab.h */
%name-prefix="attr"

%union {
	int32 _i;
	int8 *_s;
	lslpAttrList *_atl;
}

%token<_i> _TRUE _FALSE _MULTIVAL _INT 
%token<_s> _ESCAPED _TAG _STRING _OPAQUE

/* typecast the non-terminals */

/* %type <_i> */
%type <_atl> attr_list attr attr_val_list attr_val

%%

attr_list: attr {
			while (! _LSLP_IS_HEAD(inProcessAttr.next))
			{
				$$ = inProcessAttr.next;
				_LSLP_UNLINK($$);
				_LSLP_INSERT_BEFORE($$, &attrHead);
			}
		/* all we really want to do here is link each attribute */
		/* to the global list head. */
		}
	| attr_list ',' attr {
		/* both of these non-terminals are really lists */
		/* ignore the first non-terminal */
			while (! _LSLP_IS_HEAD(inProcessAttr.next))
			{
				$$ = inProcessAttr.next;
				_LSLP_UNLINK($$);
				_LSLP_INSERT_BEFORE($$, &attrHead);
			}
		}
	;

attr: _TAG 	{
			$$ =  lslpAllocAttr($1, tag, NULL, 0);
			if (NULL != $$)
			{
				_LSLP_INSERT_BEFORE($$, &inProcessAttr);
			}
		}
	| '(' _TAG ')' 	{
			$$ =  lslpAllocAttr($2, tag, NULL, 0);
			if (NULL != $$)
			{
				_LSLP_INSERT_BEFORE($$, &inProcessAttr);
			}
		}
        | '(' _TAG '=' ')' {
  			$$ =  lslpAllocAttr($2, tag, NULL, 0);
			if (NULL != $$)
			{
				_LSLP_INSERT_BEFORE($$, &inProcessAttr);
			}
               }

	| '(' _TAG '=' attr_val_list ')' {
			$$ = inProcessTag.next;
			while (! _LSLP_IS_HEAD($$))
			{
				$$->name = strdup($2); 
				_LSLP_UNLINK($$);
				_LSLP_INSERT_BEFORE($$, &inProcessAttr);
				$$ = inProcessTag.next;
			}
		}
	;

attr_val_list: attr_val {

			if(NULL != $1)
			{
				_LSLP_INSERT($1, &inProcessTag);
			}
		}
	| attr_val_list _MULTIVAL attr_val {
			if (NULL != $3)
			{
				_LSLP_INSERT_BEFORE($3, &inProcessTag);
			}
		}
	;
attr_val: _TRUE {
			$$ = lslpAllocAttr(NULL, bool_type,  &bt, sizeof(BOOL));
		}
	|     _FALSE {
			$$ = lslpAllocAttr(NULL, bool_type,  &bf, sizeof(BOOL));
		}
	|     _ESCAPED {
			$$ = lslpAllocAttr(NULL, opaque, $1, (int16)(strlen($1) + 1));
		}
	|	  _STRING {
	                     if(strlen($1) > 5 ) {
				if( *($1) == '\\' && ((*($1 + 1) == 'f') || (*($1 + 1) == 'F')) &&  ((*($1 + 2) == 'f') || (*($1 + 2) == 'F'))) {
				       $$ = lslpAllocAttr(NULL, opaque, $1, (int16)(strlen($1) + 1));
                                     } else {
				       $$ = lslpAllocAttr(NULL, string, $1, (int16)(strlen($1) + 1));
				     }
                                  }
			     else {
			       
			       $$ = lslpAllocAttr(NULL, string, $1, (int16)(strlen($1) + 1));
			     }
               } 

	|     _INT {
			$$ = lslpAllocAttr(NULL, integer, &($1), sizeof(int32));
		}
	;
	
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

lslpAttrList *_lslpDecodeAttrString(int8 *s)
{
  uint32 lexer = 0;
  lslpAttrList *temp = NULL;
  assert(s != NULL);
  _lslpInitInternalAttrList();
  if (s != NULL) {
    if(NULL != (temp = lslpAllocAttrList()))  {
      if ((0 != (lexer = attr_init_lexer( s))) &&  attrparse()) {
	lslpFreeAttrList(temp, TRUE);
	while (! _LSLP_IS_HEAD(inProcessTag.next))  {
	  temp = inProcessTag.next;
	    _LSLP_UNLINK(temp);
	    lslpFreeAttr(temp);
	}
	while (! _LSLP_IS_HEAD(inProcessAttr.next))  {
	  temp = inProcessAttr.next;
	  _LSLP_UNLINK(temp);
	  lslpFreeAttr(temp);
	}
	while (! _LSLP_IS_HEAD(attrHead.next))  {
	  temp = attrHead.next;
	  _LSLP_UNLINK(temp);
	  lslpFreeAttr(temp);
	}
	attr_close_lexer(lexer);
	return(NULL);
      }
      
      if (! _LSLP_IS_EMPTY(&attrHead)) {
	temp->attr_string_len = strlen(s);
	temp->attr_string = (int8 *)malloc(temp->attr_string_len + 1);
	if(temp->attr_string != NULL) {
	  memcpy(temp->attr_string, s, temp->attr_string_len);
	  temp->attr_string[temp->attr_string_len] = 0x00;
	}
	_LSLP_LINK_HEAD(temp, &attrHead);
      }
      if(lexer != 0) 
	attr_close_lexer(lexer);
    }
  }
  
  return(temp);
}	


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
	  attr->attr_len = len;
	  switch (type)	    {
	    case string:
	      if ( NULL != (attr->val.stringVal = strdup((int8 *)val)))
		return(attr);
	      break;
	    case integer:
	      attr->val.intVal = *(uint32 *)val;
	      break;
	    case bool_type:
	      attr->val.boolVal = *(BOOL *)val;
	      break;
	    case opaque:
	      if ( NULL != (attr->val.opaqueVal = strdup((int8 *)val)))
		return(attr);
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
  if(attr->attr_string != NULL)
    free(attr->attr_string);
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


