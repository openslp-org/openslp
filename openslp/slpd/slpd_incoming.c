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

/** Incoming request handler.
 *
 * Handles "incoming" network conversations requests made by other agents to 
 * slpd. (slpd_outgoing.c handles reqests made by slpd to other agents)
 *
 * @file       slpd_incoming.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#include "slpd_incoming.h"
#include "slpd_socket.h"
#include "slpd_process.h"
#include "slpd_property.h"
#include "slpd_log.h"

#include "slp_xmalloc.h"
#include "slp_message.h"
#include "slp_net.h"

SLPList G_IncomingSocketList =
{
   0, 0, 0
};

/** Read an inbound datagram.
 *
 * @param[in] socklist - The list of monitored sockets.
 * @param[in] sock - The socket to be read.
 *
 * @internal
 */
static void IncomingDatagramRead(SLPList * socklist, SLPDSocket * sock)
{
   int bytesread;
   int byteswritten;
   socklen_t peeraddrlen = sizeof(struct sockaddr_storage);
   char addr_str[INET6_ADDRSTRLEN];
   sockfd_t sendsock = SLP_INVALID_SOCKET;

   (void)socklist;

   bytesread = recvfrom(sock->fd, (char*)sock->recvbuf->start, 
         G_SlpdProperty.MTU, 0, (struct sockaddr *)&sock->peeraddr, 
         &peeraddrlen);
   if (bytesread > 0)
   {
      sock->recvbuf->end = sock->recvbuf->start + bytesread;

      if (!sock->sendbuf)
         /* Some of the error handling code expects a sendbuf to be available
          * to be emptied, so make sure there is at least a minimal buffer
          */
         sock->sendbuf = SLPBufferAlloc(1);

      switch (SLPDProcessMessage(&sock->peeraddr, &sock->localaddr,
            sock->recvbuf, &sock->sendbuf, 0))
      {
         case SLP_ERROR_PARSE_ERROR:
         case SLP_ERROR_VER_NOT_SUPPORTED:
         case SLP_ERROR_MESSAGE_NOT_SUPPORTED:
            break;                    

         default:
#ifdef DARWIN
            /* If the socket is a multicast socket, find the designated UDP output socket for sending*/
            if(DATAGRAM_MULTICAST == sock->state)
            {
               if ((sendsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) !=
                  SLP_INVALID_SOCKET)
               {
                  SLPNetworkSetSndRcvBuf(sendsock, 0);
               }
            }
#endif
            if(SLP_INVALID_SOCKET == sendsock)
               sendsock = sock->fd;

            /* check to see if we should send anything, breaking up individual packets in the buffer
               into different sendto calls (should only be an issue with the loopback DA response)*/
            sock->sendbuf->curpos = sock->sendbuf->start;
            while (sock->sendbuf->curpos < sock->sendbuf->end)
            {
               int packetbytes = PEEK_LENGTH(sock->sendbuf->curpos);

               byteswritten = sendto(sendsock, (char*)sock->sendbuf->curpos,
                     packetbytes, 0, (struct sockaddr *)&sock->peeraddr,
                     SLPNetAddrLen(&sock->peeraddr));

               if (byteswritten != packetbytes)
               {
                  /* May be an overflow reply */
                  int flags = AS_UINT16(sock->sendbuf->curpos + 5);
                  if ((byteswritten == -1) &&
#ifdef _WIN32
                      (WSAGetLastError() == WSAEMSGSIZE) &&
#else
                      (errno == EMSGSIZE) &&
#endif
                      (flags & SLP_FLAG_OVERFLOW))
                  {
                      int byteswrittenmax = sendto(sendsock, (char*)sock->sendbuf->curpos,
                                G_SlpdProperty.MTU, 0, (struct sockaddr *)&sock->peeraddr,
                                SLPNetAddrLen(&sock->peeraddr));
                      if (byteswrittenmax == G_SlpdProperty.MTU)
                         byteswritten = packetbytes;
                  }
               }

               if (byteswritten != packetbytes)
                  SLPDLog("NETWORK_ERROR - %d replying %s\n", errno,
                        SLPNetSockAddrStorageToString(&(sock->peeraddr),
                              addr_str, sizeof(addr_str)));

               sock->sendbuf->curpos += packetbytes;
            }

            if(sendsock != sock->fd)  /*Only close if we allocated a new socket*/
              closesocket(sendsock);

      }
   }
}

/** Write inbound stream data.
 *
 * @param[in] socklist - The list of monitored sockets.
 * @param[in] sock - The socket to be written.
 *
 * @internal
 */
