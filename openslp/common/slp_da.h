/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP                                                    */
/*                                                                         */
/* File:        slp_da.h                                                   */
/*                                                                         */
/* Abstract:    Functions and data structures to keep track of DAs         */
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

#if(!defined SLP_DA_H_INCLUDED)
#define SLP_DA_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>

#include <slp_linkedlist.h>

/*=========================================================================*/
typedef struct _SLPDAEntry
/*=========================================================================*/
{
    ListItem        listitem;
    struct in_addr  daaddr;
    char*           scopelist;
    int             scopelistlen;
}SLPDAEntry;

/*=========================================================================*/
SLPDAEntry* SLPDAEntryCreate(struct in_addr* addr,
                             const char* scopelist,
                             int scopelistlen);
/* Creates a SLPDAEntry                                                    */
/*                                                                         */
/* addr     (IN) pointer to in_addr of the DA                              */
/*                                                                         */
/* daadvert (IN) pointer to a daadvert of the DA                           */
/*                                                                         */
/* returns  Pointer to new SLPDAEntry.                                     */
/*=========================================================================*/


/*=========================================================================*/
void SLPDAEntryFree(SLPDAEntry* entry);
/* Frees a SLPDAEntry                                                      */
/*                                                                         */
/* entry    (IN) pointer to entry to free                                  */
/*                                                                         */
/* returns  none                                                           */
/*=========================================================================*/


/*=========================================================================*/
int SLPDAEntryWrite(int fd, SLPDAEntry* entry);
/*=========================================================================*/


/*=========================================================================*/
SLPDAEntry* SLPDAEntryRead(int fd);
/*=========================================================================*/


/*=========================================================================*/
int SLPDAEntryListWrite(int fd, SLPDAEntry** head);
/* Returns: Number of entries written                                      */
/*=========================================================================*/


/*=========================================================================*/
int SLPDAEntryListRead(int fd, SLPDAEntry** head);
/* Returns: zero on success, -1 on error                                   */
/*=========================================================================*/

#endif
