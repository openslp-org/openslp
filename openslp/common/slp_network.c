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

/** Functions related to network and ipc communication.
 *
 * These routines are specific to stream-oriented (TCP) message transfer.
 *
 * @file       slp_network.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#include "slp_network.h"
#include "slp_net.h"

/** Connect a TCP stream to the specified peer.
 *
 * @param[in] peeraddr - A pointer to the peer to connect to.
 * @param[in] timeout - The maximum time to spend connecting.
 *
 * @return A connected socket, or SLP_INVALID_SOCKET on error.
 */
int SLPNetworkConnectStream(struct sockaddr_storage * peeraddr,
      struct timeval * timeout)
{
#ifdef _WIN32
   char lowat;
#else
   int lowat;
#endif

   int result;

   /** @todo Make the socket non-blocking so we can timeout on connect. */
   if (peeraddr->ss_family == AF_INET6)
      result = socket(PF_INET6, SOCK_STREAM, 0);
   else if (peeraddr->ss_family == AF_INET)
      result = socket(PF_INET, SOCK_STREAM, 0);
   else
      return -1;

   if (result >= 0)
   {
      if (connect(result, (struct sockaddr *)peeraddr,
            sizeof(struct sockaddr_storage)) == 0)
      {
         /* set the receive and send buffer low water mark to 18 bytes 
          * (the length of the smallest slpv2 message) 
          */
         lowat = 18;
         setsockopt(result,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
         setsockopt(result,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat));
         return result;
      }
      else
      {
#ifdef _WIN32 
         closesocket(result);
#else
         close(result);
#endif
         result = -1;
      }
   }
   return result;
}

/** Sends a message.
 *
 * @param[in] sockfd - The pre-configured socket to send on.
 * @param[in] socktype - The type of message (datagram or stream).
 * @param[in] buf - The buffer to be sent.
 * @param[in] peeraddr - The address to which @p buf should be sent.
 * @param[in] timeout - The maximum time to wait for network operations.
 *
 * @return Zero on success non-zero on failure with errno set. Possible 
 *    values for errno include EPIPE on write error, and ETIMEOUT on 
 *    read timeout error.
 */
int SLPNetworkSendMessage(int sockfd, int socktype, SLPBuffer buf, 
      struct sockaddr_storage * peeraddr, struct timeval * timeout)
{
   fd_set writefds;
   int xferbytes;
   int flags = 0;

#if defined(MSG_NOSIGNAL)
   flags = MSG_NOSIGNAL;
#endif

   buf->curpos = buf->start;

   while (buf->curpos < buf->end)
   {
      FD_ZERO(&writefds);
      FD_SET((unsigned int) sockfd, &writefds);

      xferbytes = select(sockfd + 1, 0, &writefds, 0, timeout);
      if (xferbytes > 0)
      {
         if (socktype == SOCK_DGRAM)
            xferbytes = sendto(sockfd, buf->curpos, buf->end - buf->curpos,
                  flags, (struct sockaddr *)peeraddr,
                  sizeof(struct sockaddr_storage));
         else
            xferbytes = send(sockfd, buf->curpos, buf->end - buf->curpos, 
                  flags);

         if (xferbytes > 0)
            buf->curpos = buf->curpos + xferbytes;
         else
         {
            errno = EPIPE;
            return -1;
         }
      }
      else if (xferbytes == 0)
      {
         errno = ETIMEDOUT;
         return -1;
      }
      else
      {
         errno = EPIPE;
         return -1;
      }
   }
   return 0;
}

/** Receives a message.
 *
 * @param[in] sockfd - The pre-configured socket on which to read.
 * @param[in] socktype - The socket type (stream or datagram).
 * @param[out] buf - The buffer into which a message should be read.
 * @param[out] peeraddr - The address of storage for the remote address.
 * @param[in] timeout - The maximum time to wait for network operations.
 *
 * @return Zero on success, or non-zero on failure with errno set. Values
 *    for errno include ENOTCONN on read error, ETIMEOUT on network timeout,
 *    ENOMEM on out-of-memory error, and EINVAL on parse error.
 */
int SLPNetworkRecvMessage(int sockfd, int socktype, SLPBuffer * buf,
      struct sockaddr_storage * peeraddr, struct timeval * timeout)
{
   int xferbytes;
   fd_set readfds;
   char peek[16];
   int peeraddrlen = sizeof(struct sockaddr_storage);

   /*---------------------------------------------------------------*/
   /* take a peek at the packet to get version and size information */
   /*---------------------------------------------------------------*/
   FD_ZERO(&readfds);
   FD_SET((unsigned int) sockfd, &readfds);
   xferbytes = select(sockfd + 1, &readfds, 0 , 0, timeout);
   if (xferbytes > 0)
   {
      if (socktype == SOCK_DGRAM)
         xferbytes = recvfrom(sockfd, peek, sizeof(peek), MSG_PEEK,
               (struct sockaddr *)peeraddr, &peeraddrlen);
      else
         xferbytes = recv(sockfd, peek, sizeof(peek), MSG_PEEK);

      if (xferbytes <= 0)
      {
#ifdef _WIN32
         if (WSAGetLastError() != WSAEMSGSIZE)
         {
            errno = ENOTCONN;
            return -1;
         }
#else
         errno = ENOTCONN;
         return -1;
#endif
      }
   }
   else if (xferbytes == 0)
   {
      errno = ETIMEDOUT;
      return -1;
   }
   else
   {
      errno = ENOTCONN;
      return -1;
   }

   /*------------------------------*/
   /* Read the rest of the message */
   /*------------------------------*/
   /* check the version */
   if (*peek == 2)
   {
      /* allocate the recvmsg big enough for the whole message */
      *buf = SLPBufferRealloc(*buf, AsUINT24(peek + 2));
      if (*buf)
      {
         while ((*buf)->curpos < (*buf)->end)
         {
            FD_ZERO(&readfds);
            FD_SET((unsigned int) sockfd, &readfds);
            xferbytes = select(sockfd + 1, &readfds, 0 , 0, timeout);
            if (xferbytes > 0)
            {
               xferbytes = recv(sockfd, (*buf)->curpos, 
                     (*buf)->end - (*buf)->curpos, 0);
               if (xferbytes > 0)
                  (*buf)->curpos = (*buf)->curpos + xferbytes;
               else
               {
                  errno = ENOTCONN;
                  return -1;
               }
            }
            else if (xferbytes == 0)
            {
               errno = ETIMEDOUT;
               return -1;
            }
            else
            {
               errno =  ENOTCONN;
               return -1;
            }
         }
      }
      else
      {
         errno = ENOMEM;
         return -1;
      }
   }
   else
   {
      errno = EINVAL;
      return -1;
   }
   return 0;
}

/*=========================================================================*/