static void IncomingStreamWrite(SLPList * socklist, SLPDSocket * sock)
{
   int byteswritten, flags = 0;

   (void)socklist;

#if defined(MSG_DONTWAIT)
   flags = MSG_DONTWAIT;
#endif

   if (sock->state == STREAM_WRITE_FIRST)
   {
      /* make sure that the start and curpos pointers are the same */
      sock->sendbuf->curpos = sock->sendbuf->start;
      sock->state = STREAM_WRITE;
   }

   if (sock->sendbuf->end - sock->sendbuf->curpos != 0)
   {
      byteswritten = send(sock->fd, (char *)sock->sendbuf->curpos,
            (int)(sock->sendbuf->end - sock->sendbuf->curpos), flags);
      if (byteswritten > 0)
      {
         /* reset lifetime to max because of activity */
         sock->age = 0;
         sock->sendbuf->curpos += byteswritten;
         if (sock->sendbuf->curpos == sock->sendbuf->end)
         {
            /* message is completely sent */
            sock->state = STREAM_READ_FIRST;
         }
      }
      else
      {
#ifdef _WIN32
         if (WSAEWOULDBLOCK != WSAGetLastError())
#else
         if (errno != EWOULDBLOCK)
#endif
            sock->state = SOCKET_CLOSE; /* Error or conn was closed */
      }
   }
   else
   {
      /* nothing to write */
#ifdef DEBUG
      SLPDLog("yikes, an empty message is being written!\n");
#endif
      sock->state = SOCKET_CLOSE;
   }
}

/** Read inbound stream data.
 *
 * @param[in] socklist - The list of monitored sockets.
 * @param[in] sock - The socket to be read.
 *
 * @internal
 */
static void IncomingStreamRead(SLPList * socklist, SLPDSocket * sock)
{
   int bytesread;
   size_t recvlen = 0;
   char peek[16];
   socklen_t peeraddrlen = sizeof(struct sockaddr_storage);

   if (sock->state == STREAM_READ_FIRST)
   {
      /*---------------------------------------------------*/
      /* take a peek at the packet to get size information */
      /*---------------------------------------------------*/
      bytesread = recvfrom(sock->fd, (char *)peek, 16, MSG_PEEK,
            (struct sockaddr *)&sock->peeraddr, &peeraddrlen);
      if (bytesread > 0)
      {
         recvlen = PEEK_LENGTH(peek);

         /* allocate the recvbuf big enough for the whole message */
         sock->recvbuf = SLPBufferRealloc(sock->recvbuf, recvlen);
         if (sock->recvbuf)
            sock->state = STREAM_READ;
         else
         {
            SLPDLog("INTERNAL_ERROR - out of memory!\n");
            sock->state = SOCKET_CLOSE;
         }
      }
      else
      {
         sock->state = SOCKET_CLOSE;
         return;
      }
   }

   if (sock->state == STREAM_READ)
   {
      /*------------------------------*/
      /* recv the rest of the message */
      /*------------------------------*/
      bytesread = recv(sock->fd, (char *)sock->recvbuf->curpos,
            (int)(sock->recvbuf->end - sock->recvbuf->curpos), 0);              

      if (bytesread > 0)
      {
         /* reset age to max because of activity */
         sock->age = 0;
         sock->recvbuf->curpos += bytesread;
         if (sock->recvbuf->curpos == sock->recvbuf->end) {
            if (!sock->sendbuf)
               /* Some of the error handling code expects a sendbuf to be available
                * to be emptied, so make sure there is at least a minimal buffer
                */
               sock->sendbuf = SLPBufferAlloc(1);
            switch (SLPDProcessMessage(&sock->peeraddr, 
                  &sock->localaddr, sock->recvbuf, &sock->sendbuf, 0))
            {
               case SLP_ERROR_PARSE_ERROR:
               case SLP_ERROR_VER_NOT_SUPPORTED:
               case SLP_ERROR_MESSAGE_NOT_SUPPORTED:
                  sock->state = SOCKET_CLOSE;
                  break;                    

               default:
                  sock->state = STREAM_WRITE_FIRST;
                  IncomingStreamWrite(socklist, sock);
            }
         }
      }
      else {
         sock->state = SOCKET_CLOSE; /* error in recv() */
      }
   }
}

/** Listen on an inbound socket.
 *
 * @param[in] socklist - The list of monitored sockets.
 * @param[in] sock - The socket on which to listen.
 *
 * @internal
 */
