/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP                                                    */
/*                                                                         */
/* File:        slp_da.c                                                   */
/*                                                                         */
/* Abstract:    Functions to keep track of DAs                             */
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

#include <slp_da.h>
#include <slp_compare.h>

/*=========================================================================*/
SLPDAEntry* SLPDAEntryCreate(struct in_addr* addr,
                             const SLPDAEntry* daentry)
/* Creates a SLPDAEntry                                                    */
/*                                                                         */
/* addr     (IN) pointer to in_addr of the DA to create                    */
/*                                                                         */
/* daentry (IN) pointer to daentry that will be duplicated in the new      */
/*              daentry                                                    */
/*                                                                         */
/* returns  Pointer to the created SLPDAEntry.  Must be freed by caller.   */
/*=========================================================================*/
{
    SLPDAEntry* entry;
    char*       curpos;
    size_t      size;

    size = sizeof(SLPDAEntry);
    size += daentry->langtaglen;
    size += daentry->urllen;
    size += daentry->scopelistlen;
    size += daentry->attrlistlen;
    size += daentry->spilistlen;

    entry = (SLPDAEntry*)malloc(size);
    if(entry == 0) return 0;
    
    entry->daaddr = *addr;
    entry->bootstamp = daentry->bootstamp;
    entry->langtaglen = daentry->langtaglen;
    entry->urllen = daentry->urllen;
    entry->scopelistlen = daentry->scopelistlen;
    entry->attrlistlen = daentry->attrlistlen;
    entry->spilistlen = daentry->spilistlen;
    curpos = (char*)(entry + 1);
    memcpy(curpos,daentry->langtag,daentry->langtaglen);
    entry->langtag = curpos;
    curpos = curpos + daentry->langtaglen; 
    memcpy(curpos,daentry->url,daentry->urllen);
    entry->url = curpos;
    curpos = curpos + daentry->urllen;
    memcpy(curpos,daentry->scopelist,daentry->scopelistlen);
    entry->scopelist = curpos;
    curpos = curpos + daentry->scopelistlen;
    memcpy(curpos,daentry->attrlist,daentry->attrlistlen);
    entry->attrlist = curpos;
    curpos = curpos + daentry->attrlistlen;
    memcpy(curpos,daentry->spilist,daentry->spilistlen);
    entry->spilist = curpos;
    
    return entry;
}

/*=========================================================================*/
void SLPDAEntryFree(SLPDAEntry* entry)
/* Frees a SLPDAEntry                                                      */
/*                                                                         */
/* entry    (IN) pointer to entry to free                                  */
/*                                                                         */
/* returns  none                                                           */
/*=========================================================================*/
{
    if(entry)
    {
        free(entry);
    }
}

