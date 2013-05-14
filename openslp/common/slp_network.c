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
 * @ingroup    CommonCodeNetConn
 */

#include "slp_network.h"
#include "slp_net.h"

/**
 * Sets the SO_SNDBUF and SO_RCVBUF values on a given socket.
 *
 * @param[in] sock - the socket file descriptor for which to set the
 *                     SO_SNDBUF and SO_RCVBUF values.
 * @param[in] size - A pointer to the integer which specifies the buffer size.
 *                   If null, MTU value is used as buffer size.
 */
void SLPNetworkSetSndRcvBuf(sockfd_t sock)
{
#ifndef _WIN32
   int sndbufSize;
   int rcvBufSize;

   SLPPropertyInternalGetSndRcvBufSize(&sndbufSize, &rcvBufSize);
   if (sndbufSize)
   {
       setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sndbufSize, sizeof(int));
   }

   if (rcvBufSize)
   {
       setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvBufSize, sizeof(int));
   }
#else
   (void)sock;
#endif
}

/** Connect a TCP stream to the specified peer.
 *
 * @param[in] peeraddr - A pointer to the peer to connect to.
 * @param[in] timeout - The maximum time to spend connecting.
 *
 * @return A connected socket, or SLP_INVALID_SOCKET on error.
 */
sockfd_t SLPNetworkConnectStream(void * peeraddr,
      struct timeval * timeout)
{
   struct sockaddr * a = (struct sockaddr *)peeraddr;
   sockfd_t result;

   (void)timeout;
   /** @todo Make the socket non-blocking so we can timeout on connect. */

   result = socket(a->sa_family, SOCK_STREAM, IPPROTO_TCP);
   if (result != SLP_INVALID_SOCKET)
   {
      if (connect(result, peeraddr, sizeof(struct sockaddr_storage)) == 0)
      {
#ifndef _WIN32
         /* Set the receive and send buffer low water mark to 18 bytes 
          * (the length of the smallest slpv2 message). Note that Winsock
          * doesn't support these socket level options, so we skip them.
          */
         int lowat = 18;
         setsockopt(result, SOL_SOCKET, SO_RCVLOWAT, 
               (char *)&lowat, sizeof(lowat));
         setsockopt(result, SOL_SOCKET, SO_SNDLOWAT, 
               (char *)&lowat, sizeof(lowat));
#endif
         return result;
      }
      else
      {
         closesocket(result);
         result = SLP_INVALID_SOCKET;
      }
   }
   return result;
}

/** Creates a datagram socket
 *
 * @param[in] family -- the family (IPv4, IPv6 to create the socket in)
 *
 * @return A datagram socket, or SLP_INVALID_SOCKET on error.
 */
sockfd_t SLPNetworkCreateDatagram(short family)
{
   /*We don't attempt to call connect(), since it doesn't really connect and
     recvfrom will fail on some platforms*/
   sockfd_t result;

   result = socket(family, SOCK_DGRAM, IPPROTO_UDP);
   if (result != SLP_INVALID_SOCKET)
   {
#ifndef _WIN32
         /* Set the receive and send buffer low water mark to 18 bytes 
          * (the length of the smallest slpv2 message). Note that Winsock
          * doesn't support these socket level options, so we skip them.
          */
         int lowat = 18;
         setsockopt(result, SOL_SOCKET, SO_RCVLOWAT, 
               (char *)&lowat, sizeof(lowat));
         setsockopt(result, SOL_SOCKET, SO_SNDLOWAT, 
               (char *)&lowat, sizeof(lowat));

         SLPNetworkSetSndRcvBuf(result);
#endif
   }
   return result;
}

/** Sends a message.
 *
 * @param[in] sockfd - The pre-configured socket to send on.
 * @param[in] socktype - The type of message (datagram or stream).
 * @param[in] buf - The buffer to be sent.
 * @param[in] bufsz - The number of bytes in @p buf to be sent.
 * @param[in] peeraddr - The address to which @p buf should be sent.
 * @param[in] timeout - The maximum time to wait for network operations.
 *
 * @return Zero on success non-zero on failure with errno set. Possible 
 *    values for errno include EPIPE on write error, and ETIMEOUT on 
 *    read timeout error.
 */
int SLPNetworkSendMessage(sockfd_t sockfd, int socktype, 
      const SLPBuffer buf, size_t bufsz, void * peeraddr, 
      struct timeval * timeout)
{
#if HAVE_POLL
   struct pollfd writefd;
#else
   fd_set writefds;
#endif
   int xferbytes;
   int flags = 0;
   const uint8_t * cur = buf->start;
   const uint8_t * end = cur + bufsz;

#ifdef MSG_NOSIGNAL
   flags = MSG_NOSIGNAL;
#endif

