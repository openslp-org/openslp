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

/** Header file for multicast and broadcast SLP message functions.
 *
 * @file       slp_xcast.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#ifndef SLP_XCAST_H_INCLUDED
#define SLP_XCAST_H_INCLUDED

#include "slp_iface.h"
#include "slp_buffer.h"

/*!@defgroup CommonCodeXCast Multicast Messages */

/*!@addtogroup CommonCodeXCast
 * @ingroup CommonCode
 * @{
 */

typedef struct _SLPXcastSockets
{
   int sock_count;
   /*!< @brief The number of sock array.
    */
   int sock[SLP_MAX_IFACES];
   /*!< @brief An array of sockets managed by this structure.
    */
   struct sockaddr_storage peeraddr[SLP_MAX_IFACES];
   /*!< @brief An array of addresses that matches each socket in the sock 
    * array.
    */
} SLPXcastSockets;

int SLPBroadcastSend(const SLPIfaceInfo* ifaceinfo, 
                     SLPBuffer msg,
                     SLPXcastSockets* socks);

int SLPMulticastSend(const SLPIfaceInfo* ifaceinfo, 
                     SLPBuffer msg,
                     SLPXcastSockets* socks,
                     struct sockaddr_storage *dst);

int SLPXcastRecvMessage(const SLPXcastSockets* sockets,
                        SLPBuffer* buf,
                        struct sockaddr_storage* peeraddr,
                        struct timeval* timeout);

int SLPXcastSocketsClose(SLPXcastSockets* socks);

/*! @} */

#endif   /* SLP_XCAST_H_INCLUDED */

/*=========================================================================*/
