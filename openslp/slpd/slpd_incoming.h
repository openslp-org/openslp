/*-------------------------------------------------------------------------
 * Copyright (C) 2000 Caldera Systems, Inc
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    Neither the name of Caldera Systems nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CALDERA
 * SYSTEMS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *-------------------------------------------------------------------------*/

/** Header file for incoming network conversations.
 *
 * @file       slpd_incoming.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#ifndef SLPD_INCOMING_H_INCLUDE
#define SLPD_INCOMING_H_INCLUDE

/*!@defgroup SlpdCodeInComing Incoming Conversations */

/*!@addtogroup SlpdCodeInComing
 * @ingroup SlpdCode
 * @{
 */

#include "slp_types.h"
#include "slp_linkedlist.h"
#include "slpd.h"

extern SLPList G_IncomingSocketList;

void SLPDIncomingAge(time_t seconds);
int SLPDIncomingAddService(const char * srvtype, size_t len, 
      struct sockaddr_storage * localaddr);
int SLPDIncomingRemoveService(const char * srvtype, size_t len);
void SLPDIncomingHandler(int * fdcount, fd_set * readfds, fd_set * writefds);
int SLPDIncomingInit(void);
int SLPDIncomingDeinit(void);
int SLPDIncomingReinit(void);

#ifdef DEBUG
void SLPDIncomingSocketDump(void);
#endif

/*! @} */

#endif   /* SLPD_INCOMING_H_INCLUDE */

/*=========================================================================*/
