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

/** Functions used to multicast and broadcast SLP messages.
 *
 * The routines in this file are used by the OpenSLP UA, SA and DA code to
 * to send and receive SLPv2 broadcast and multicast messages.
 *
 * @file       slp_xcast.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeXCast
 */

#if !defined(UNICAST_NOT_SUPPORTED)
# include "../libslp/slp.h"
#endif

#include "slp_types.h"
#include "slp_xcast.h"
#include "slp_message.h"
#include "slp_v1message.h"
#include "slp_net.h"
#include "slp_network.h"
#include "slp_property.h"

/** Broadcast a message.
 *
 * @param[in] ifaceinfo - A pointer to the SLPIfaceInfo structure that
 *    contains information about the interfaces on which to send.
 * @param[in] msg - The buffer to be sent.
 * @param[out] socks - The address of storage for returning the sockets
 *    that were used to broadcast.
 *
 * @return Zero on sucess, or a non-zero value with @a errno set on error.
 *
 * @remarks The sockets returned in @p socks may be used to receive
 *    responses. Must be close by caller using SLPXcastSocketsClose.
 */
int SLPBroadcastSend(const SLPIfaceInfo * ifaceinfo,
      const SLPBuffer msg, SLPXcastSockets * socks)
{
   int xferbytes;
   so_bool_t on = 1;

   for (socks->sock_count = 0;
         socks->sock_count < ifaceinfo->bcast_count;
         socks->sock_count++)
   {
      if (ifaceinfo[socks->sock_count].bcast_addr->ss_family == AF_INET)
      {
         socks->sock[socks->sock_count] = socket(ifaceinfo[socks->sock_count]
               .bcast_addr->ss_family, SOCK_DGRAM, 0);

         if (socks->sock[socks->sock_count] == SLP_INVALID_SOCKET)
            return -1;  /* error creating socket */

         SLPNetworkSetSndRcvBuf(socks->sock[socks->sock_count]);

         if (setsockopt(socks->sock[socks->sock_count], SOL_SOCKET,
               SO_BROADCAST, &on, sizeof(on)) != 0)
            return -1;  /* Error setting socket option */

         memcpy(&socks->peeraddr[socks->sock_count],
               &ifaceinfo->bcast_addr[socks->sock_count],
               sizeof(ifaceinfo->bcast_addr[socks->sock_count]));

         SLPNetSetAddr(&socks->peeraddr[socks->sock_count], AF_INET,
               (uint16_t)SLPPropertyAsInteger("net.slp.port"), 0);
         xferbytes = sendto(socks->sock[socks->sock_count],
               (char *)msg->start, (int)(msg->end - msg->start), 0,
               (struct sockaddr *)&socks->peeraddr[socks->sock_count],
               SLPNetAddrLen(&socks->peeraddr[socks->sock_count]));
         if (xferbytes  < 0)
            return -1;  /* Error sending to broadcast */
      }
      else
         socks->sock[socks->sock_count] = 0; /* assume bcast for IPV4 only */
   }
   return 0;
}

/** Set the multicast interface for a socket. -- Copied from slpd_socket.c
 *
 * @param[in] sockfd - the socket file descriptor for which to set the multicast IF
 * @param[in] addr - A pointer to the address of the local network interface
 *
 * @return Zero on success, or a non-zero value on failure.
 *
 * @internal
 */
static int SetMulticastIF(int family, sockfd_t sockfd, const struct sockaddr_storage * addr)
{
   if (SLPNetIsIPV4() && ((family == AF_INET) || (family == AF_UNSPEC)))
      return setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF,
                        (char*)(&(((struct sockaddr_in*)addr)->sin_addr)), sizeof(struct in_addr));
   else if (SLPNetIsIPV6() && ((family == AF_INET6) || (family == AF_UNSPEC)))
      return setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                          (char*)(&((struct sockaddr_in6 *)addr)->sin6_scope_id), sizeof(unsigned int));
   return -1;
}

/** Set the socket options for ttl (time-to-live). -- Copied from slpd_socket.c
 *
 * @param[in] sockfd - the socket file descriptor for which to set TTL state.
 * @param[in] ttl - A boolean value indicating whether to enable or disable
 *    the time-to-live option.
 *
 * @return Zero on success, or a non-zero value on failure.
 *
 * @internal
 */
