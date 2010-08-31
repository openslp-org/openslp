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

/** Header file for functions related to stream-oriented network transfer.
 *
 * @file       slp_network.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeNetConn
 */

#ifndef SLP_NETWORK_H_INCLUDED
#define SLP_NETWORK_H_INCLUDED

/*!@defgroup CommonCodeNetConn Connection
 * @ingroup CommonCodeNetwork
 * @{
 */

#include "slp_types.h"
#include "slp_buffer.h"
#include "slp_property.h"
#include "slp_message.h"
#include "slp_xid.h"

/** Multicast service type values for basic and DA operation. */
#define SLP_MULTICAST_SERVICE_TYPE_SRVLOC    0x01
#define SLP_MULTICAST_SERVICE_TYPE_SRVLOCDA  0x02

sockfd_t SLPNetworkConnectStream(void * peeraddr, struct timeval * timeout);  
sockfd_t SLPNetworkCreateDatagram(short family);
int SLPNetworkSendMessage(sockfd_t sockfd, int socktype, const SLPBuffer buf, 
      size_t bufsz, void * peeraddr, struct timeval * timeout);  
int SLPNetworkRecvMessage(sockfd_t sockfd, int socktype, SLPBuffer * buf,
      void * peeraddr, struct timeval * timeout); 
const char * saddr_ntop(const void * src, char * dst, size_t dstsz);

void SLPNetworkSetSndRcvBuf(sockfd_t sock);
/*! @} */

#endif   /* SLP_NETWORK_H_INCLUDED */

/*=========================================================================*/
