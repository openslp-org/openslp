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
int SLPDAEntryListWrite(int fd, SLPList* dalist)
/* Returns: Number of entries written                                      */
/*=========================================================================*/
{
    int         result = 0;
    SLPDAEntry* entry = (SLPDAEntry*)dalist->head;
    
    while(entry)
    {
        if(SLPDAEntryWrite(fd, entry) == 0)
        {
            break;
        } 
        result += 1;
        entry = (SLPDAEntry*)entry->listitem.next;
    }

    return result;
}

/*=========================================================================*/
int SLPDAEntryListRead(int fd, SLPList* dalist)
/* Returns: zero on success, -1 on error                                   */
/*=========================================================================*/
{
    int         result = 0;
    SLPDAEntry* entry;

    while(1)
    {
        entry = SLPDAEntryRead(fd);
        if(entry == 0)
        {
            break;
        }
        result += 1;
        SLPListLinkHead(dalist,(SLPListItem*)entry);
    }

    return result;
}

/*=========================================================================*/
SLPDAEntry* KnownDAListFindByScope(SLPList* dalist,
                                   int scopelistlen,
                                   const char* scopelist)
/* Returns: pointer to found DA.                                           */
/*=========================================================================*/
{
    SLPDAEntry*     entry;
    
    entry = (SLPDAEntry*)dalist->head;
    while(entry)
    {
        if(SLPCompareString(scopelistlen,
                            scopelist,
                            entry->scopelistlen,
                            entry->scopelist) == 0)
        {
            break;
        }   
        
        entry = (SLPDAEntry*) entry->listitem.next;
    }

    return entry;
}


/*=========================================================================*/
int KnownDAListAdd(SLPList* dalist, SLPDAEntry* addition)
/* Add an entry to the KnownDA cache                                       */
/*=========================================================================*/
{
    SLPDAEntry*     entry;
    
    entry = (SLPDAEntry*)dalist->head;
    while(entry)
    {
        if(SLPCompareString(addition->urllen,
                            addition->url,
                            entry->urllen,
                            entry->url) == 0)
        {
            /* entry already in the list */
            break;
        }   

        entry = (SLPDAEntry*) entry->listitem.next;
    }

    if(entry == 0)
    {
        entry = SLPDAEntryCreate(&(addition->daaddr),
                                 addition);
        if(entry == 0)
        {
            return -1;
        }   
        
        SLPListLinkHead(dalist,(SLPListItem*)entry);
    }

    return 0;
}


/*=========================================================================*/
int KnownDAListRemove(SLPList* dalist, SLPDAEntry* remove)
/* Remove an entry to the KnownDA cach                                     */
/*=========================================================================*/
{
    SLPDAEntry*     entry;
    
    entry = (SLPDAEntry*)dalist->head;
    while(entry)
    {
        if(SLPCompareString(remove->urllen,
                            remove->url,
                            entry->urllen,
                            entry->url) == 0)
        {
            /* Remove entry from list and free it */
            SLPDAEntryFree( (SLPDAEntry*)SLPListUnlink(dalist, (SLPListItem*)entry) );
            break;
        }

        entry = (SLPDAEntry*) entry->listitem.next;
    }

    return 0;
}

/*=========================================================================*/
int KnownDAListRemoveAll(SLPList* dalist)
/* Remove all the entries of the given list                                */
/*=========================================================================*/
{
    SLPDAEntry*     del;
    SLPDAEntry*     entry = (SLPDAEntry*)dalist->head;
    while(entry)
    {
        del = entry;
        entry = (SLPDAEntry*) entry->listitem.next;
        SLPDAEntryFree( (SLPDAEntry*)SLPListUnlink(dalist, (SLPListItem*)del) );
    }

    dalist->head = 0;
    dalist->tail = 0;
    dalist->count = 0;

    return 0;
}


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

/*=========================================================================*/
int SLPDAEntryWrite(int fd, SLPDAEntry* entry)
/*=========================================================================*/
{
    int byteswritten;
    int size = entry->scopelistlen + sizeof(SLPDAEntry);
    byteswritten = write(fd,&size,sizeof(size));
    if(byteswritten == 0) return 0;
    byteswritten = write(fd,entry,size);
    return byteswritten;
}

/*=========================================================================*/
SLPDAEntry* SLPDAEntryRead(int fd)
/*=========================================================================*/
{   
    int bytesread;
    int size;
    char* curpos;
    SLPDAEntry* entry;

    bytesread = read(fd,&size,sizeof(size));
    if(bytesread == 0) return 0;
    if(size > 4096) return 0;  /* paranoia */

    entry = malloc(size + sizeof(SLPDAEntry));
    if(entry == 0) return 0;

    bytesread = read(fd,entry,size);
    if(bytesread < size)
    {
        free(entry);
        return 0;
    }

    curpos = (char*)(entry + 1);
    entry->langtag = curpos;
    curpos = curpos + entry->langtaglen;
    entry->url = curpos;
    curpos = curpos + entry->urllen;
    entry->scopelist = curpos;
    curpos = curpos + entry->scopelistlen;
    entry->attrlist = curpos;
    curpos = curpos + entry->attrlistlen;
    entry->spilist = curpos;
    
    return entry;
}
