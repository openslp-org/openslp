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
 *-------------------------------------------------------------------------*/

/** Functions to manipulate a simple linked-list.
 *
 * Lists are managed by ensuring that objects that want to be linked have
 * a link structure as their first element. These routines would not be 
 * good for a generalized linked-list implementation, but since all the
 * structures we want to "listify" can be modified to contain a link
 * structure as their first element, they serve us well.
 *
 * @file       slp_linkedlist.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#include "slp_linkedlist.h"

/** Unlinks an item from a list.
 *
 * @param[in] list - A pointer to the list to unlink the item from.
 * @param[in] item - The item to be removed from @p list.
 *
 * @return A pointer to the unlinked item.
 */
SLPListItem* SLPListUnlink(SLPList* list, SLPListItem* item)
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

/** Links an item into a list.
 *
 * @param[in] list - A pointer to the list to link to.
 * @param[in] item - The item to be linked into @p list.
 *
 * @return A pointer to the linked item.
 */
SLPListItem* SLPListLinkHead(SLPList* list, SLPListItem* item)
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

/** Links an item to the tail of a list.
 *
 * @param[in] list - A pointer to the list to link to.
 * @param[in] item - The item to be linked into @p list.
 *
 * @return A pointer to the linked item.
 */
SLPListItem* SLPListLinkTail(SLPList* list, SLPListItem* item)
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

/*=========================================================================*/
