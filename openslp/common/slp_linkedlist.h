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
