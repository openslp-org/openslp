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
void SLPDAEntryFree(SLPDAEntry* entry)
/* Frees a SLPDAEntry                                                      */
/*                                                                         */
/* entry    (IN) pointer to entry to free                                  */
/*                                                                         */
/* returns  none                                                           */
/*=========================================================================*/
{
    if(entry->daadvert.url) free((void*)entry->daadvert.url);    
    if(entry->daadvert.scopelist) free((void*)entry->daadvert.scopelist);
    if(entry->daadvert.attrlist) free((void*)entry->daadvert.attrlist);
    if(entry->daadvert.spilist) free((void*)entry->daadvert.spilist);
    free((void*)entry);
}


/*=========================================================================*/
SLPDAEntry* SLPDAEntryCreate(struct in_addr* addr, SLPDAAdvert* daadvert)
/* Creates a SLPDAEntry                                                    */
/*                                                                         */
/* addr     (IN) pointer to in_addr of the DA                              */
/*                                                                         */
/* daadvert (IN) pointer to a daadvert of the DA                           */
/*                                                                         */
/* returns  Pointer to new SLPDAEntry.                                     */
/*=========================================================================*/
{
    SLPDAEntry* result;
     
    result = (SLPDAEntry*)malloc(sizeof(SLPDAEntry));
    if(result == 0)
    {
        return 0;
    }
    memset(result,0,sizeof(SLPDAEntry));
    
    result->daaddr = *addr;
    memcpy(&(result->daadvert),daadvert,sizeof(SLPDAAdvert));
    result->daadvert.url = memdup(result->daadvert.url,
                                  result->daadvert.urllen);
    result->daadvert.scopelist = memdup(result->daadvert.scopelist,
                                        result->daadvert.scopelistlen);
    result->daadvert.attrlist = memdup(result->daadvert.attrlist,
                                       result->daadvert.attrlistlen);
    result->daadvert.spilist = memdup(result->daadvert.spilist,
                                      result->daadvert.spilistlen);
    result->daadvert.authcount = 0;
    result->daadvert.autharray = 0;
    /* IGNORE DUPLICATION OF AUTHBLOCK BECAUSE IT WON'T BE USED*/

    if(result->daadvert.url == 0 ||
       result->daadvert.scopelist == 0  ||
       result->daadvert.attrlist == 0 ||
       result->daadvert.spilist == 0)
    {
        SLPDAEntryFree(result);
        return 0;
    }
    
    return result;
}