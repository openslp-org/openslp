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

