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

#include <netinet/in.h>

#include <slp_buffer.h>
#include <slp_message.h>
#include <slp_linkedlist.h>
#include <slp_network.h>
#include <slp_xid.h>

/*=========================================================================*/
typedef struct _SLPDAEntry
/*=========================================================================*/
{
    ListItem        listitem;
    SLPDAAdvert     daadvert;
    struct in_addr  daaddr;
}SLPDAEntry;

/*=========================================================================*/
void SLPDAEntryFree(SLPDAEntry* entry);
/* Frees a SLPDAEntry                                                      */
/*                                                                         */
/* entry    (IN) pointer to entry to free                                  */
/*                                                                         */
/* returns  none                                                           */
/*=========================================================================*/


/*=========================================================================*/
SLPDAEntry* SLPDAEntryCreate(struct in_addr* addr, SLPDAAdvert* daadvert);
/* Creates a SLPDAEntry                                                    */
/*                                                                         */
/* addr     (IN) pointer to in_addr of the DA                              */
/*                                                                         */
/* daadvert (IN) pointer to a daadvert of the DA                           */
/*                                                                         */
/* returns  Pointer to new SLPDAEntry.                                     */
/*=========================================================================*/


/*=========================================================================*/
int SLPDiscoverDAs(SLPDAEntry** dalisthead,
                   int scopelistlen,
                   const char* scopelist,
                   unsigned long flags,
                   const char* daaddresses,
                   struct timeval* timeout);
/* Discover all known DAs.  Use the net.slp.DAAddresses property, DHCP,    */
/* IPC with slpd, and finally active DA discovery via multicast.  The      */
/* list is NOT emptied before the newly discovered SLPDAEntrys are added.  */
/* Duplicate are discarded.                                                */
/*                                                                         */
/* dalisthead   (IN/OUT) pointer to the head of the list to add to         */
/*                                                                         */
/* scopelistlen (IN) length in bytes of scopelist                          */
/*                                                                         */
/* scopelist    (IN) find DAs of the specified scope.  NULL means find     */
/*                   DAs in all scopes                                     */
/*                                                                         */
/* flags        (IN) Discovery flags:                                      */
/*                   0x000000001 = First DA sufficient                     */
/*                   0x000000002 = First discovery method sufficient       */
/*                                                                         */
/* daaddresses  (IN) value of net.slp.DAAddresses                          */
/*                                                                         */
/* timeout      (IN) Maximum time to wait for discovery                    */
/*                                                                         */
/* Returns:     number of DAs found. -1 on error.                          */
/*                                                                         */
/* WARNING:     This function may block for a *long* time                  */
/*=========================================================================*/

#endif
