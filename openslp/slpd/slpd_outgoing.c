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

/** Outgoing request handler.
 *
 * Handles "outgoing" network conversations requests made by other agents 
 * to slpd.(slpd_incoming.c handles reqests made by other agents to slpd)
 *
 * @file       slpd_outgoing.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#include "slpd_outgoing.h"
#include "slpd_property.h"
#include "slpd_process.h"
#include "slpd_log.h"
#include "slpd_knownda.h"

#include "slp_message.h"
#include "slp_net.h"

SLPList G_OutgoingSocketList = { 0, 0, 0 };

/** Read a datagram from an outbound socket.
 *
 * @param[in] socklist - The list of sockets being monitored.
 * @param[in] sock - The socket to be read from.
 */
void OutgoingDatagramRead(SLPList * socklist, SLPDSocket * sock)
{
   int bytesread;
   socklen_t peeraddrlen = sizeof(struct sockaddr_storage);

   (void)socklist;

   bytesread = recvfrom(sock->fd, (char*)sock->recvbuf->start, 
         SLP_MAX_DATAGRAM_SIZE, 0, (struct sockaddr *)&sock->peeraddr, 
         &peeraddrlen);
   if (bytesread > 0)
   {
      sock->recvbuf->end = sock->recvbuf->start + bytesread;

      SLPDProcessMessage(&sock->peeraddr, &sock->localaddr, 
            sock->recvbuf, &sock->sendbuf);

      /* Completely ignore the message */

   }
}

/** Reconnect an outbound socket.
 *
 * @param[in] socklist - The list of sockets being monitored.
 * @param[in] sock - The socket to be reconnected.
 */
void OutgoingStreamReconnect(SLPList * socklist, SLPDSocket * sock)
{
   char addr_str[INET6_ADDRSTRLEN];

   (void)socklist;

   /*-----------------------------------------------------------------*/
   /* If socket is already being reconnected but is reconnect blocked */
   /* just return.  Blocking connect sockets will eventually time out */
   /*-----------------------------------------------------------------*/
   if (sock->state == STREAM_CONNECT_BLOCK)
      return;

#ifdef DEBUG
   /* Log that reconnect warning */
   SLPDLog("WARNING: Reconnect to agent at %s.  "
           "Agent may not be making efficient \n"
           "         use of TCP.\n",
         SLPNetSockAddrStorageToString(&sock->peeraddr,
               addr_str, sizeof(addr_str)));
#endif

   /*----------------------------------------------------------------*/
   /* Make sure we have not reconnected too many times               */
   /* We only allow SLPD_CONFIG_MAX_RECONN reconnection retries      */
   /* before we stop                                                 */
   /*----------------------------------------------------------------*/
   sock->reconns += 1;
   if (sock->reconns > SLPD_CONFIG_MAX_RECONN)
   {
      SLPDLog("WARNING: Reconnect tries to agent at %s "
              "exceeded maximum. It\n         is possible that "
              "the agent is malicious. Check it out!\n",
            SLPNetSockAddrStorageToString(&sock->peeraddr, 
                  addr_str, sizeof(addr_str)));

      /*Since we can't connect, remove it as a DA*/
      SLPDKnownDARemove(&(sock->peeraddr));
      sock->state = SOCKET_CLOSE;
      return;
   }

   /*----------------------------------------------------------------*/
   /* Close the existing socket to clean the stream  and open an new */
   /* socket                                                         */
   /*----------------------------------------------------------------*/
   closesocket(sock->fd);

   if (sock->peeraddr.ss_family == AF_INET)
      sock->fd = socket(PF_INET, SOCK_STREAM, 0);
   else if (sock->peeraddr.ss_family == AF_INET6)
      sock->fd = socket(PF_INET6, SOCK_STREAM, 0);

   if (sock->fd == SLP_INVALID_SOCKET)
   {
      sock->state = SOCKET_CLOSE;
      return;
   }

   /*---------------------------------------------*/
   /* Set the new socket to enable nonblocking IO */
   /*---------------------------------------------*/
#ifdef _WIN32
   {
      u_long fdflags = 1;
      ioctlsocket(sock->fd, FIONBIO, &fdflags);
   }
#else
   {
      int fdflags = fcntl(sock->fd, F_GETFL, 0);
      fcntl(sock->fd, F_SETFL, fdflags | O_NONBLOCK);
   }
#endif

   /*--------------------------*/
   /* Connect a the new socket */
   /*--------------------------*/
   if (connect(sock->fd, (struct sockaddr *)&sock->peeraddr,
         sizeof(struct sockaddr_storage)))
   {
#ifdef _WIN32
      if (WSAEWOULDBLOCK == WSAGetLastError())
#else
      if (errno == EINPROGRESS)
#endif
      {
         /* Connect blocked */
         sock->state = STREAM_CONNECT_BLOCK;
         return;
      }
   }

   /* Connection occured immediately. Set to WRITE_FIRST so whole */
   /* packet will be written                                      */
   sock->state = STREAM_WRITE_FIRST;
}

