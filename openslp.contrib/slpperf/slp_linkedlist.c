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
/* Copyright (c) 1995, 1999  Caldera Systems, Inc.                         */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify it */
/* under the terms of the GNU Lesser General Public License as published   */
/* by the Free Software Foundation; either version 2.1 of the License, or  */
/* (at your option) any later version.                                     */
/*                                                                         */
/*     This program is distributed in the hope that it will be useful,     */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       */
/*     GNU Lesser General Public License for more details.                 */
/*                                                                         */
/*     You should have received a copy of the GNU Lesser General Public    */
/*     License along with this program; see the file COPYING.  If not,     */
/*     please obtain a copy from http://www.gnu.org/copyleft/lesser.html   */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*     Please submit patches to http://www.openslp.org                     */
/*                                                                         */
/***************************************************************************/

#include "slp_linkedlist.h"


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