static int SetMulticastTTL(int family, sockfd_t sockfd, int ttl)
{
   if (SLPNetIsIPV4() && ((family == AF_INET) || (family == AF_UNSPEC)))
   {
#if defined(linux) || defined(_WIN32)
      int optarg = ttl;
#else
      /*Solaris and Tru64 expect an unsigned char parameter*/
      unsigned char optarg = (unsigned char)ttl;
#endif
      return setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&optarg, sizeof(optarg));
   }
   else if (SLPNetIsIPV6() && ((family == AF_INET6) || (family == AF_UNSPEC)))
   {
      int optarg = ttl;
      return setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (char *)&optarg, sizeof(optarg));
   }
   return -1;
}


/** Multicast a message.
 *
 * @param[in] ifaceinfo - A pointer to the SLPIfaceInfo structure that
 *    contains information about the interfaces on which to send.
 * @param[in] msg - The buffer to be sent.
 * @param[out] socks - The address of storage for the sockets that were used
 *    to multicast.
 * @param[in] dst - The target address, if using ipv6; can be null for IPv4.
 *
 * @return Zero on sucess, or a non-zero value, with errno set on error.
 *
 * @remarks May be used to receive responses. Must be close by caller using
 *    SLPXcastSocketsClose.
 */
int SLPMulticastSend(const SLPIfaceInfo * ifaceinfo, const SLPBuffer msg,
      SLPXcastSockets * socks, struct sockaddr_storage * dst)
{
   int flags = 0;
   int xferbytes;

#ifdef MSG_NOSIGNAL
   flags = MSG_NOSIGNAL;
#endif

   for (socks->sock_count = 0;
         socks->sock_count < ifaceinfo->iface_count;
         socks->sock_count++)
   {
      int family = ifaceinfo->iface_addr[socks->sock_count].ss_family;

      socks->sock[socks->sock_count] = socket(family, SOCK_DGRAM, 0);
      if((socks->sock[socks->sock_count] == SLP_INVALID_SOCKET) ||
         (SetMulticastIF(family, socks->sock[socks->sock_count], &ifaceinfo->iface_addr[socks->sock_count]) ||
         (SetMulticastTTL(family, socks->sock[socks->sock_count], SLPPropertyAsInteger("net.slp.multicastTTL")))))
         return -1; /* error creating socket or setting socket option */

      SLPNetworkSetSndRcvBuf(socks->sock[socks->sock_count]);
      memcpy(&socks->peeraddr[socks->sock_count], dst, sizeof(struct sockaddr_storage));

      xferbytes = sendto(socks->sock[socks->sock_count],
         (char *)msg->start, (int)(msg->end - msg->start), flags,
         (struct sockaddr *)&socks->peeraddr[socks->sock_count],
         SLPNetAddrLen(&socks->peeraddr[socks->sock_count]));
      if (xferbytes <= 0)
         return -1; /* error sending */
   }
   return 0;
}

/** Receives datagram messages.
 *
 * Receives messages from one of the sockets in the specified
 * SLPXcastsSockets structure, @p sockets.
 *
 * @param[in] sockets - A pointer to the SOPXcastSockets structure that
 *    describes the sockets from which to read messages.
 * @param[out] buf - A pointer to an SLPBuffer that will contain the
 *    received message upon successful return.
 * @param[out] peeraddr - A pointer to a sockaddr structure that will
 *    contain the address of the peer from which the message was received.
 * @param[in,out] timeout - A pointer to the timeval structure that
 *    indicates how much time to wait for a message to arrive.
 *
 * @return SLP_ERROR_OK on success, or a non-zero SLP_ERROR_* code
 *    optionally with errno set on failure.
 */
