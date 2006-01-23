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

/** Header file that defines prototypes for SLPv1 messages.
 *
 * @file       slp_v1message.h
 * @author     Ganesan Rajagopal, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#ifndef SLP_V1MESSAGE_H_INCLUDED
#define SLP_V1MESSAGE_H_INCLUDED

/*!@defgroup CommonCodeMessageV1 SLPv1 Wire Message */

/*!@addtogroup CommonCodeMessageV1
 * @ingroup CommonCodeMessage
 * @{
 */

#include "slp_message.h"

/* SLP language encodings for SLPv1 compatibility */
#define SLP_CHAR_ASCII        3
#define SLP_CHAR_UTF8         106
#define SLP_CHAR_UNICODE16    1000
#define SLP_CHAR_UNICODE32    1001

/* SLPv1 Flags */
#define SLPv1_FLAG_OVERFLOW   0x80
#define SLPv1_FLAG_MONOLING   0x40
#define SLPv1_FLAG_URLAUTH    0x20
#define SLPv1_FLAG_ATTRAUTH   0x10
#define SLPv1_FLAG_FRESH      0x08

int SLPv1MessageParseHeader(const SLPBuffer buffer, SLPHeader * header);

int SLPv1MessageParseBuffer(const SLPBuffer buffer, SLPMessage message); 

/*! @} */

#endif   /* SLP_V1MESSAGE_H_INCLUDED */

/*=========================================================================*/