static void IncomingSocketListen(SLPList * socklist, SLPDSocket * sock)
{
   sockfd_t fd;
   SLPDSocket * connsock;
   struct sockaddr_storage peeraddr;
   socklen_t peeraddrlen;

   /* Only accept if we can. If we still maximum number of sockets, just*/
   /* ignore the connection */
   if (socklist->count < SLPD_MAX_SOCKETS)
   {
      peeraddrlen = sizeof(peeraddr);
      fd = accept(sock->fd, (struct sockaddr *)&peeraddr, &peeraddrlen);
      if (fd != SLP_INVALID_SOCKET)
      {
         connsock = SLPDSocketAlloc();
         if (connsock)
         {
            /* setup the accepted socket */
            connsock->fd = fd;
            memcpy(&connsock->peeraddr, &peeraddr,
                  sizeof(struct sockaddr_storage));
            memcpy(&connsock->localaddr, &peeraddr,
                  sizeof(struct sockaddr_storage));
            connsock->state = STREAM_READ_FIRST;
#ifndef _WIN32
            {
               /* Set the receive and send buffer low water mark to 18 bytes 
                * (the length of the smallest slpv2 message). Note that Winsock
                * doesn't support these socket level options, so we skip them.
                */
               int lowat = 18;
               setsockopt(connsock->fd, SOL_SOCKET, SO_RCVLOWAT, 
                     (char *)&lowat, sizeof(lowat));
               setsockopt(connsock->fd, SOL_SOCKET, SO_SNDLOWAT, 
                     (char *)&lowat, sizeof(lowat)); 
            }
#endif
            /* set accepted socket to non blocking */
#ifdef _WIN32
            {
               u_long fdflags = 1;
               ioctlsocket(connsock->fd, FIONBIO, &fdflags);
            }
#else
            {
               int fdflags = fcntl(connsock->fd, F_GETFL, 0);
               fcntl(connsock->fd, F_SETFL, fdflags | O_NONBLOCK);
            }
#endif        
            SLPListLinkHead(socklist, (SLPListItem *)connsock);
         }
      }
   }
}

/** Handles outgoing requests pending on the specified file discriptors.
 *
 * @param[in,out] fdcount - The number of file descriptors marked in fd_sets.
 * @param[in] readfds - The set of file descriptors with pending read IO.
 * @param[in] writefds - The set of file descriptors with pending read IO.
 */
void SLPDIncomingHandler(int * fdcount, fd_set * readfds, fd_set * writefds)
{
   SLPDSocket * sock;
   sock = (SLPDSocket *)G_IncomingSocketList.head;
   while (sock && *fdcount)
   {
      if (FD_ISSET(sock->fd, readfds))
      {
         switch (sock->state)
         {
            case SOCKET_LISTEN:
               IncomingSocketListen(&G_IncomingSocketList, sock);
               break;

            case DATAGRAM_UNICAST:
            case DATAGRAM_MULTICAST:
            case DATAGRAM_BROADCAST:
               IncomingDatagramRead(&G_IncomingSocketList, sock);
               break;                      

            case STREAM_READ:
            case STREAM_READ_FIRST:
               IncomingStreamRead(&G_IncomingSocketList, sock);
               break;

            default:
               break;
         }

         *fdcount = *fdcount - 1;
      }
      else if (FD_ISSET(sock->fd, writefds))
      {
         switch (sock->state)
         {
            case STREAM_WRITE:
            case STREAM_WRITE_FIRST:
               IncomingStreamWrite(&G_IncomingSocketList, sock);
               break;

            default:
               break;
         }

         *fdcount = *fdcount - 1;
      }

      sock = (SLPDSocket *) sock->listitem.next;
   }
}

/** Age the inbound socket list.
 *
 * @param[in] seconds - The number of seconds old an entry must be to be 
 *    removed from the inbound list.
 */
void SLPDIncomingAge(time_t seconds)
{
   SLPDSocket * del = 0;
   SLPDSocket * sock = (SLPDSocket *) G_IncomingSocketList.head;
   while (sock)
   {
      switch (sock->state)
      {
         case STREAM_READ_FIRST:
         case STREAM_READ:
         case STREAM_WRITE_FIRST:
         case STREAM_WRITE:
            if (G_IncomingSocketList.count > SLPD_COMFORT_SOCKETS)
            {
               /* Accellerate ageing cause we are low on sockets */
               if (sock->age > SLPD_CONFIG_BUSY_CLOSE_CONN)
                  del = sock;
            }
            else
            {
               if (sock->age > SLPD_CONFIG_CLOSE_CONN)
                  del = sock;
            }
            sock->age = sock->age + seconds;
            break;

         default:
            /* don't age the other sockets at all */
            break;
      }

      sock = (SLPDSocket *)sock->listitem.next;

      if (del)
      {
         SLPDSocketFree((SLPDSocket *)SLPListUnlink(
               &G_IncomingSocketList, (SLPListItem *)del));
         del = 0;
      }
   }
}

