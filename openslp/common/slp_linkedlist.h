/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP                                                    */
/*                                                                         */
/* File:        linkedlist.h                                               */
/*                                                                         */
/* Abstract:    Functions to manipulate a simple linked list               */
/*                                                                         */
/***************************************************************************/

#if(!defined LINKEDLIST_H_INCLUDED)
#define LINKEDLIST_H_INCLUDED


/*=========================================================================*/
typedef struct _ListItem
/*=========================================================================*/
{
    struct _ListItem* previous;
    struct _ListItem* next;
}ListItem, *PListItem;


/*=========================================================================*/
PListItem ListUnlink(PListItem* head, PListItem item);
/* Unlinks the specified item from the specified list.                     */
/*                                                                         */
/* head     (IN/OUT) pointer to a pointer to head of the list              */
/*                                                                         */
/* item     (IN) item to be removed from the list                          */
/*                                                                         */
/* Returns  pointer to the unlinked item                                   */
/*=========================================================================*/


/*=========================================================================*/
PListItem ListLink(PListItem* head, PListItem item);
/* Links the specified item to the head of the specified list.             */
/*                                                                         */
/* head     (IN/OUT) pointer to a pointer to head of the list              */
/*                                                                         */
/* item     (IN) item to be added to the list                              */
/*                                                                         */
/* Returns  pointer to the linked item                                     */
/*=========================================================================*/

#endif
