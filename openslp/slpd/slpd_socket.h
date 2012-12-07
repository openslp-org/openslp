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

/** Socket specific functions implementation.
 *
 * @file       slpd_socket.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#ifndef SLPD_SOCKET_H_INCLUDED
#define SLPD_SOCKET_H_INCLUDED

/*!@defgroup SlpdCodeSocket Socket Creation */

/*!@addtogroup SlpdCodeSocket
 * @ingroup SlpdCode
 * @{
 */

#include "slp_types.h"
#include "slp_buffer.h"
#include "slp_socket.h"
#include "slpd.h"

/* Misc constants */
#define SLPD_SMALLEST_MESSAGE       18   /* 18 bytes is smallest SLPv2 msg */

/* Values representing a type or state of a socket */
#define SOCKET_PENDING_IO       100
#define SOCKET_LISTEN           0
#define SOCKET_CLOSE            1
#define DATAGRAM_UNICAST        2
#define DATAGRAM_MULTICAST      3
#define DATAGRAM_BROADCAST      4
#define STREAM_CONNECT_IDLE     5
#define STREAM_CONNECT_BLOCK    6   + SOCKET_PENDING_IO
#define STREAM_CONNECT_CLOSE    7   + SOCKET_PENDING_IO
#define STREAM_READ             8   + SOCKET_PENDING_IO
#define STREAM_READ_FIRST       9   + SOCKET_PENDING_IO
#define STREAM_WRITE            10  + SOCKET_PENDING_IO
#define STREAM_WRITE_FIRST      11  + SOCKET_PENDING_IO
#define STREAM_WRITE_WAIT       12  + SOCKET_PENDING_IO

/** Structure representing a socket
 */
typedef struct _SLPDSocket
{
   SLPListItem listitem;    
   sockfd_t fd;
   time_t age;    /* in seconds -- in unicast dgram sockets, this also drives the resend logic */
   int state;
   int can_send_mcast;  /*Instead of allocating outgoing sockets to for sending multicast messages, slpd
                          uses incoming unicast sockets that were bound to the network interface.  Unicast
                          sockets are used because some stacks use the multicast address as the source address
                          if the socket was bound to the multicast address.  Since we don't want to send
                          mcast out of all the unicast sockets, this flag is used*/

   /* addrs related to the socket */
   struct sockaddr_storage localaddr;
   struct sockaddr_storage peeraddr;
   struct sockaddr_storage mcastaddr;

   /* Incoming socket stuff */
   SLPBuffer recvbuf;
   SLPBuffer sendbuf;

   /* Outgoing socket stuff */
   int reconns; /*For stream sockets, this drives reconnect.  For unicast dgram sockets, this drives resend*/
   SLPList sendlist;
#if HAVE_POLL
   int fdsetnr;
#endif
} SLPDSocket;

typedef struct _SLPD_fdset
{
#if HAVE_POLL
   struct pollfd * fds;
   int used;
   int allocated;
#else
   fd_set readfds;
   fd_set writefds;
   SOCKET highfd;
#endif
} SLPD_fdset;

#if HAVE_POLL

/* NOTE: the following code supports only one fdset at a time */

#define SLPD_fdset_readok(fdset, sock) ((sock)->fdsetnr != -1 && (sock)->fdsetnr < (fdset)->used && (fdset)->fds[(sock)->fdsetnr].fd == (sock)->fd ? ((fdset)->fds[(sock)->fdsetnr].revents & ((fdset)->fds[(sock)->fdsetnr].events & POLLIN ? (POLLIN|POLLERR|POLLHUP) : POLLIN)) != 0 : 0)
#define SLPD_fdset_writeok(fdset, sock) ((sock)->fdsetnr != -1 && (sock)->fdsetnr < (fdset)->used && (fdset)->fds[(sock)->fdsetnr].fd == (sock)->fd ? ((fdset)->fds[(sock)->fdsetnr].revents & ((fdset)->fds[(sock)->fdsetnr].events & POLLOUT ? (POLLOUT|POLLERR|POLLHUP) : POLLOUT)) != 0 : 0)

#define SLPD_fdset_reset(fdset) ((fdset)->used = 0)
#define SLPD_fdset_init(fdset) ((fdset)->used = (fdset)->allocated = 0, (fdset)->fds = 0)
#define SLPD_fdset_free(fdset) ((fdset)->allocated && xfree((fdset)->fds), SLPD_fdset_init(fdset))

#else

#define SLPD_fdset_readok(fdset, sock) (FD_ISSET((sock)->fd, &(fdset)->readfds))
#define SLPD_fdset_writeok(fdset, sock) (FD_ISSET((sock)->fd, &(fdset)->writefds))
#define SLPD_fdset_reset(fdset) do { FD_ZERO(&(fdset)->readfds); FD_ZERO(&(fdset)->writefds); (fdset)->highfd = 0; } while(0)
#define SLPD_fdset_init(fdset)
#define SLPD_fdset_free(fdset)

#endif

SLPDSocket * SLPDSocketCreateConnected(struct sockaddr_storage * addr);
SLPDSocket * SLPDSocketCreateListen(struct sockaddr_storage * peeraddr);
int SLPDSocketIsMcastOn(SLPDSocket * sock, struct sockaddr_storage * addr);
SLPDSocket * SLPDSocketCreateDatagram(struct sockaddr_storage * peeraddr, 
      int type); 
SLPDSocket * SLPDSocketCreateBoundDatagram(struct sockaddr_storage * myaddr,
      struct sockaddr_storage * peeraddr, int type);
int SLPDSocketAllowMcastSend(int family, SLPDSocket* psock);
SLPDSocket * SLPDSocketAlloc(void);
void SLPDSocketFree(SLPDSocket * sock);
void SLPDSocketSetSendRecvBuff(sockfd_t sock);
/*! @} */

#endif   /* SLPD_SOCKET_H_INCLUDED */

/*=========================================================================*/