/** Add sockets that will listen for service requests of the given type.
 *
 * @param[in] srvtype - The service type to add.
 * @param[in] len - The length of @p srvtype.
 * @param[in] localaddr - The local address on which the message 
 *    was received.
 *
 * @return Zero on success, or a non-zero value on failure.
 *
 * @note This function only works for IPv6.
 */
int SLPDIncomingAddService(const char * srvtype, size_t len,
      struct sockaddr_storage * localaddr)
{
   struct sockaddr_storage srvaddr;
   SLPDSocket * sock;
   int res;
   char addr_str[INET6_ADDRSTRLEN];

   /* @todo On Unix platforms, this doesn't work due to a permissions problem.
    * When slpd starts, it's running as root and can do the subscribe. After
    * it has been daemonized, it can no longer bind to multicast addresses.
    * This works on windows*/ 

   /* create a listening socket for node-local service request queries */
   res = SLPNetGetSrvMcastAddr(srvtype, len, SLP_SCOPE_NODE_LOCAL, &srvaddr);
   if (res != 0)
      return -1;

   sock = SLPDSocketCreateBoundDatagram(localaddr, &srvaddr,
               DATAGRAM_MULTICAST);
   if (sock)
   {
      SLPListLinkTail(&G_IncomingSocketList, (SLPListItem *) sock);
      SLPDLog("Listening on %s...\n",
            inet_ntop(AF_INET6,
                  &(((struct sockaddr_in6 *) &srvaddr)->sin6_addr), addr_str,
                  sizeof(addr_str)));
   }
   else
   {
      SLPDLog("NETWORK_ERROR - Could not listen on %s.\n",
            inet_ntop(AF_INET6,
                  &(((struct sockaddr_in6 *) &srvaddr)->sin6_addr), addr_str,
                  sizeof(addr_str)));
      SLPDLog("INTERNAL_ERROR - No SLPLIB support will be available\n");
   }

   /* create a listening socket for link-local service request queries */
   res = SLPNetGetSrvMcastAddr(srvtype, len, SLP_SCOPE_LINK_LOCAL, &srvaddr);
   if (res != 0)
      return -1;

   sock = SLPDSocketCreateBoundDatagram(localaddr, &srvaddr,
               DATAGRAM_MULTICAST);
   if (sock)
   {
      SLPListLinkTail(&G_IncomingSocketList, (SLPListItem *) sock);
      SLPDLog("Listening on %s...\n",
            inet_ntop(AF_INET6,
                  &(((struct sockaddr_in6 *) &srvaddr)->sin6_addr), addr_str,
                  sizeof(addr_str)));
   }
   else
   {
      SLPDLog("NETWORK_ERROR - Could not listen on %s.\n",
            inet_ntop(AF_INET6,
                  &(((struct sockaddr_in6 *) &srvaddr)->sin6_addr), addr_str,
                  sizeof(addr_str)));
      SLPDLog("INTERNAL_ERROR - No SLPLIB support will be available\n");
   }

   if (!IN6_IS_ADDR_LINKLOCAL(&(((struct sockaddr_in6 *)localaddr)->sin6_addr)))
   {
      /* create a listening socket for site-local service request queries */
      res = SLPNetGetSrvMcastAddr(srvtype, len, SLP_SCOPE_SITE_LOCAL, &srvaddr);
      if (res != 0)
        return -1;
   
      sock = SLPDSocketCreateBoundDatagram(localaddr, &srvaddr,
               DATAGRAM_MULTICAST);
      if (sock)
      {
        SLPListLinkTail(&G_IncomingSocketList, (SLPListItem *) sock);
        SLPDLog("Listening on %s...\n",
            inet_ntop(AF_INET6,
                 &(((struct sockaddr_in6 *) &srvaddr)->sin6_addr), addr_str,
                 sizeof(addr_str)));
      }
      else
      {
        SLPDLog("NETWORK_ERROR - Could not listen on %s.\n",
            inet_ntop(AF_INET6,
                 &(((struct sockaddr_in6 *) &srvaddr)->sin6_addr), addr_str,
                 sizeof(addr_str)));
        SLPDLog("INTERNAL_ERROR - No SLPLIB support will be available\n");
      }
   }

   return 0;
}

/** Remove the sockets that listen for service requests of the given type.
 *
 * @param[in] srvtype - The service to be removed.
 * @param[in] len - The length of @p srvtype.
 *
 * @return Zero on success, or a non-zero value on failure.
 *
 * @note This function only works for IPv6.
 */
