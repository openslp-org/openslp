/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_xcast.h                                                */
/*                                                                         */
/* Abstract:    Functions used to multicast and broadcast SLP messages     */
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

#ifndef SLP_XCAST_H_INCLUDED
#define SLP_XCAST_H_INCLUDED

#include "slp_iface.h"
#include "slp_buffer.h"

typedef struct _SLPXcastSockets
{
    int                 sock_count;
    int                 sock[SLP_MAX_IFACES];
    struct sockaddr_storage  peeraddr[SLP_MAX_IFACES];
}SLPXcastSockets;


/*========================================================================*/
int SLPBroadcastSend(const SLPIfaceInfo* ifaceinfo, 
                     SLPBuffer msg,
                     SLPXcastSockets* socks);
/* Description:
 *    Broadcast a message.
 *
 * Parameters:
 *    ifaceinfo (IN) Pointer to the SLPIfaceInfo structure that contains
 *                   information about the interfaces to send on
 *    msg       (IN) Buffer to send
 *
 *   socks      (OUT) Sockets used broadcast multicast.  May be used to 
 *                    recv() responses.  MUST be close by caller using 
 *                    SLPXcastSocketsClose() 
 *
 * Returns:
 *    Zero on sucess.  Non-zero with errno set on error
 *========================================================================*/


/*========================================================================*/
int SLPMulticastSend(const SLPIfaceInfo* ifaceinfo, 
                     SLPBuffer msg,
                     SLPXcastSockets* socks);
/* Description:
 *    Multicast a message.
 *
 * Parameters:
 *    ifaceinfo (IN) Pointer to the SLPIfaceInfo structure that contains
 *                   information about the interfaces to send on
 *    msg       (IN) Buffer to send
 *
 *   socks      (OUT) Sockets used to multicast.  May be used to recv() 
 *                    responses.  MUST be close by caller using 
 *                    SLPXcastSocketsClose() 
 *
 * Returns:
 *    Zero on sucess.  Non-zero with errno set on error
 *========================================================================*/


/*=========================================================================*/
int SLPXcastRecvMessage(const SLPXcastSockets* sockets,
                        SLPBuffer* buf,
                        struct sockaddr_storage* peeraddr,
                        struct timeval* timeout);
/* Description: 
 *    Receives datagram messages from one of the sockets in the specified
 *    SLPXcastsSockets structure
 *  
 * Parameters:
 *    sockets (IN) Pointer to the SOPXcastSockets structure that describes
 *                 which sockets to read messages from.
 *    buf     (OUT) Pointer to SLPBuffer that will contain the message upon
 *                  successful return.
 *    peeraddr (OUT) Pointer to struc sockaddr that will contain the
 *                   address of the peer that sent the received message.
 *    timeout (IN/OUT) pointer to the struct timeval that indicates how much
 *                     time to wait for a message to arrive
 *
 * Returns:
 *    Zero on success, errno on failure.
 *========================================================================*/


/*========================================================================*/
int SLPXcastSocketsClose(SLPXcastSockets* socks);
/* Description:
 *    Closes sockets that were opened by calls to SLPMulticastSend() and
 *    SLPBroadcastSend()
 *
 * Parameters:
 *    socks (IN) Pointer to the SLPXcastSockets structure being close
 *
 * Returns:
 *    Zero on sucess.  Non-zero with errno set on error
 *========================================================================*/


#endif