int SLPXcastRecvMessage(const SLPXcastSockets * sockets, SLPBuffer * buf,
      void * peeraddr, struct timeval * timeout)
{
#if HAVE_POLL
    struct pollfd readfds[SLP_MAX_IFACES];
#else
   fd_set readfds;
   sockfd_t highfd;
#endif
   int i;
   int readable;
   int bytesread;
   int recvloop;
   char peek[16];
   int result = SLP_ERROR_OK;
   unsigned mtu;
   unsigned int msglen;

   mtu = SLPPropertyGetMTU();
   /* recv loop */
   recvloop = 1;
   while (recvloop)
   {
#if HAVE_POLL
      for (i=0; i<sockets->sock_count; i++)
      {
         readfds[i].fd = sockets->sock[i];
         readfds[i].events = POLLIN;
         readfds[i].revents = 0;
      }
      readable = poll(readfds, sockets->sock_count, timeout ? timeout->tv_sec * 1000 + timeout->tv_usec / 1000 : -1);
#else
      /* Set the readfds */
      FD_ZERO(&readfds);
      highfd = 0;
      for (i = 0; i < sockets->sock_count; i++)
      {
         FD_SET(sockets->sock[i],&readfds);
         if (sockets->sock[i] > highfd)
            highfd = sockets->sock[i];
      }

      /* Select */
      readable = select((int)(highfd + 1), &readfds, 0, 0, timeout);
#endif
      if (readable > 0)
      {
         /* Read the datagram */
         for (i = 0; i < sockets->sock_count; i++)
         {
#if HAVE_POLL
            if ((readfds[i].revents & POLLIN) != 0)
#else
            if (FD_ISSET(sockets->sock[i], &readfds))
#endif
            {
               /* Peek at the first 16 bytes of the header */
               socklen_t peeraddrlen = sizeof(struct sockaddr_storage);
               bytesread = recvfrom(sockets->sock[i], peek, 16, MSG_PEEK,
                     peeraddr, &peeraddrlen);
               if (bytesread == 16
#ifdef _WIN32
                     /* Win32 returns WSAEMSGSIZE if the message is larger
                      * than the requested size, even with MSG_PEEK. But if
                      * this is the error code we can be sure that the
                      * message is at least 16 bytes
                      */
                     || (bytesread == -1 && WSAGetLastError() == WSAEMSGSIZE)
#endif
               )
               {
                  int ovlbit;
                  if (peek[0] == 1)
                  {
                     msglen = PEEK_LENGTH(peek);
                     ovlbit = peek[4] & SLPv1_FLAG_OVERFLOW;
                  }
                  else
                  {
                     msglen = PEEK_LENGTH(peek);
                     ovlbit = peek[5] & (SLP_FLAG_OVERFLOW >> 8);
                  }
                  if (msglen <= mtu && !ovlbit)
                  {
                     if ((*buf = SLPBufferRealloc(*buf, msglen)) == 0)
                        return SLP_MEMORY_ALLOC_FAILED;

                     bytesread = recv(sockets->sock[i], (char *)(*buf)->curpos,
                           (int)((*buf)->end - (*buf)->curpos), 0);

                     /* This should never happen but we'll be paranoid*/
                     if (bytesread != (int)msglen)
                        (*buf)->end = (*buf)->curpos + bytesread;

                     /* Message read. We're done! */
                     result = 0;
                     recvloop = 0;
                     break;
                  }
                  else
                  {
                     /* we got a bad message, or one that is too big! */
#ifndef UNICAST_NOT_SUPPORTED
                     /* Reading mtu bytes on the socket */
                     if (msglen > mtu)
                        msglen = mtu;
                     *buf = SLPBufferRealloc(*buf, msglen);

                     bytesread = recv(sockets->sock[i], (char *)(*buf)->curpos,
                           (int)((*buf)->end - (*buf)->curpos), 0);
                     /* This should never happen but we'll be paranoid*/
                     if (bytesread != (int)msglen)
                        (*buf)->end = (*buf)->curpos + bytesread;

                     result = SLP_ERROR_RETRY_UNICAST;
                     recvloop = 0;
                     return result;
#endif
                  }
               }
               else
               {
                  /* Not even 16 bytes available */
               }
            }
         }
      }
      else if (readable == 0)
      {
         result = SLP_NETWORK_TIMED_OUT;
         errno = ETIMEDOUT;
         recvloop = 0;
      }
      else
      {
         result = SLP_NETWORK_ERROR;
         recvloop = 0;
      }
   }
   return result;
}

/** Closes sockets.
 *
 * Closes the sockets in the @p socks parameter, which were previously
 * opened by calls to SLPMulticastSend and SLPBroadcastSend.
 *
 * @param[in,out] socks - A pointer to the SLPXcastSockets structure
 *    being closed.
 *
 * @return Zero on sucess, or a non-zero with errno set on error.
 */
int SLPXcastSocketsClose(SLPXcastSockets * socks)
{
   while (socks->sock_count)
   {
      socks->sock_count = socks->sock_count - 1;
      closesocket(socks->sock[socks->sock_count]);
   }
   return 0;
}

/*===========================================================================
 * TESTING CODE may be compiling with the following command line:
 *
 * $ gcc -g -DDEBUG -DSLP_XMIT_TEST slp_xcast.c slp_iface.c slp_buffer.c
 *    slp_linkedlist.c slp_compare.c slp_xmalloc.c
 */
