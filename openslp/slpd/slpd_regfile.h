/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_regfile.c                                             */
/*                                                                         */
/* Abstract:    Reads service registrations from a file                    */
/*                                                                         */
/* WARNING:     NOT thread safe!                                           */
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

#ifndef SLP_REGFILE_H_INCLUDE
#define SLP_REGFILE_H_INCLUDE

#include "slpd.h"

/*=========================================================================*/
/* common code includes                                                    */
/*=========================================================================*/
#include "../common/slp_buffer.h"
#include "../common/slp_message.h"


/*=========================================================================*/
int SLPDRegFileReadSrvReg(FILE* fd,
                          SLPMessage* msg,
                          SLPBuffer* buf);
/* A really big and nasty function that reads an service registration from */
/* from a file. Don't look at this too hard or you'll be sick.  This is by */
/* the most horrible code in OpenSLP.  Please volunteer to rewrite it!     */
/*                                                                         */
/*  "THANK GOODNESS this function is only called at startup" -- Matt       */
/*                                                                         */
/*                                                                         */
/* fd       (IN) file to read from                                         */
/*                                                                         */
/* msg      (OUT) message describing the SrvReg in buf                     */
/*                                                                         */
/* buf      (OUT) buffer containing the SrvReg                             */
/*                                                                         */
/* Returns:  zero on success. > 0 on error.  < 0 if EOF                    */
/*                                                                         */
/* Note:    Eventually the caller needs to call SLPBufferFree() and        */
/*          SLPMessageFree() to free memory                                */
/*=========================================================================*/


#endif
