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
 * @ingroup    CommonCode
 */

#if !defined(UNICAST_NOT_SUPPORTED)
# include "../libslp/slp.h"
#endif

#include "slp_types.h"
#include "slp_xcast.h"
#include "slp_message.h"
#include "slp_net.h"

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

         if (setsockopt(socks->sock[socks->sock_count], SOL_SOCKET,
               SO_BROADCAST, &on, sizeof(on)) != 0)
            return -1;  /* Error setting socket option */

         memcpy(&socks->peeraddr[socks->sock_count], 
               &ifaceinfo->bcast_addr[socks->sock_count], 
               sizeof(ifaceinfo->bcast_addr[socks->sock_count]));

         SLPNetSetAddr(&socks->peeraddr[socks->sock_count], AF_INET, 
               SLP_RESERVED_PORT, 0);
         xferbytes = sendto(socks->sock[socks->sock_count],
               (char *)msg->start, (int)(msg->end - msg->start), 0, 
               (struct sockaddr *)&socks->peeraddr[socks->sock_count],
               sizeof(struct sockaddr_storage));
         if (xferbytes  < 0)
            return -1;  /* Error sending to broadcast */
      }
      else 
         socks->sock[socks->sock_count] = 0; /* assume bcast for IPV4 only */
   }
   return 0;
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
      SLPXcastSockets * socks, const void * dst)
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
      socks->sock[socks->sock_count] = socket(ifaceinfo->iface_addr
            [socks->sock_count].ss_family, SOCK_DGRAM, 0);
      if (socks->sock[socks->sock_count] == SLP_INVALID_SOCKET)
         return -1; /* error creating socket */
      memcpy(&socks->peeraddr[socks->sock_count], 
            &ifaceinfo->iface_addr[socks->sock_count],
            sizeof(ifaceinfo->iface_addr[socks->sock_count]));
   if (ifaceinfo->iface_addr[socks->sock_count].ss_family == AF_INET) 
   {
      struct sockaddr_in * s4 = (struct sockaddr_in *)
            &socks->peeraddr[socks->sock_count];
      if (setsockopt(socks->sock[socks->sock_count], IPPROTO_IP, 
            IP_MULTICAST_IF, (char *)&s4->sin_addr, sizeof(struct in_addr)))
         return -1; /* error setting socket option */
      s4->sin_family = AF_INET;
      s4->sin_port = htons(SLP_RESERVED_PORT);
      s4->sin_addr.s_addr = htonl(SLP_MCAST_ADDRESS);
   }
   else if (ifaceinfo->iface_addr[socks->sock_count].ss_family == AF_INET6) 
   {
      const struct sockaddr_in6 * s6dst = (const struct sockaddr_in6 *)dst;

      /* send via IPV6 multicast */
      if (bind(socks->sock[socks->sock_count], 
            (struct sockaddr *)&socks->peeraddr[socks->sock_count], 
            sizeof(struct sockaddr_storage)) != 0) 
         return -1;  /* error binding socket to address */
      if (s6dst->sin6_family == AF_INET6) 
         SLPNetSetAddr(&socks->peeraddr[socks->sock_count], AF_INET6, 
               SLP_RESERVED_PORT, &s6dst->sin6_addr);
      else 
         return -1;
   }
   else 
      return -1;  /* unknown family */

   xferbytes = sendto(socks->sock[socks->sock_count], 
         (char *)msg->start, (int)(msg->end - msg->start), flags, 
         (struct sockaddr *)&socks->peeraddr[socks->sock_count],
         sizeof(struct sockaddr_storage));
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
 * @return Zero on success, or a non-zero with errno set on failure.
 */
int SLPXcastRecvMessage(const SLPXcastSockets * sockets, SLPBuffer * buf,
      void * peeraddr, struct timeval * timeout)
{
   fd_set readfds;
   sockfd_t highfd;
   int i;
   int readable;
   int bytesread;
   int recvloop;
   char peek[16];
   int result = 0;

   /* recv loop */
   recvloop = 1;
   while (recvloop)
   {
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
      if (readable > 0)
      {
         /* Read the datagram */
         for (i = 0; i < sockets->sock_count; i++)
         {
            if (FD_ISSET(sockets->sock[i], &readfds))
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
                  if (AS_UINT24(peek + 2) <=  SLP_MAX_DATAGRAM_SIZE)
                  {
                     *buf = SLPBufferRealloc(*buf, AS_UINT24(peek + 2));
                     bytesread = recv(sockets->sock[i], (char *)(*buf)->curpos,
                           (int)((*buf)->end - (*buf)->curpos), 0);

                     /* This should never happen but we'll be paranoid*/
                     if (bytesread != (int)AS_UINT24(peek + 2))
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
                     /* Reading SLP_MAX_DATAGRAM_SIZE bytes on the socket */
                     *buf = SLPBufferRealloc(*buf, SLP_MAX_DATAGRAM_SIZE);
                     bytesread = recv(sockets->sock[i], (char *)(*buf)->curpos,
                           (int)((*buf)->end - (*buf)->curpos), 0);
                     /* This should never happen but we'll be paranoid*/
                     if (bytesread != SLP_MAX_DATAGRAM_SIZE)
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
         result = -1;
         errno = ETIMEDOUT;
         recvloop = 0;
      }
      else
      {
         result = -1;
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
void main(void)
{
   SLPIfaceInfo ifaceinfo;
   SLPIfaceInfo ifaceinfo6;
   SLPXcastSockets socks;
   SLPBuffer buffer;
   /* multicast srvloc address */
   BYTE v6Addr[] = {0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x16};
   struct sockaddr_storage dst;

#ifdef _WIN32
   WSADATA wsadata;
   WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif

   //const struct in6_addr in6addr;
   buffer = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
   if (buffer)
   {
      strcpy(buffer->start, "testdata");

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
      SLPNetSetAddr(&dst, AF_INET6, 0, (BYTE *)v6Addr, sizeof(v6Addr));
      if (SLPMulticastSend(&ifaceinfo6, buffer, &socks, &dst) !=0)
         printf("\n SLPMulticast failed for ipv\n");
      SLPXcastSocketsClose(&socks);

      printf("Success\n");

      SLPBufferFree(buffer);
   }

#ifdef _WIN32
   WSACleanup();
#endif
}

#endif

/*=========================================================================*/
