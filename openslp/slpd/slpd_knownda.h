/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_knownda.h                                             */
/*                                                                         */
/* Abstract:    Keeps track of known DAs                                   */
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

#ifndef SLPD_KNOWNDA_H_INCLUDED
#define SLPD_KNOWNDA_H_INCLUDED

#include "slpd.h"

/*=========================================================================*/
/* common code includes                                                    */
/*=========================================================================*/
#include "../common/slp_buffer.h"
#include "../common/slp_message.h"


/*=========================================================================*/
int SLPDKnownDAInit();
/* Initializes the KnownDA list.  Removes all entries and adds entries     */
/* that are statically configured.                                         */
/*                                                                         */
/* returns  zero on success, Non-zero on failure                           */
/*=========================================================================*/


/*=========================================================================*/
int SLPDKnownDADeinit();
/* Deinitializes the KnownDA list.  Removes all entries and deregisters    */
/* all services.                                                           */
/*                                                                         */
/* returns  zero on success, Non-zero on failure                           */
/*=========================================================================*/


/*=========================================================================*/
int SLPDKnownDAAdd(SLPMessage msg, SLPBuffer buf);
/* Adds a DA to the known DA list if it is new, removes it if DA is going  */
/* down or adjusts entry if DA changed.                                    */
/*                                                                         */
/* msg     (IN) DAAdvert Message descriptor                                */
/*                                                                         */
/* buf     (IN) The DAAdvert message buffer                                */
/*                                                                         */
/* returns  Zero on success, Non-zero on error                             */
/*=========================================================================*/


/*=========================================================================*/
void SLPDKnownDARemove(struct in_addr* addr);
/* Removes known DAs that sent DAAdverts from the specified in_addr        */
/*=========================================================================*/

/*=========================================================================*/
void* SLPDKnownDAEnumStart();
/* Start an enumeration of all Known DAs                                   */
/*                                                                         */
/* Returns: An enumeration handle that is passed to subsequent calls to    */
/*          SLPDKnownDAEnum().  Returns NULL on failure.  Returned         */
/*          enumeration handle (if not NULL) must be passed to             */
/*          SLPDKnownDAEnumEnd() when you are done with it.                */
/*=========================================================================*/


/*=========================================================================*/
SLPMessage SLPDKnownDAEnum(void* eh, SLPMessage* msg, SLPBuffer* buf);
/* Enumerate through all Known DAs                                         */
/*                                                                         */
/* eh (IN) pointer to opaque data that is used to maintain                 */
/*         enumerate entries.  Pass in a pointer to NULL to start          */
/*         enumeration.                                                    */
/*                                                                         */
/* msg (OUT) pointer to the DAAdvert message descriptor                    */
/*                                                                         */
/* buf (OUT) pointer to the DAAdvert message buffer                        */
/*                                                                         */
/* returns: Pointer to enumerated entry or NULL if end of enumeration      */
/*=========================================================================*/


/*=========================================================================*/
void SLPDKnownDAEnumEnd(void* eh);
/* End an enumeration started by SLPDKnownDAEnumStart()                    */
/*                                                                         */
/* Parameters:  eh (IN) The enumeration handle returned by                 */
/*              SLPDKnownDAEnumStart()                                     */
/*=========================================================================*/


/*=========================================================================*/
int SLPDKnownDAGenerateMyDAAdvert(int errorcode,
                                  int deadda,
                                  int xid,
                                  SLPBuffer* sendbuf);
/* Pack a buffer with a DAAdvert using information from a SLPDAentry       */
/*                                                                         */
/* errorcode (IN) the errorcode for the DAAdvert                           */
/*                                                                         */
/* xid (IN) the xid to for the DAAdvert                                    */
/*                                                                         */
/* daentry (IN) pointer to the daentry that contains the rest of the info  */
/*              to make the DAAdvert                                       */
/*                                                                         */
/* sendbuf (OUT) pointer to the SLPBuffer that will be packed with a       */
/*               DAAdvert                                                  */
/*                                                                         */
/* returns: zero on success, non-zero on error                             */
/*=========================================================================*/


#if defined(ENABLE_SLPv1)
/*=========================================================================*/
int SLPDKnownDAGenerateMyV1DAAdvert(int errorcode,
                                    int encoding,
                                    unsigned int xid,
                                    SLPBuffer* sendbuf);
/* Pack a buffer with a v1 DAAdvert using information from a SLPDAentry    */
/*                                                                         */
/* errorcode (IN) the errorcode for the DAAdvert                           */
/*                                                                         */
/* encoding (IN) the SLPv1 language encoding for the DAAdvert              */
/*                                                                         */
/* xid (IN) the xid to for the DAAdvert                                    */
/*                                                                         */
/* sendbuf (OUT) pointer to the SLPBuffer that will be packed with a       */
/*               DAAdvert                                                  */
/*                                                                         */
/* returns: zero on success, non-zero on error                             */
/*=========================================================================*/
#endif


/*=========================================================================*/
void SLPDKnownDAEcho(SLPMessage msg, SLPBuffer buf);
/* Echo a srvreg message to a known DA                                     */
/*									                                       */
/* msg (IN) the SrvReg message descriptor                                  */
/*                                                                         */
/* buf (IN) the SrvReg message buffer to echo                              */
/*                                                                         */
/* Returns:  none                                                          */
/*=========================================================================*/


/*=========================================================================*/
void SLPDKnownDAActiveDiscovery(int seconds);
/* Add a socket to the outgoing list to do active DA discovery SrvRqst     */
/*									                                       */
/* Returns:  none                                                          */
/*=========================================================================*/


/*=========================================================================*/
void SLPDKnownDAPassiveDAAdvert(int seconds, int dadead);
/* Send passive daadvert messages if properly configured and running as    */
/* a DA                                                                    */
/*	                                                                       */
/* seconds (IN) number seconds that elapsed since the last call to this    */
/*              function                                                   */
/*                                                                         */
/* dadead  (IN) nonzero if the DA is dead and a bootstamp of 0 should be   */
/*              sent                                                       */
/*                                                                         */
/* Returns:  none                                                          */
/*=========================================================================*/


/*=========================================================================*/
void SLPDKnownDAImmortalRefresh(int seconds);
/* Refresh all SLP_LIFETIME_MAXIMUM services                               */
/* 																		   */
/* seconds (IN) time in seconds since last call                            */
/*=========================================================================*/

#endif 