/* #define SLP_XMIT_TEST */
#ifdef SLP_XMIT_TEST
int main(void)
{
   SLPIfaceInfo ifaceinfo;
   SLPIfaceInfo ifaceinfo6;
   SLPXcastSockets socks;
   SLPBuffer buffer;
   /* multicast srvloc address */
   uint8_t v6Addr[] = {0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x16};
   struct sockaddr_storage dst;
   int mtu;

   socks.sock = malloc(slp_max_ifaces*sizeof(sockfd_t));
   if (socks.sock == NULL)
   {
      fprintf(stderr, "socks.sock malloc(%d) failed\n",
         slp_max_ifaces*sizeof(sockfd_t));
      exit(1);
   }
   socks.peeraddr = malloc(slp_max_ifaces*sizeof(struct sockaddr_storage));
   if (socks.peeraddr == NULL)
   {
      fprintf(stderr, "socks.peeraddr malloc(%d) failed\n",
         slp_max_ifaces*sizeof(struct sockaddr_storage));
      exit(1);
   }
   ifaceinfo.iface_addr = malloc(slp_max_ifaces*sizeof(struct sockaddr_storage));
   if (ifaceinfo.iface_addr == NULL)
   {
      fprintf(stderr, "ifaceinfo.iface_addr malloc(%d) failed\n",
         slp_max_ifaces*sizeof(struct sockaddr_storage));
      exit(1);
   }
   ifaceinfo.bcast_addr = malloc(slp_max_ifaces*sizeof(struct sockaddr_storage));
   if (ifaceinfo.bcast_addr == NULL)
   {
      fprintf(stderr, "ifaceinfo.bcast_addr malloc(%d) failed\n",
         slp_max_ifaces*sizeof(struct sockaddr_storage));
      exit(1);
   }
   ifaceinfo6.iface_addr = malloc(slp_max_ifaces*sizeof(struct sockaddr_storage));
   if (ifaceinfo.bcast_addr == NULL)
   {
      fprintf(stderr, "ifaceinfo6.iface_addr malloc(%d) failed\n",
         slp_max_ifaces*sizeof(struct sockaddr_storage));
      exit(1);
   }
   ifaceinfo6.bcast_addr = malloc(slp_max_ifaces*sizeof(struct sockaddr_storage));
   if (ifaceinfo.bcast_addr == NULL) {
      fprintf(stderr, "ifaceinfo6.bcast_addr malloc(%d) failed\n",
         slp_max_ifaces*sizeof(struct sockaddr_storage));
      exit(1);
   }
#ifdef _WIN32
   WSADATA wsadata;
   WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif

   mtu = SLPPropertyGetMTU();
   //const struct in6_addr in6addr;
   buffer = SLPBufferAlloc(mtu);
   if (buffer)
   {
      strcpy((char *)buffer->start, "testdata");

      SLPIfaceGetInfo(NULL,&ifaceinfo, AF_INET);
      SLPIfaceGetInfo(NULL,&ifaceinfo6, AF_INET6);

      if (SLPBroadcastSend(&ifaceinfo, buffer, &socks) !=0)
         printf("\n SLPBroadcastSend failed \n");
      SLPXcastSocketsClose(&socks);

      /* for v6 */
      if (SLPBroadcastSend(&ifaceinfo6, buffer,&socks) !=0)
         printf("\n SLPBroadcastSend failed for ipv6\n");
      SLPXcastSocketsClose(&socks);

      if (SLPMulticastSend(&ifaceinfo, buffer, &socks, 0) !=0)
         printf("\n SLPMulticast failed \n");
      SLPXcastSocketsClose(&socks);

      /* set up address and scope for v6 multicast */
      SLPNetSetAddr(&dst, AF_INET6, 0, (uint8_t *)v6Addr);
      if (SLPMulticastSend(&ifaceinfo6, buffer, &socks, &dst) !=0)
         printf("\n SLPMulticast failed for ipv\n");
      SLPXcastSocketsClose(&socks);

      printf("Success\n");

      SLPBufferFree(buffer);
   }
   xfree(ifaceinfo.iface_addr);
   xfree(ifaceinfo.bcast_addr);
   xfree(ifaceinfo6.iface_addr);
   xfree(ifaceinfo6.bcast_addr);
   xfree(socks.sock);
   xfree(socks.peeraddr);

#ifdef _WIN32
   WSACleanup();
#endif

   return 0;
}

#endif

/*=========================================================================*/