int SLPDIncomingRemoveService(const char * srvtype, size_t len)
{
   SLPDSocket * sock = NULL;
   SLPDSocket * sock_next = (SLPDSocket *) G_IncomingSocketList.head;
   struct sockaddr_storage srvaddr_node;
   struct sockaddr_storage srvaddr_link;
   struct sockaddr_storage srvaddr_site;
   int res;

   while (sock_next)
   {
      sock = sock_next;
      sock_next = (SLPDSocket *) sock->listitem.next;

      res = SLPNetGetSrvMcastAddr(srvtype, len, SLP_SCOPE_NODE_LOCAL,
                  &srvaddr_node);
      if (res != 0)
         return -1;
      res = SLPNetGetSrvMcastAddr(srvtype, len, SLP_SCOPE_LINK_LOCAL,
                  &srvaddr_link);
      if (res != 0)
         return -1;
      res = SLPNetGetSrvMcastAddr(srvtype, len, SLP_SCOPE_SITE_LOCAL,
                  &srvaddr_site);
      if (res != 0)
         return -1;

      if (sock
            && (SLPDSocketIsMcastOn(sock, &srvaddr_node)
            || SLPDSocketIsMcastOn(sock,
                     &srvaddr_link)
            || SLPDSocketIsMcastOn(sock,
                     &srvaddr_site)))
      {
         SLPDSocketFree((SLPDSocket *)
               SLPListUnlink(&G_IncomingSocketList, (SLPListItem *) sock));
         sock = NULL;
      }
   } 

   return 0;
}

/** Initialize incoming socket list for all network interfaces.
 *
 * @return Zero on success, or a non-zero value on failure.
 */
