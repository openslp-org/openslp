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

/** Header file for slpd logging functions.
 *
 * @file       slpd_log.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#ifndef SLPD_LOG_H_INCLUDED
#define SLPD_LOG_H_INCLUDED

/*!@defgroup SlpdCodeLog Logging */

/*!@addtogroup SlpdCodeLog
 * @ingroup SlpdCode
 * @{
 */

#include "slpd.h"

#include "slp_message.h"
#include "slp_database.h"

#define SLPDLOG_TRACEMSG     0x80000000
#define SLPDLOG_TRACEDROP    0x40000000
#define SLPDLOG_TRACEMSG_IN  (SLPDLOG_TRACEMSG | 0x00000001)
#define SLPDLOG_TRACEMSG_OUT (SLPDLOG_TRACEMSG | 0x00000002)

int SLPDLogFileOpen(const char* path, int append);

#ifdef DEBUG
int SLPDLogFileClose();
#endif

void SLPDLogBuffer(const char* prefix, int bufsize, const char* buf);

void SLPDLog(const char* msg, ...);

void SLPDFatal(const char* msg, ...);

void SLPDLogTime();

void SLPDLogMessageInternals(SLPMessage message);

void SLPDLogMessage(int msglogflags,
                    struct sockaddr_storage* peerinfo,
                    struct sockaddr_storage* localaddr,
                    SLPBuffer buf);

void SLPDLogRegistration(const char* prefix, SLPDatabaseEntry* entry);

void SLPDLogDAAdvertisement(const char* prefix,
                            SLPDatabaseEntry* entry);

void SLPDLogParseWarning(struct sockaddr_storage* peeraddr, SLPBuffer buf);

/*! @} */

#endif   /* SLPD_LOG_H_INCLUDED */

/*=========================================================================*/