   while (cur < end)
   {
#if HAVE_POLL
      writefd.fd = sockfd;
      writefd.events = POLLOUT;
      writefd.revents = 0;
      xferbytes = poll(&writefd, 1, timeout ? timeout->tv_sec * 1000 + timeout->tv_usec / 1000 : -1);
#else
      FD_ZERO(&writefds);
      FD_SET(sockfd, &writefds);

      xferbytes = select((int)sockfd + 1, 0, &writefds, 0, timeout);
#endif
      if (xferbytes > 0)
      {
         if (socktype == SOCK_DGRAM)
            xferbytes = sendto(sockfd, (char *)cur, 
                  (int)(end - cur), flags, peeraddr, 
                  SLPNetAddrLen(peeraddr));
         else
            xferbytes = send(sockfd, (char *)cur, 
                  (int)(end - cur), flags);

         if (xferbytes > 0)
            cur += xferbytes;
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
int SLPNetworkRecvMessage(sockfd_t sockfd, int socktype, 
      SLPBuffer * buf, void * peeraddr, struct timeval * timeout)
{
   int xferbytes, recvlen;
#if HAVE_POLL
   struct pollfd readfd;
#else
   fd_set readfds;
#endif
   char peek[16];

   /* Take a peek at the packet to get version and size information. */
#if HAVE_POLL
    readfd.fd = sockfd;
    readfd.events = POLLIN;
    readfd.revents = 0;
    xferbytes = poll(&readfd, 1, timeout ? timeout->tv_sec * 1000 + timeout->tv_usec / 1000 : -1);
#else
   FD_ZERO(&readfds);
   FD_SET(sockfd, &readfds);
   xferbytes = select((int)sockfd + 1, &readfds, 0, 0, timeout);
#endif
   if (xferbytes > 0)
   {
      if (socktype == SOCK_DGRAM)
      {
         socklen_t addrlen = sizeof(struct sockaddr_storage);
         xferbytes = recvfrom(sockfd, peek, 16, MSG_PEEK,
               peeraddr, &addrlen);
      }
      else
         xferbytes = recv(sockfd, peek, 16, MSG_PEEK);

      if (xferbytes <= 0)
      {
#ifdef _WIN32
         /* Winsock return WSAEMSGSIZE (even on MSG_PEEK!) if the amount of 
          * data queued on the socket is larger than the specified buffer. 
          * Just ignore it.
          */
         if (WSAGetLastError() == WSAEMSGSIZE)
		 {
			 xferbytes = 16;
		 }
		 else
#endif
         {
            errno = ENOTCONN;
            return -1;
         }
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

   /* Now check the version and read the rest of the message. */
   if (xferbytes >= 5 && (*peek == 1 || *peek == 2))
   {
      /* Allocate the receive buffer as large as necessary. */
      recvlen = PEEK_LENGTH(peek);
      *buf = SLPBufferRealloc(*buf, recvlen);
      if (*buf)
      {
         while ((*buf)->curpos < (*buf)->end)
         {
#if HAVE_POLL
            readfd.fd = sockfd;
            readfd.events = POLLIN;
            readfd.revents = 0;
            xferbytes = poll(&readfd, 1, timeout ? timeout->tv_sec * 1000 + timeout->tv_usec / 1000 : -1);
#else
            FD_SET(sockfd, &readfds);
            xferbytes = select((int)sockfd + 1, &readfds, 0, 0, timeout);
#endif
            if (xferbytes > 0)
            {
               xferbytes = recv(sockfd, (char *)(*buf)->curpos,
                     (int)((*buf)->end - (*buf)->curpos), 0);
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
               errno = ENOTCONN;
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

/** Convert a generic socket structure from network to presentation format.
 *
 * @param[in] src - A pointer to family-independent address information.
 * @param[out] dst - The address of storage for the formatted address string.
 * @param[in] dstsz - The size of @p dst in bytes.
 *
 * @returns A const pointer to @p dst on success; or NULL on failure, 
 *    and sets @a errno to EAFNOSUPPORT.
 */ 
const char * saddr_ntop(const void * src, char * dst, size_t dstsz)
{
   switch (((const struct sockaddr *)src)->sa_family)
   {
      case AF_INET:
      {
         const struct sockaddr_in * sin = (const struct sockaddr_in *)src;
         return inet_ntop(AF_INET, &sin->sin_addr, dst, dstsz);
      }
      case AF_INET6:
      {
         const struct sockaddr_in6 * sin6 = (const struct sockaddr_in6 *)src;
         return inet_ntop(AF_INET6, &sin6->sin6_addr, dst, dstsz);
      }
   }
   errno = EAFNOSUPPORT;
   return 0;
}

/*=========================================================================*/
