/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_v1message.h                                            */
/*                                                                         */
/* Abstract:    Header file that defines prototypes for SLPv1 messages     */
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

#if(!defined SLP_V1MESSAGE_H_INCLUDED)
#define SLP_V1MESSAGE_H_INCLUDED

#include "slp_message.h"

/*=========================================================================*/
/* SLP language encodings for SLPv1 compatibility                          */
/*=========================================================================*/
#define SLP_CHAR_ASCII          3
#define SLP_CHAR_UTF8           106
#define SLP_CHAR_UNICODE16      1000
#define SLP_CHAR_UNICODE32      1001

/*=========================================================================*/
/* SLPv1 Flags                                                             */
/*=========================================================================*/
#define SLPv1_FLAG_OVERFLOW         0x80
#define SLPv1_FLAG_MONOLING         0x40
#define SLPv1_FLAG_URLAUTH          0x20
#define SLPv1_FLAG_ATTRAUTH         0x10
#define SLPv1_FLAG_FRESH            0x08

/*=========================================================================*/
/* Prototypes for SLPv1 functions                                          */
/*=========================================================================*/

/*=========================================================================*/
extern int SLPv1MessageParseBuffer(struct sockaddr_storage* peerinfo,
                                   SLPBuffer buffer, 
                                   SLPMessage message); 
/* Initializes a SLPv1 message descriptor by parsing the specified buffer. */
/*                                                                         */
/* peerinfo - (IN pointer to information about where buffer came from      */
/*                                                                         */
/* buffer   - (IN) pointer the SLPBuffer to parse                          */
/*                                                                         */
/* message  - (OUT) set to describe the message from the buffer            */
/*                                                                         */
/* Returns  - Zero on success, SLP_ERROR_PARSE_ERROR, or                   */
/*            SLP_ERROR_INTERNAL_ERROR if out of memory.  SLPMessage is    */
/*            invalid return is not successful.                            */
/*                                                                         */
/* WARNING  - If successful, pointers in the SLPMessage reference memory in*/ 
/*            the parsed SLPBuffer.  If SLPBufferFree() is called then the */
/*            pointers in SLPMessage will be invalidated.                  */
/*=========================================================================*/


/*=========================================================================*/
extern int SLPv1MessageParseHeader(SLPBuffer buffer, SLPHeader* header);
/*                                                                         */
/* Returns  - Zero on success, SLP_ERROR_VER_NOT_SUPPORTED, or             */
/*            SLP_ERROR_PARSE_ERROR.                                       */
/*=========================================================================*/

#endif
