/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP                                                    */
/*                                                                         */
/* File:        linkedlist.h                                               */
/*                                                                         */
/* Abstract:    Functions to manipulate a simple linked list               */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*     Please submit patches to http://www.openslp.org                     */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/* Copyright (C) 2000 Caldera Systems, Inc                                 */
/* All rights reserved.                                                    */
/*                                                                         */
/* Redistribution and use in source and binary forms, with or without      */
/* modification, are permitted provided that the following conditions are  */
/* met:                                                                    */ 
/*                                                                         */
/*      Redistributions of source code must retain the above copyright     */
/*      notice, this list of conditions and the following disclaimer.      */
/*                                                                         */
/*      Redistributions in binary form must reproduce the above copyright  */
/*      notice, this list of conditions and the following disclaimer in    */
/*      the documentation and/or other materials provided with the         */
/*      distribution.                                                      */
/*                                                                         */
/*      Neither the name of Caldera Systems nor the names of its           */
/*      contributors may be used to endorse or promote products derived    */
/*      from this software without specific prior written permission.      */
/*                                                                         */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS     */
/* `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT      */
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   */
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CALDERA      */
/* SYSTEMS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, */
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON       */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    */
/*                                                                         */
/***************************************************************************/

#if(!defined LINKEDLIST_H_INCLUDED)
#define LINKEDLIST_H_INCLUDED


/*=========================================================================*/
typedef struct _SLPListItem
/*=========================================================================*/
{
    struct _SLPListItem*    previous;
    struct _SLPListItem*    next;

}SLPListItem;


/*=========================================================================*/
typedef struct _SLPList
/*=========================================================================*/
{
    SLPListItem* head;
    SLPListItem* tail;
    int          count;
}SLPList;


/*=========================================================================*/
SLPListItem* SLPListUnlink(SLPList* list, SLPListItem* item);
/* Unlinks the specified item from the specified list.                     */
/*                                                                         */
/* list     (IN) pointer to the list to unlink the item from               */
/*                                                                         */
/* item     (IN) item to be removed from the list                          */
/*                                                                         */
/* Returns  pointer to the unlinked item                                   */
/*=========================================================================*/


/*=========================================================================*/
SLPListItem* SLPListLinkHead(SLPList* list, SLPListItem* item);
/* Links the specified item to the head of the specified list.             */
/*                                                                         */
/* list     (IN) pointer to the list to link to                            */
/*                                                                         */
/* item     (IN) item to be linkedto the list                              */
/*                                                                         */
/* Returns  pointer to the linked item                                     */
/*=========================================================================*/


/*=========================================================================*/
SLPListItem* SLPListLinkTail(SLPList* list, SLPListItem* item);
/* Links the specified item to the tail of the specified list.             */
/*                                                                         */
/* list     (IN) pointer to the list to link to                            */
/*                                                                         */
/* item     (IN) item to be linkedto the list                              */
/*                                                                         */
/* Returns  pointer to the linked item                                     */
/*=========================================================================*/


  /********* circular list macros *********************/

  /*---------------------------------------------------------------------**
  ** structures used with these macros MUST have the following elements: **
  ** struct type-name {                                                  **
  ** 	struct type-name *next;                                            **
  ** 	struct type-name *prev;                                            **
  ** 	BOOL isHead;                                                       **
  ** 	}                                                                  **
  **---------------------------------------------------------------------*/

  /*  is node x the head of the list? */
  /* BOOL IS_HEAD(node *x); */
#define _LSLP_IS_HEAD(x) ((x)->isHead )

  /* where h is the head of the list */
  /* BOOL _LSLP_IS_EMPTY(head);          */
#define _LSLP_IS_EMPTY(h) \
	((((h)->next == (h)) && ((h)->prev == (h)) ) ? TRUE : FALSE)

  /* where n is the new node, insert it immediately after node x */
  /* x can be the head of the list                               */
  /* void _LSLP_INSERT(new, after);                                  */
#define _LSLP_INSERT(n, x)   	\
	{(n)->prev = (x); 		\
	(n)->next = (x)->next; 	\
	(x)->next->prev = (n); 	\
	(x)->next = (n); }		

#define _LSLP_INSERT_AFTER _LSLP_INSERT
#define _LSLP_INSERT_BEFORE(n, x)   \
	{(n)->next = (x);					\
	(n)->prev = (x)->prev;				\
	(x)->prev->next = (n);				\
	(x)->prev = (n); }

#define _LSLP_INSERT_WORKNODE_LAST(n, x)    \
        {gettimeofday(&((n)->timer));  \
	(n)->next = (x);	       \
	(n)->prev = (x)->prev;	       \
	(x)->prev->next = (n);	       \
	(x)->prev = (n); }

#define _LSLP_INSERT_WORKNODE_FIRST(n, x)    \
        {gettimeofday(&((n)->timer));  \
	(n)->prev = (x); 	       \
	(n)->next = (x)->next; 	       \
	(x)->next->prev = (n); 	       \
	(x)->next = (n); }		


  /* delete node x  - harmless if mib is empty */
  /* void _LSLP_DELETE_(x);                        */
#define _LSLP_UNLINK(x)				\
	{(x)->prev->next = (x)->next;	\
	(x)->next->prev = (x)->prev;}	

  /* given the head of the list h, determine if node x is the last node */
  /* BOOL _LSLP_IS_LAST(head, x);                                           */
#define _LSLP_IS_LAST(h, x) \
	(((x)->prev == (h) && (h)->prev == (x)) ? TRUE : FALSE)

  /* given the head of the list h, determine if node x is the first node */
  /* BOOL _LSLP_IS_FIRST(head, x);                                           */
#define _LSLP_IS_FIRST(h, x) \
	(((x)->prev == (h) && (h)->next == (x)) ? TRUE : FALSE)

  /* given the head of the list h, determine if node x is the only node */
  /* BOOL _LSLP_IS_ONLY(head, x);                                           */
#define _LSLP_IS_ONLY(h, x) \
	(((x)->next == (h) && (h)->prev == (x)) ? TRUE : FALSE)
  /* void _LSLP_LINK_HEAD(dest, src); */
#define _LSLP_LINK_HEAD(d, s) \
	{(d)->next = (s)->next;  \
	(d)->prev = (s)->prev;  \
	(s)->next->prev = (d);  \
	(s)->prev->next = (d); }


#endif
