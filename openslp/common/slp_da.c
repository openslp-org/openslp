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

/*=========================================================================*/
SLPDAEntry* SLPDAEntryCreate(struct in_addr* addr,
                             const char* scopelist,
                             int scopelistlen)
/* Creates a SLPDAEntry                                                    */
/*                                                                         */
/* addr     (IN) pointer to in_addr of the DA                              */
/*                                                                         */
/* daadvert (IN) pointer to a daadvert of the DA                           */
/*                                                                         */
/* returns  Pointer to new SLPDAEntry.                                     */
/*=========================================================================*/
{
    SLPDAEntry* entry;
    entry = (SLPDAEntry*)malloc(sizeof(SLPDAEntry)+scopelistlen);
    if(entry == 0) return 0;
    memset(entry,0,sizeof(SLPDAEntry)+scopelistlen);

    entry->daaddr = *addr;
    entry->scopelist = (char*)(entry+1);
    memcpy(entry->scopelist,scopelist,scopelistlen);
    entry->scopelistlen = scopelistlen;

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

    entry->scopelist = (char*)(entry+1);
  
    return entry;
}
