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


#endif
