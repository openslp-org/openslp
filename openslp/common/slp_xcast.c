/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_xcast.c                                                */
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

#include "slp_xcast.h"

/*========================================================================*/
int SLPBroadcastSend(SLPInterfaceInfo* ifaceinfo, SLPBuffer msg);
/* Description:
 *    Broadcast a message.
 *
 * Parameters:
 *    ifaceinfo (IN) Pointer to the SLPInterfaceInfo structure that contains
 *                   information about the interfaces to send on
 *    msg       (IN) Buffer to send
 *
 * Returns:
 *    Zero on sucess.  Non-zero with errno set on error
 *========================================================================*/
{
    /* TODO:
     *    To be finished by Satya and Venu.
     *
     * HINTS:
     *    For each member of the ifaceinfo->bcast_addr (you know how many
     *    there are by looking at ifaceinfo->iface_count) you will need to
     *    call sendto().  Look at slp_network.c:244 to see how this is done
     *    currently.
     *
     *    Before you iterate through the ifaceinfo, you will need to create
     *    a SOCK_DGRAM socket and set the socket option for broadcast
     *    Look at slp_network.c:200 to see how this is done.
     *
     *    Note that the current implementation from slp_network.c only
     *    sends to 255.255.255.255 instead of the broadcast for each
     *    interface.
     *
     *    You need only create one socket, but you will have to call sendto()
     *    for each member of ifaceinfo->bcast_addr().  If any of the sockets
     *    calls fail, return -1 otherwise return zero.
     *
     *    By definition, datagram sockets never block on sendto().
     */
    return 0;
}


/*========================================================================*/
int SLPMulticastSend(SLPInterfaceInfo* ifaceinfo, SLPBuffer msg);
/* Description:
 *    Multicast a message.
 *
 * Parameters:
 *    ifaceinfo (IN) Pointer to the SLPInterfaceInfo structure that contains
 *                   information about the interfaces to send on
 *    msg       (IN) Buffer to send
 *
 * Returns:
 *    Zero on sucess.  Non-zero with errno set on error
 *========================================================================*/
 {
     /* TODO:
      *    To be finished by Satya and Venu
      *
      * HINTS:
      *    For each member of ifaceinfo->iface_addr (you know how many there
      *    are by looking at ifaceinfo->iface_count) you will need to create
      *    a socket, set the socket option for IP_MULTICAST_IF to be the
      *    ifaceinfo->iface_addr[x], call sendto(), and close the socket.
      *    The address you should send to is 239.255.255.253.
      *
      *    The loop should look something like this.
      *        set struct sockaddr_in peeraddr to be AF_INET, port 427, addr 239.255.255.253
      *        for every item in ifaceinfo->iface_addr
      *        begin loop
      *            create socket x
      *            set IP_MULTICAST_IF socket option for x to be ifaceinfo->iface_addr[x]
      *            call sendo() using x, msg, and peeraddr
      *            close socket x
      *        end loop
      *
      *    Please see current implementation at slp_network.c:109-170 for help.
      *    Note that the current implementation in slp_network.c does not set the
      *    IP_MULTICAST_IF socket option. 
      *
      *    If any of the sockets calls fail return -1 otherwise return zero.
      */

     return 0;
 }