int SLPDIncomingInit(void)
{
   struct sockaddr_storage myaddr;
   struct sockaddr_storage mcast4addr;
#if defined(ENABLE_SLPv1)
   struct sockaddr_storage v1mcast4addr;
#endif
   struct sockaddr_storage lo4addr;
   struct sockaddr_storage lo6addr;
   struct sockaddr_storage srvloc6addr_node;
   struct sockaddr_storage srvlocda6addr_node;
   struct sockaddr_storage srvloc6addr_link;
   struct sockaddr_storage srvlocda6addr_link;
   struct sockaddr_storage srvloc6addr_site;
   struct sockaddr_storage srvlocda6addr_site;
   SLPDSocket * sock;
   char addr_str[INET6_ADDRSTRLEN];
   SLPIfaceInfo ifaces;
   int i;

#if 1
   /* 
    * do not remove ipv6 multicast service sockets previously
    * created from database init (slp.reg)
    */
#else
   /*------------------------------------------------------------*/
   /* First, remove all of the sockets that might be in the list */
   /*------------------------------------------------------------*/
   while (G_IncomingSocketList.count)
      SLPDSocketFree((SLPDSocket *)
            SLPListUnlink(&G_IncomingSocketList, G_IncomingSocketList.head));
#endif


   /*---------------------------------------------------------*/
   /* set up addresses to use for ipv4 loopback and multicast */
   /*---------------------------------------------------------*/
   if(SLPNetIsIPV4())
   {
      int tmpaddr = INADDR_LOOPBACK;
      SLPNetSetAddr(&lo4addr, AF_INET, G_SlpdProperty.port, &tmpaddr);
      tmpaddr = SLP_MCAST_ADDRESS;
      SLPNetSetAddr(&mcast4addr, AF_INET, G_SlpdProperty.port, &tmpaddr);
#if defined(ENABLE_SLPv1)
      tmpaddr = SLPv1_DA_MCAST_ADDRESS;
      SLPNetSetAddr(&v1mcast4addr, AF_INET, G_SlpdProperty.port, &tmpaddr);
#endif

   }

   /*-------------------------------------------------------*/
   /* set up address to use for ipv6 loopback and multicast */
   /*-------------------------------------------------------*/
   if(SLPNetIsIPV6())
   {
      /*All of these addresses are needed to support RFC 3111*/
      SLPNetSetAddr(&lo6addr, AF_INET6, G_SlpdProperty.port, &slp_in6addr_loopback);
      SLPNetSetAddr(&srvloc6addr_node, AF_INET6, G_SlpdProperty.port, &in6addr_srvloc_node);
      SLPNetSetAddr(&srvloc6addr_link, AF_INET6, G_SlpdProperty.port, &in6addr_srvloc_link);
      SLPNetSetAddr(&srvloc6addr_site, AF_INET6, G_SlpdProperty.port, &in6addr_srvloc_site);
      SLPNetSetAddr(&srvlocda6addr_node, AF_INET6, G_SlpdProperty.port, &in6addr_srvlocda_node);
      SLPNetSetAddr(&srvlocda6addr_link, AF_INET6, G_SlpdProperty.port, &in6addr_srvlocda_link);
      SLPNetSetAddr(&srvlocda6addr_site, AF_INET6, G_SlpdProperty.port, &in6addr_srvlocda_site);
   }

   /*--------------------------------------------------------------------*/
   /* Create SOCKET_LISTEN socket and datagram socket on LOOPBACK for    */
   /* the different library versions to talk to.                         */
   /*--------------------------------------------------------------------*/
   if (SLPNetIsIPV4())
   {
      sock = SLPDSocketCreateListen(&lo4addr);
      if (sock)
      {
         SLPListLinkTail(&G_IncomingSocketList, (SLPListItem *) sock);
         SLPDLog("Listening on loopback TCP...\n");
      }
      else
      {
         SLPDLog("NETWORK_ERROR - Could not listen for TCP on IPv4 loopback\n");
         SLPDLog("INTERNAL_ERROR - No SLPLIB support will be available with TCP\n");
      }
      sock = SLPDSocketCreateBoundDatagram(&lo4addr, &lo4addr, DATAGRAM_UNICAST);
      if (sock)
      {
         SLPListLinkTail(&G_IncomingSocketList, (SLPListItem *) sock);
         SLPDLog("Listening on loopback UDP...\n");
      }
      else
      {
         SLPDLog("NETWORK_ERROR - Could not listen for UDP on IPv4 loopback\n");
         SLPDLog("INTERNAL_ERROR - No SLPLIB support will be available with UDP\n");
      }
   }
   if (SLPNetIsIPV6())
   {
      sock = SLPDSocketCreateListen(&lo6addr);
      if (sock)
      {
         SLPListLinkTail(&G_IncomingSocketList, (SLPListItem *) sock);
         SLPDLog("Listening on IPv6 loopback TCP...\n");
      }
      else
      {
         SLPDLog("NETWORK_ERROR - Could not listen on TCP IPv6 loopback\n");
         SLPDLog("INTERNAL_ERROR - No SLPLIB support will be available with TCP\n");
      }
      sock = SLPDSocketCreateBoundDatagram(&lo6addr, &lo6addr, DATAGRAM_UNICAST);
      if (sock)
      {
         SLPListLinkTail(&G_IncomingSocketList, (SLPListItem *) sock);
         SLPDLog("Listening on IPv6 loopback UDP...\n");
      }
      else
      {
         SLPDLog("NETWORK_ERROR - Could not listen on UDP IPv6 loopback\n");
         SLPDLog("INTERNAL_ERROR - No SLPLIB support will be available with UDP\n");
      }
   }

   /*---------------------------------------------------------------------*/
   /* Create sockets for all of the interfaces in the interfaces property */
   /*---------------------------------------------------------------------*/

   /*---------------------------------------------------------------------*/
   /* Copy G_SlpdProperty.interfaces to a temporary buffer to parse the   */
   /*   string in a safety way                                            */
   /*---------------------------------------------------------------------*/

   ifaces.iface_addr = malloc(slp_max_ifaces*sizeof(struct sockaddr_storage));
   if (ifaces.iface_addr == NULL)
   {
      SLPDLog("can't allocate %d iface_addrs\n", slp_max_ifaces);
      exit(1);
   }
   ifaces.bcast_addr = malloc(slp_max_ifaces*sizeof(struct sockaddr_storage));
   if (ifaces.bcast_addr == NULL)
   {
      SLPDLog("can't allocate %d bcast_addrs\n", slp_max_ifaces);
      exit(1);
   }
   ifaces.bcast_addr = malloc(slp_max_ifaces*sizeof(struct sockaddr_storage));
   if (G_SlpdProperty.interfaces != NULL)
   {
      if (SLPIfaceGetInfo(G_SlpdProperty.interfaces, &ifaces, AF_UNSPEC) < 0) {
         SLPDLog("SLPIfaceGetInfo failed: %s\n", strerror(errno));
         exit(1);
      }
   } else
      ifaces.iface_count = 0;

   for (i = 0; i < ifaces.iface_count; i++)
   {
      memcpy(&myaddr, &ifaces.iface_addr[i], sizeof(struct sockaddr_storage));

      /*------------------------------------------------*/
      /* Create TCP_LISTEN that will handle unicast TCP */
      /*------------------------------------------------*/
      sock = SLPDSocketCreateListen(&myaddr);
      if (sock)
      {
         SLPListLinkTail(&G_IncomingSocketList, (SLPListItem *) sock);
         SLPDLog("Listening on %s ...\n",
               SLPNetSockAddrStorageToString(&myaddr, addr_str,
                     sizeof(addr_str)));
      }

      /*----------------------------------------------------------------*/
      /* Create socket that will handle multicast UDP.                  */
      /*----------------------------------------------------------------*/
      if (SLPNetIsIPV4() && myaddr.ss_family == AF_INET)
      {
         sock = SLPDSocketCreateBoundDatagram(&myaddr, &mcast4addr,
                     DATAGRAM_MULTICAST);
         if (sock)
         {
            SLPListLinkTail(&G_IncomingSocketList, (SLPListItem *) sock);
            SLPDLog("Multicast (IPv4) socket on %s ready\n",
                  SLPNetSockAddrStorageToString(&myaddr, addr_str,
                        sizeof(addr_str)));
         }
         else
            SLPDLog("Couldn't bind to (IPv4) multicast for interface %s (%s)\n",
                  SLPNetSockAddrStorageToString(&myaddr, addr_str,
                        sizeof(addr_str)),
                  strerror(errno));
      }
      else if (SLPNetIsIPV6() && myaddr.ss_family == AF_INET6)
      {
         /* node-local scope multicast */
         sock = SLPDSocketCreateBoundDatagram(&myaddr, &srvloc6addr_node,
                     DATAGRAM_MULTICAST);
         if (sock)
         {
            SLPListLinkTail(&G_IncomingSocketList, (SLPListItem *) sock);
            SLPDLog("Multicast (IPv6 node scope) socket on %s ready\n",
                  SLPNetSockAddrStorageToString(&myaddr, addr_str,
                        sizeof(addr_str)));
         }
         else
            SLPDLog("Couldn't bind to (IPv6 node scope) multicast for interface %s (%s)\n",
                  SLPNetSockAddrStorageToString(&myaddr, addr_str,
                        sizeof(addr_str)),
                  strerror(errno));
         /* node-local scope DA multicast */
         sock = SLPDSocketCreateBoundDatagram(&myaddr, &srvlocda6addr_node,
                     DATAGRAM_MULTICAST);
         if (sock)
         {
            SLPListLinkTail(&G_IncomingSocketList, (SLPListItem *) sock);
            SLPDLog("Multicast DA (IPv6 node scope) socket on %s ready\n",
                  SLPNetSockAddrStorageToString(&myaddr, addr_str,
                        sizeof(addr_str)));
         }
         else
            SLPDLog("Couldn't bind to (IPv6 node scope) DA multicast for interface %s (%s)\n",
                  SLPNetSockAddrStorageToString(&myaddr, addr_str,
                        sizeof(addr_str)),
                  strerror(errno));

         /* link-local scope multicast */
         sock = SLPDSocketCreateBoundDatagram(&myaddr, &srvloc6addr_link,
                     DATAGRAM_MULTICAST);
         if (sock)
         {
            SLPListLinkTail(&G_IncomingSocketList, (SLPListItem *) sock);
            SLPDLog("Multicast (IPv6 link scope) socket on %s ready\n",
                  SLPNetSockAddrStorageToString(&myaddr, addr_str,
                        sizeof(addr_str)));
         }
         else
            SLPDLog("Couldn't bind to (IPv6 link scope) multicast for interface %s (%s)\n",
                  SLPNetSockAddrStorageToString(&myaddr, addr_str,
                        sizeof(addr_str)),
                  strerror(errno));
         /* link-local scope DA multicast */
         sock = SLPDSocketCreateBoundDatagram(&myaddr, &srvlocda6addr_link,
                     DATAGRAM_MULTICAST);
         if (sock)
         {
            SLPListLinkTail(&G_IncomingSocketList, (SLPListItem *) sock);
            SLPDLog("Multicast DA (IPv6 link scope) socket on %s ready\n",
                  SLPNetSockAddrStorageToString(&myaddr, addr_str,
                        sizeof(addr_str)));
         }
         else
            SLPDLog("Couldn't bind to (IPv6 link scope) DA multicast for interface %s (%s)\n",
                  SLPNetSockAddrStorageToString(&myaddr, addr_str,
                        sizeof(addr_str)),
                  strerror(errno));

         if (!IN6_IS_ADDR_LINKLOCAL(&(((struct sockaddr_in6 *) &myaddr)->sin6_addr)))
         {
            /* site-local scope multicast */
            sock = SLPDSocketCreateBoundDatagram(&myaddr, &srvloc6addr_site,
                        DATAGRAM_MULTICAST);
            if (sock)
            {
               SLPListLinkTail(&G_IncomingSocketList, (SLPListItem *) sock);
               SLPDLog("Multicast (IPv6 site scope) socket on %s ready\n",
                     SLPNetSockAddrStorageToString(&myaddr, addr_str,
                           sizeof(addr_str)));
            }
            else
               SLPDLog("Couldn't bind to (IPv6 site scope) multicast for interface %s (%s)\n",
                     SLPNetSockAddrStorageToString(&myaddr, addr_str,
                           sizeof(addr_str)),
                     strerror(errno));
            /* site-local scope DA multicast */
            sock = SLPDSocketCreateBoundDatagram(&myaddr, &srvlocda6addr_site,
                        DATAGRAM_MULTICAST);
            if (sock)
            {
               SLPListLinkTail(&G_IncomingSocketList, (SLPListItem *) sock);
               SLPDLog("Multicast DA (IPv6 site scope) socket on %s ready\n",
                     SLPNetSockAddrStorageToString(&myaddr, addr_str,
                           sizeof(addr_str)));
            }
            else
               SLPDLog("Couldn't bind to (IPv6 site scope) DA multicast for interface %s (%s)\n",
                     SLPNetSockAddrStorageToString(&myaddr, addr_str,
                           sizeof(addr_str)),
                     strerror(errno));
         }
      }

#if defined(ENABLE_SLPv1)
      if (G_SlpdProperty.isDA && SLPNetIsIPV4())
      {
         /*------------------------------------------------------------*/
         /* Create socket that will handle multicast UDP for SLPv1 DA  */
         /* Discovery.                                                 */
         /* Don't need to do this for ipv6, SLPv1 is not supported.    */
         /*------------------------------------------------------------*/
         sock = SLPDSocketCreateBoundDatagram(&myaddr, &v1mcast4addr, DATAGRAM_MULTICAST);
         if (sock)
         {
            SLPListLinkTail(&G_IncomingSocketList, (SLPListItem *) sock);
            SLPDLog("SLPv1 DA Discovery Multicast socket on %s ready\n",
                  SLPNetSockAddrStorageToString(&myaddr, addr_str,
                        sizeof(addr_str)));
         }
         else
            SLPDLog("Couldn't bind to SLPv1 DA Discovery Multicast socket for interface %s (%s)\n",
                  SLPNetSockAddrStorageToString(&myaddr, addr_str,
                        sizeof(addr_str)),
                        strerror(errno));
      }
#endif

      /*--------------------------------------------*/
      /* Create socket that will handle unicast UDP */
      /*--------------------------------------------*/
      sock = SLPDSocketCreateBoundDatagram(&myaddr, &myaddr, DATAGRAM_UNICAST);
      if (sock)
      {
         /*These sockets are also used for the outgoing multicast*/
         SLPDSocketAllowMcastSend(myaddr.ss_family, sock); 
         SLPListLinkTail(&G_IncomingSocketList, (SLPListItem *) sock);
         SLPDLog("Unicast socket on %s ready\n",
               SLPNetSockAddrStorageToString(&myaddr, addr_str,
                     sizeof(addr_str)));
      }
      else
         SLPDLog("Couldn't bind to Unicast socket for interface %s (%s)\n",
               SLPNetSockAddrStorageToString(&myaddr, addr_str,
                     sizeof(addr_str)),
                     strerror(errno));
 
   }     

   return 0;
}