/** Read data from an outbound stream-oriented connection.
 *
 * @param[in] socklist - The list of sockets being monitored.
 * @param[in] sock - The socket to be read from.
 */
void OutgoingStreamRead(SLPList * socklist, SLPDSocket * sock)
{
   int bytesread;
   char peek[16];
   socklen_t peeraddrlen = sizeof(struct sockaddr_storage);

   if (sock->state == STREAM_READ_FIRST)
   {
      /*---------------------------------------------------*/
      /* take a peek at the packet to get size information */
      /*---------------------------------------------------*/
      bytesread = recvfrom(sock->fd, peek, 16, MSG_PEEK,
                        (struct sockaddr *) &(sock->peeraddr), &peeraddrlen);
      if (bytesread > 0)
      {
         /* allocate the recvbuf big enough for the whole message */
         sock->recvbuf = SLPBufferRealloc(sock->recvbuf, AS_UINT24(peek + 2));
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
#ifdef _WIN32
         if (WSAEWOULDBLOCK != WSAGetLastError())
#else
         if (errno != EWOULDBLOCK)
#endif
         {
            /* Error occured or connection was closed. Try to reconnect */
            /* Socket will be closed if connect times out               */
            OutgoingStreamReconnect(socklist, sock);
         }
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
         /* reset age because of activity */
         sock->age = 0;

         /* move buffer pointers */
         sock->recvbuf->curpos += bytesread;

         /* check to see if everything was read */
         if (sock->recvbuf->curpos == sock->recvbuf->end)
            switch (SLPDProcessMessage(&(sock->peeraddr), &(sock->localaddr),
                        sock->recvbuf, &(sock->sendbuf)))
            {
               case SLP_ERROR_DA_BUSY_NOW:
                  sock->state = STREAM_WRITE_WAIT;
                  break;
               case SLP_ERROR_PARSE_ERROR:
               case SLP_ERROR_VER_NOT_SUPPORTED:
                  sock->state = SOCKET_CLOSE;
                  break;
               default:
                  /* End of outgoing message exchange. Unlink   */
                  /* send buf from to do list and free it       */
                  SLPBufferFree(sock->sendbuf);
                  sock->sendbuf = NULL;
                  sock->state = STREAM_WRITE_FIRST;
                  /* clear the reconnection count since we actually
                   * transmitted a successful message exchange
                   */
                  sock->reconns = 0;
                  break;
            }
      }
      else
      {
#ifdef _WIN32
         if (WSAEWOULDBLOCK != WSAGetLastError())
#else
         if (errno != EWOULDBLOCK)
#endif
         {
            /* Error occured or connection was closed. Try to reconnect */
            /* Socket will be closed if connect times out               */
            OutgoingStreamReconnect(socklist, sock);
         }
      }
   }
}

/** Write data to an outbound, stream-oriented socket.
 *
 * @param[in] socklist - The list of sockets being monitored.
 * @param[in] sock - The socket to be written to.
 */
