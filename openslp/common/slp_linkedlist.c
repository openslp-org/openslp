/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP                                                    */
/*                                                                         */
/* File:        linkedlist.c                                               */
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

#include <slp_linkedlist.h>


/*=========================================================================*/
SLPListItem* SLPListUnlink(SLPList* list, SLPListItem* item)
/* Unlinks the specified item from the specified list.                     */
/*                                                                         */
/* list     (IN) pointer to the list to unlink the item from               */
/*                                                                         */
/* item     (IN) item to be removed from the list                          */
/*                                                                         */
/* Returns  pointer to the unlinked item                                   */
/*=========================================================================*/
{
	if(item->previous)
	{
		item->previous->next = item->next;
	}

	if(item->next)
	{
		item->next->previous = item->previous;
	}

	if(item == list->head)
	{
		list->head = item->next;
	}

	if(item == list->tail)
	{
		list->tail = item->previous;
	}

	list->count = list->count - 1;
	return item;
}


/*=========================================================================*/
SLPListItem* SLPListLinkHead(SLPList* list, SLPListItem* item)
/* Links the specified item to the head of the specified list.             */
/*                                                                         */
/* list     (IN) pointer to the list to link to                            */
/*                                                                         */
/* item     (IN) item to be linkedto the list                              */
/*                                                                         */
/* Returns  pointer to the linked item                                     */
/*=========================================================================*/
{
	item->previous = 0;
	item->next = list->head;

	if(list->head)
	{
		list->head->previous = item;
	}

	list->head = item;

	if(list->tail == 0)
	{
		list->tail = item;
	}

	list->count = list->count + 1;

	return item;
}


/*=========================================================================*/
SLPListItem* SLPListLinkTail(SLPList* list, SLPListItem* item)
/* Links the specified item to the tail of the specified list.             */
/*                                                                         */
/* list     (IN) pointer to the list to link to                            */
/*                                                                         */
/* item     (IN) item to be linkedto the list                              */
/*                                                                         */
/* Returns  pointer to the linked item                                     */
/*=========================================================================*/
{
	item->previous = list->tail;
	item->next = 0;

	if(list->tail)
	{
		list->tail->next = item;
	}

	list->tail = item;

	if(list->head == 0)
	{
		list->head = item;
	}

	list->count = list->count + 1;

	return item;
}