/** Deinitialize incoming socket list for all network interfaces.
 *
 * @return Zero on success, or a non-zero value on failure.
 */
int SLPDIncomingDeinit(void)
{
   SLPDSocket * del = 0;
   SLPDSocket * sock = (SLPDSocket *) G_IncomingSocketList.head;
   while (sock)
   {
      del = sock;
      sock = (SLPDSocket *) sock->listitem.next;
      if (del)
      {
         SLPDSocketFree((SLPDSocket *)
               SLPListUnlink(&G_IncomingSocketList, (SLPListItem *) del));
         del = 0;
      }
   } 

   return 0;
}

#ifdef DEBUG
/** Dump the incoming socket table.
 *
 * @note This routine is only compiled into DEBUG code.
 */
void SLPDIncomingSocketDump(void)
{
   char str1[INET6_ADDRSTRLEN];
   char str2[INET6_ADDRSTRLEN];
   char str3[INET6_ADDRSTRLEN];
   SLPDSocket * sock = (SLPDSocket *) G_IncomingSocketList.head;
   SLPDLog("========================================================================\n");
   SLPDLog("Dumping IncomingSocketList\n");
   SLPDLog("========================================================================\n");
   while (sock)
   {
      SLPDLog("localaddr=%s peeraddr=%s mcastaddr=%s\n",
         SLPNetSockAddrStorageToString(&(sock->localaddr), str1, sizeof(str1)),
         SLPNetSockAddrStorageToString(&(sock->peeraddr), str2, sizeof(str2)),
         SLPNetSockAddrStorageToString(&(sock->mcastaddr), str3, sizeof(str3)));
      sock = (SLPDSocket *) sock->listitem.next;
   }
}
#endif

/*=========================================================================*/