void OutgoingStreamWrite(SLPList * socklist, SLPDSocket * sock)
{
   int byteswritten;
   int flags = 0;

#if defined(MSG_DONTWAIT)
   flags = MSG_DONTWAIT;
#endif

   if (sock->state == STREAM_WRITE_FIRST)
   {
      /* set sendbuf to the first item in the send list if it is not set */
      if (sock->sendbuf == NULL)
      {
         sock->sendbuf = (SLPBuffer) sock->sendlist.head;
         if (sock->sendbuf == NULL)
         {
            /* there is nothing in the to do list */
            sock->state = STREAM_CONNECT_IDLE;
            return;
         }
         /* Unlink the send buffer we are sending from the send list */
         SLPListUnlink(&(sock->sendlist), (SLPListItem *) (sock->sendbuf));
      }

      /* make sure that the start and curpos pointers are the same */
      sock->sendbuf->curpos = sock->sendbuf->start;
      sock->state = STREAM_WRITE;
   }

   if (sock->sendbuf->end - sock->sendbuf->curpos > 0)
   {
      byteswritten = send(sock->fd, (char *)sock->sendbuf->curpos,
            (int)(sock->sendbuf->end - sock->sendbuf->curpos), flags);
      if (byteswritten > 0)
      {
         /* reset age because of activity */
         sock->age = 0; 

         /* move buffer pointers */
         sock->sendbuf->curpos += byteswritten;

         /* check to see if everything was written */
         if (sock->sendbuf->curpos == sock->sendbuf->end)
         {
            /* Message is completely sent. Set state to read the reply */
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
         {
            /* Error occured or connection was closed. Try to reconnect */
            /* Socket will be closed if connect times out               */
            OutgoingStreamReconnect(socklist, sock);
         }
      }
   }
   else
   {
      /* nothing to write */
#ifdef DEBUG
      SLPDLog("yikes, an empty socket is being written!\n");
#endif
      sock->state = SOCKET_CLOSE;
   }
}

/** Connect for outbound traffic to a specified remote address.
 *
 * @param[in] addr - The address to be connected to.
 *
 * @remarks Get a pointer to a connected socket that is associated with
 * the outgoing socket list. If a connected socket already exists on the
 * outgoing list, a pointer to it is returned, otherwise a new connection
 * is made and added to the outgoing list
 *
 * @return A pointer to socket, or null on error.
 */
SLPDSocket * SLPDOutgoingConnect(struct sockaddr_storage * addr)
{
   SLPDSocket * sock = (SLPDSocket *) G_OutgoingSocketList.head;
   while (sock)
   {
      if (sock->state == STREAM_CONNECT_IDLE
            || sock->state > STREAM_CONNECT_CLOSE)
      {
         if (SLPNetCompareAddrs(&(sock->peeraddr), addr) == 0)
            break;
      }
      sock = (SLPDSocket *) sock->listitem.next;
   }

   if (sock == 0)
   {
      sock = SLPDSocketCreateConnected(addr);
      if (sock)
         SLPListLinkTail(&(G_OutgoingSocketList), (SLPListItem *) sock);
   }

   return sock;
}

/** Add a ready-to-write outgoing datagram socket to the outgoing list.
 *
 * The datagram will be written then sit in the list until it ages out
 * (after net.slp.unicastMaximumWait).
 *
 * @param[in] sock - The socket that will belong on the outgoing list.
 * @param[in] mcast - If non-zero, the message will be sent to sock->mcastaddr instead of sock->peeraddr.
 * @param[in] addtolist - If non-zero, the socket is a throwaway to add to the aging list
 */
void SLPDOutgoingDatagramWrite(SLPDSocket * sock, int mcast, int addtolist)
{
   if (sendto(sock->fd, (char*)sock->sendbuf->start,
            (int)(sock->sendbuf->end - sock->sendbuf->start), 0,
            mcast ? (struct sockaddr *)&sock->mcastaddr : (struct sockaddr *)&sock->peeraddr,
            sizeof(struct sockaddr_storage)) >= 0)
   {
      /* Link the socket into the outgoing list so replies will be */
      /* processed -- if there would be any                        */
      if(addtolist)
         SLPListLinkHead(&G_OutgoingSocketList, (SLPListItem *) (sock));
   }
   else if (!mcast)  /*Since multicast now uses the incoming list, I'll let the incoming handler clean up correctly*/
   {
#ifdef DEBUG
      SLPDLog("ERROR: Data could not send() in SLPDOutgoingDatagramWrite()");
#endif
      SLPDSocketFree(sock);
   }
}

/** Handles outgoing requests pending on specified file discriptors.
 *
 * @param[in,out] fdcount - The number of file descriptors marked in fd_sets.
 * @param[in] readfds - The set of file descriptors with pending read IO.
 * @param[in] writefds - The set of file descriptors with pending write IO.
 */
void SLPDOutgoingHandler(int * fdcount, fd_set * readfds, fd_set * writefds)
{
   SLPDSocket * sock;
   sock = (SLPDSocket *) G_OutgoingSocketList.head;
   while (sock && *fdcount)
   {
      if (FD_ISSET(sock->fd, readfds))
      {
         switch (sock->state)
         {
            case DATAGRAM_MULTICAST:
            case DATAGRAM_BROADCAST:
            case DATAGRAM_UNICAST:
               OutgoingDatagramRead(&G_OutgoingSocketList, sock);
               break;

            case STREAM_READ:
            case STREAM_READ_FIRST:
               OutgoingStreamRead(&G_OutgoingSocketList, sock);
               break;

            default:
               /* No SOCKET_LISTEN sockets should exist */
               break;
         }

         *fdcount = *fdcount - 1;
      }
      else if (FD_ISSET(sock->fd, writefds))
      {
         switch (sock->state)
         {
            case STREAM_CONNECT_BLOCK:
               sock->age = 0;
               sock->state = STREAM_WRITE_FIRST;

            case STREAM_WRITE:
            case STREAM_WRITE_FIRST:
               OutgoingStreamWrite(&G_OutgoingSocketList, sock);
               break;

            default:
               break;
         }

         *fdcount = *fdcount - 1;
      }

      sock = (SLPDSocket *) sock->listitem.next;
   }
}

/** Age the outgoing socket list.
 *
 * @param[in] seconds - The number of seconds old an entry must be to be 
 *    removed from the outgoing list.
 */
void SLPDOutgoingAge(time_t seconds)
{
   SLPDSocket * del = 0;
   SLPDSocket * sock = (SLPDSocket *) G_OutgoingSocketList.head;

   while (sock)
   {
      switch (sock->state)
      {
         case DATAGRAM_MULTICAST:
         case DATAGRAM_BROADCAST:
         case DATAGRAM_UNICAST:
            if (sock->age > G_SlpdProperty.unicastMaximumWait / 1000)
               del = sock;
            sock->age = sock->age + seconds;
            break;

         case STREAM_READ_FIRST:
         case STREAM_WRITE_FIRST:
            sock->age = 0;
            break;

         case STREAM_CONNECT_BLOCK:
         case STREAM_READ:
         case STREAM_WRITE:
            if (G_OutgoingSocketList.count > SLPD_COMFORT_SOCKETS)
            {
               /* Accelerate ageing cause we are low on sockets */
               if (sock->age > SLPD_CONFIG_BUSY_CLOSE_CONN)
               {
                  /* Remove peer from KnownDAs since it might be dead */
                  SLPDKnownDARemove(&(sock->peeraddr));
                  del = sock;
               }
            }
            else
            {
               if (sock->age > SLPD_CONFIG_CLOSE_CONN)
               {
                  /* Remove peer from KnownDAs since it might be dead */
                  SLPDKnownDARemove(&(sock->peeraddr));
                  del = sock;
               }
            }
            sock->age = sock->age + seconds;
            break;

         case STREAM_CONNECT_IDLE:
            if (G_OutgoingSocketList.count > SLPD_COMFORT_SOCKETS)
            {
               /* Accelerate ageing cause we are low on sockets */
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

         case STREAM_WRITE_WAIT:
            /* this when we are talking to a busy DA */
            sock->age = 0;
            sock->state = STREAM_WRITE_FIRST;
            break;

         default:
            /* don't age the other sockets at all */
            break;
      }

      sock = (SLPDSocket *) sock->listitem.next;

      if (del)
      {
         SLPDSocketFree((SLPDSocket *)
               SLPListUnlink(&G_OutgoingSocketList, (SLPListItem *) del));
         del = 0;
      }
   }
}

/** Initialize outgoing socket list for all network interfaces.
 *
 * @return Zero - always.
 */
int SLPDOutgoingInit(void)
{
   /*------------------------------------------------------------*/
   /* First, remove all of the sockets that might be in the list */
   /*------------------------------------------------------------*/
   while (G_OutgoingSocketList.count)
      SLPDSocketFree((SLPDSocket *)
            SLPListUnlink(&G_OutgoingSocketList,
                  (SLPListItem *) G_OutgoingSocketList.head));

   return 0;
}

/** Release all unused socket on inbound socket list.
 *
 * Deinitialize incoming socket list to have appropriate sockets for all
 * network interfaces.
 *
 * @param[in] graceful - Flag indicates do NOT close sockets with pending 
 *    writes outstanding.
 *
 * @return Zero on success, or a non-zero value when pending writes 
 *    remain.
 */
int SLPDOutgoingDeinit(int graceful)
{
   SLPDSocket * del = 0;
   SLPDSocket * sock = (SLPDSocket *) G_OutgoingSocketList.head;

   while (sock)
   {
      /* graceful only closes sockets without pending I/O */
      if (graceful == 0)
         del = sock;
      else if (sock->state < SOCKET_PENDING_IO)
         del = sock;

      sock = (SLPDSocket *) sock->listitem.next;

      if (del)
      {
         SLPDSocketFree((SLPDSocket *)
               SLPListUnlink(&G_OutgoingSocketList, (SLPListItem *) del));
         del = 0;
      }
   }

   return G_OutgoingSocketList.count;
}

#ifdef DEBUG
/** Dump outbound socket data.
 *
 * @note This routine is compiled in Debug code only.
 */
void SLPDOutgoingSocketDump(void) 
{
}
#endif

/*=========================================================================*/
