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

/** Header file for functions that keep track of known DA's.
 *
 * @file       slpd_knownda.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#ifndef SLPD_KNOWNDA_H_INCLUDED
#define SLPD_KNOWNDA_H_INCLUDED

/*!@defgroup SlpdCodeKnownDA Known DA Tracking */

/*!@addtogroup SlpdCodeKnownDA
 * @ingroup SlpdCode
 * @{
 */

#include "slp_types.h"
#include "slp_buffer.h"
#include "slp_message.h"
#include "slpd.h"

int SLPDKnownDAInit(void);
int SLPDKnownDADeinit(void);
int SLPDKnownDAAdd(SLPMessage * msg, SLPBuffer buf);
void SLPDKnownDARemove(struct sockaddr_storage * addr);
void * SLPDKnownDAEnumStart(void);
SLPMessage * SLPDKnownDAEnum(void * eh, SLPMessage ** msg, SLPBuffer * buf);
void SLPDKnownDAEnumEnd(void * eh);
int SLPDKnownDAGenerateMyDAAdvert(struct sockaddr_storage * localaddr,
      int errorcode, int deadda, int xid, SLPBuffer * sendbuf);

#if defined(ENABLE_SLPv1)
int SLPDKnownDAGenerateMyV1DAAdvert(struct sockaddr_storage * localaddr,
      int errorcode, int encoding, unsigned int xid, SLPBuffer * sendbuf);
#endif

void SLPDKnownDAEcho(SLPMessage * msg, SLPBuffer buf);
void SLPDKnownDAActiveDiscovery(int seconds);
void SLPDKnownDAPassiveDAAdvert(int seconds, int dadead);
void SLPDKnownDAImmortalRefresh(int seconds);
void SLPDKnownDADeRegisterWithAllDas(SLPMessage * msg, SLPBuffer buf);
void SLPDKnownDARegisterWithAllDas(SLPMessage * msg, SLPBuffer buf);

#ifdef DEBUG
void SLPDKnownDADump(void);
#endif

/*! @} */

#endif   /* SLPD_KNOWNDA_H_INCLUDED */

/*=========================================================================*/
