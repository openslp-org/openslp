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

#ifdef WIN32
#include <windows.h>
#include <io.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#endif

#include <slp_linkedlist.h>


/*=========================================================================*/
typedef struct _SLPDAEntry
/*=========================================================================*/
{
    SLPListItem     listitem;
    struct in_addr  daaddr;
    unsigned int    bootstamp;
    int             langtaglen;
    const char*     langtag;
    int             urllen;
    const char*     url;
    int             scopelistlen;
    const char*     scopelist;
    int             attrlistlen;
    const char*     attrlist;
    int             spilistlen;
    const char*     spilist;
}SLPDAEntry;


/*=========================================================================*/
SLPDAEntry* SLPDAEntryCreate(struct in_addr* addr,
                             const SLPDAEntry* daentry);
/* Creates a SLPDAEntry                                                    */
/*                                                                         */
/* addr     (IN) pointer to in_addr of the DA to create                    */
/*                                                                         */
/* daentry (IN) pointer to daentry that will be duplicated in the new      */
/*              daentry                                                    */
/*                                                                         */
/* returns  Pointer to the created SLPDAEntry.  Must be freed by caller.   */
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
int SLPDAEntryListWrite(int fd, SLPList* dalist);
/* Returns: Number of entries written                                      */
/*=========================================================================*/


/*=========================================================================*/
int SLPDAEntryListRead(int fd, SLPList* dalist);
/* Returns: zero on success, -1 on error                                   */
/*=========================================================================*/

#endif
