/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP                                                    */
/*                                                                         */
/* File:        linkedlist.c                                               */
/*                                                                         */
/* Abstract:    Functions to manipulate a simple linked list               */
/*                                                                         */
/***************************************************************************/

#include "slp_linkedlist.h"

/*=========================================================================*/
PListItem ListUnlink(PListItem* head, PListItem item)
/* Unlinks the specified item from the specified list.                     */
/*                                                                         */
/* head     (IN/OUT) pointer to a pointer to head of the list              */
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
    
    if(item == *head)
    {
        *head = item->next;
    }

    #if(defined DEBUG)
    item->previous = 0;
    item->next = 0;
    #endif
    
    return item;
}



/*=========================================================================*/
PListItem ListLink(PListItem* head, PListItem item)
/* Links the specified item to the head of the specified list.             */
/*                                                                         */
/* head     (IN/OUT) pointer to a pointer to head of the list              */
/*                                                                         */
/* item     (IN) item to be added to the list                              */
/*                                                                         */
/* Returns  pointer to the linked item                                     */
/*=========================================================================*/
{
    item->previous = 0;
    item->next = *head;
    
    if(*head)
    {
        (*head)->previous = item;
    }
    
    *head = item;
    
    return item;
}



