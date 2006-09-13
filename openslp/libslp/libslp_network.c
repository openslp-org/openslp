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

/** Networking routines.
 *
 * Implementation for functions that are related to INTERNAL library 
 * network (and ipc) communication.
 *
 * @file       libslp_network.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    LibSLPCode
 */

#include "slp.h"
#include "libslp.h"
#include "slp_net.h"
#include "slp_network.h"
#include "slp_iface.h"
#include "slp_message.h"
#include "slp_compare.h"
#include "slp_xmalloc.h"
#include "slp_xcast.h"
#include "slp_socket.h"

#if !defined(MI_NOT_SUPPORTED)

/** Returns all multi-cast addresses to which a message type can be sent.
 *
 * Returns all the multicast addresses the msgtype can be sent out to. If
 * there is more than one address returned, the addresses will be in the
 * order that they should be consumed (according to policy).
 *
 * @param[in] msgtype - The function-id to use in the SLPMessage header
 * @param[in] msg - A pointer to the portion of the SLP message to send. 
 *    The portion to that should be pointed to is everything after the 
 *    pr-list. Only needed for Service Requests. Set to NULL if not needed.
 * @param[out] ifaceinfo - The interface to send the msg to.
 *
 * @return SLP_OK on success. SLP_ERROR on failure.
 *
 * @internal
 */
static int NetworkGetMcastAddrs(const char msgtype, uint8_t * msg, 
      SLPIfaceInfo * ifaceinfo)
{
   if (!ifaceinfo)
      return SLP_PARAMETER_BAD;

   ifaceinfo->bcast_count = ifaceinfo->iface_count = 0;
   switch (msgtype) 
   {
      case SLP_FUNCT_SRVRQST:
         if (!msg)
            return SLP_PARAMETER_BAD;
         if (SLPNetIsIPV6()) 
         {
            uint16_t srvtype_len = GetUINT16(&msg);
            const char * srvtype = GetStrPtr(&msg, srvtype_len);
            /* Add IPv6 multicast groups in order they should appear. */
            SLPNetGetSrvMcastAddr(srvtype, srvtype_len, SLP_SCOPE_NODE_LOCAL, 
                  &ifaceinfo->iface_addr[ifaceinfo->iface_count]);
            SLPNetSetPort(&ifaceinfo->iface_addr[ifaceinfo->iface_count], 
                  SLP_RESERVED_PORT);
            ifaceinfo->iface_count++;
            SLPNetGetSrvMcastAddr(srvtype, srvtype_len, SLP_SCOPE_LINK_LOCAL, 
                  &ifaceinfo->iface_addr[ifaceinfo->iface_count]);
            SLPNetSetPort(&ifaceinfo->iface_addr[ifaceinfo->iface_count], 
                  SLP_RESERVED_PORT);
            ifaceinfo->iface_count++;
            SLPNetGetSrvMcastAddr(srvtype, srvtype_len, SLP_SCOPE_SITE_LOCAL, 
                  &ifaceinfo->iface_addr[ifaceinfo->iface_count]);
            SLPNetSetPort(&ifaceinfo->iface_addr[ifaceinfo->iface_count], 
                  SLP_RESERVED_PORT);
            ifaceinfo->iface_count++;
         }
         if (SLPNetIsIPV4()) 
         {
            struct in_addr mcastaddr;
            mcastaddr.s_addr = SLP_MCAST_ADDRESS;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count], 
                  AF_INET, SLP_RESERVED_PORT, &mcastaddr);
            ifaceinfo->iface_count++;
         }
         break;

      case SLP_FUNCT_ATTRRQST:
         if (SLPNetIsIPV6()) 
         {
            /* Add IPv6 multicast groups in order they should appear. */
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, SLP_RESERVED_PORT, &in6addr_srvloc_node);
            ifaceinfo->iface_count++;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, SLP_RESERVED_PORT, &in6addr_srvloc_link);
            ifaceinfo->iface_count++;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, SLP_RESERVED_PORT, &in6addr_srvloc_site);
            ifaceinfo->iface_count++;
         }
         if (SLPNetIsIPV4()) 
         {
            struct in_addr mcastaddr;
            mcastaddr.s_addr = SLP_MCAST_ADDRESS;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count], 
                  AF_INET, SLP_RESERVED_PORT, &mcastaddr);
            ifaceinfo->iface_count++;
         }
         break;

      case SLP_FUNCT_SRVTYPERQST:
         if (SLPNetIsIPV6()) 
         {
            /* Add IPv6 multicast groups in order they should appear. */
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, SLP_RESERVED_PORT, &in6addr_srvloc_node);
            ifaceinfo->iface_count++;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, SLP_RESERVED_PORT, &in6addr_srvloc_link);
            ifaceinfo->iface_count++;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, SLP_RESERVED_PORT, &in6addr_srvloc_site);
            ifaceinfo->iface_count++;
         }
         if (SLPNetIsIPV4()) 
         {
            struct in_addr mcastaddr;
            mcastaddr.s_addr = SLP_MCAST_ADDRESS;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count], 
                  AF_INET, SLP_RESERVED_PORT, &mcastaddr);
            ifaceinfo->iface_count++;
         }
         break;

      case SLP_FUNCT_DASRVRQST:
         if (SLPNetIsIPV6()) 
         {
            /* Add IPv6 multicast groups in order they should appear. */
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, SLP_RESERVED_PORT, &in6addr_srvlocda_node);
            ifaceinfo->iface_count++;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, SLP_RESERVED_PORT, &in6addr_srvlocda_link);
            ifaceinfo->iface_count++;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, SLP_RESERVED_PORT, &in6addr_srvlocda_site);
            ifaceinfo->iface_count++;
         }
         if (SLPNetIsIPV4()) 
         {
            struct in_addr mcastaddr;
            mcastaddr.s_addr = SLP_MCAST_ADDRESS;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count], 
                  AF_INET, SLP_RESERVED_PORT, &mcastaddr);
            ifaceinfo->iface_count++;
         }
         break;

      default:
         return SLP_PARAMETER_BAD;
   }
   return SLP_OK;
}     

#endif   /* ! MI_NOT_SUPPORTED */

/** Connects to slpd and provides a peeraddr to send to.
 *
 * @param[out] peeraddr - The address of storage for the connected 
 *    DA's address.
 * @param[in,out] peeraddrsz - The size in bytes of @p peeraddr on 
 *    entry; the size of the address stored in @p peeraddr on exit.
 *
 * @return The connected socket, or -1 if no DA connection can be made.
 */
sockfd_t NetworkConnectToSlpd(void * peeraddr)
{
   sockfd_t sock = SLP_INVALID_SOCKET;

   if (SLPNetIsIPV6()) 
      if (!SLPNetSetAddr(peeraddr, AF_INET6, SLP_RESERVED_PORT, 
            &slp_in6addr_loopback))
         sock = SLPNetworkConnectStream(peeraddr, 0);

   if (sock == SLP_INVALID_SOCKET && SLPNetIsIPV4()) 
   {
      int tempAddr = INADDR_LOOPBACK;
      if (SLPNetSetAddr(peeraddr, AF_INET, 
            SLP_RESERVED_PORT, &tempAddr) == 0)
         sock = SLPNetworkConnectStream(peeraddr, 0);
   }
   return sock;
}

/** Disconnect from the connected DA.
 *
 * Called after DA fails to respond.
 *
 * @param[in] handle - The SLP handle containing the socket to close.
 */
void NetworkDisconnectDA(SLPHandleInfo * handle)
{
   if (handle->dasock)
   {
      closesocket(handle->dasock);
      handle->dasock = SLP_INVALID_SOCKET;
   }
   KnownDABadDA(&handle->daaddr);
}

/** Disconnect from the connected SA.
 *
 * Called after SA fails to respond.
 *
 * @param[in] handle - The SLP handle containing the socket to close.
 */
void NetworkDisconnectSA(SLPHandleInfo * handle)
{
   if (handle->sasock)
   {
      closesocket(handle->sasock);
      handle->sasock = SLP_INVALID_SOCKET;
   }
}

/** Connects to slpd and provides a peeraddr to send to
 *
 * @param[in] handle - The SLP handle containing the socket to close.
 * @param[in] scopelist - The scope that must be supported by DA. Pass 
 *    in NULL for any scope
 * @param[in] scopelistlen - The length of @p scopelist. Ignored if
 *    @p scopelist is NULL.
 * @param[out] peeraddr - The address of storage to receive the connected 
 *    DA's address.
 *
 * @return The connected socket, or -1 if no DA connection can be made.
 */ 
sockfd_t NetworkConnectToDA(SLPHandleInfo * handle, const char * scopelist,
      size_t scopelistlen, void * peeraddr)
{
   /* Attempt to use a cached socket if scope is supported otherwise
    * discover a DA that supports the scope.
    */
   if (handle->dasock != SLP_INVALID_SOCKET && handle->dascope != 0 
         && SLPCompareString(handle->dascopelen, handle->dascope, 
               scopelistlen, scopelist) == 0)
      memcpy(peeraddr, &handle->daaddr, sizeof(struct sockaddr_storage));
   else
   {
      /* Close existing handle because it doesn't support the scope. */
      if (handle->dasock != SLP_INVALID_SOCKET)
         closesocket(handle->dasock);

      /* Attempt to connect to DA that does support the scope. */
      handle->dasock = KnownDAConnect(handle, scopelistlen, scopelist, 
            &handle->daaddr);
      if (handle->dasock != SLP_INVALID_SOCKET)
      {
         xfree(handle->dascope);
         handle->dascope = xmemdup(scopelist, scopelistlen);
         handle->dascopelen = scopelistlen; 
         memcpy(peeraddr, &handle->daaddr, sizeof(struct sockaddr_storage));
      }
   }
   return handle->dasock;
}

/** Connects to slpd and provides a network address to send to.
 *
 * This routine attempts to use a cached socket if the cached socket supports
 * one of the scopes specified in @p scopelist, otherwise it attempts to 
 * connect to the local Service Agent, or a DA that supports the scope, in 
 * order to register directly, if no local SA is present.
 *
 * @param[in] handle - SLPHandle info (caches connection info).
 * @param[in] scopelist - Scope that must be supported by SA. Pass in 
 *    NULL for any scope.
 * @param[in] scopelistlen - The length of @p scopelist in bytes.
 *    Ignored if @p scopelist is NULL.
 * @param[out] saaddr - The address of storage to receive the connected 
 *    SA's address.
 *
 * @return The connected socket, or -1 if no SA connection can be made.
 *
 * @note The memory pointed to by @p saaddr must at least as large as a 
 * sockaddr_storage buffer.
 */ 
sockfd_t NetworkConnectToSA(SLPHandleInfo * handle, const char * scopelist,
      size_t scopelistlen, void * saaddr)
{
   if (handle->sasock != SLP_INVALID_SOCKET && handle->sascope != 0 
         && SLPCompareString(handle->sascopelen, handle->sascope,
               scopelistlen, scopelist) == 0)
      memcpy(saaddr, &handle->saaddr, sizeof(handle->saaddr));
   else
   {
      /* Close last cached SA socket - scopes not supported. */
      if (handle->sasock != SLP_INVALID_SOCKET)
         closesocket(handle->sasock);

      /* Attempt to connect to slpd via loopback. */
      handle->sasock = NetworkConnectToSlpd(&handle->saaddr);

      /* If we connected to something, cache scope and addr info. */
      if (handle->sasock != SLP_INVALID_SOCKET)
      {
         xfree(handle->sascope);
         handle->sascope = xmemdup(scopelist, scopelistlen);
         handle->sascopelen = scopelistlen; 
         memcpy(saaddr, &handle->saaddr, sizeof(handle->saaddr));
      }
   }
   return handle->sasock;
}

/** Make a request and wait for a reply, or timeout.
 *
 * @param[in] sock - The socket to send/receive on.
 * @param[in] peeraddr - The address to send to.
 * @param[in] langtag - The language to send in.
 * @param[in] extoffset - The offset to the first extension in @p buf.
 * @param[in] buf - The message to send.
 * @param[in] buftype - The type of @p buf.
 * @param[in] bufsize - The size of @p buf.
 * @param[in] callback - The user callback to call with response data.
 * @param[in] cookie - A pass through value from the caller to @p callback.
 *
 * @return SLP_OK on success, or an SLP error code on failure.
 */ 
SLPError NetworkRqstRply(sockfd_t sock, void * peeraddr, 
      const char * langtag, size_t extoffset, void * buf, char buftype,
      size_t bufsize, NetworkRplyCallback callback, void * cookie)
{
   char * prlist = 0;
   unsigned short flags;

   size_t mtu = 0;
   size_t prlistlen = 0;
   SLPBuffer sendbuf = 0;
   SLPBuffer recvbuf = 0;
   SLPError result = SLP_OK;

   int looprecv;
   int xmitcount;
   int maxwait;
   int socktype;
   int rplycount = 0;
   int totaltimeout = 0;
   int xid = SLPXidGenerate();
   int timeouts[MAX_RETRANSMITS];

   struct sockaddr_storage addr;
   size_t langtaglen = strlen(langtag);

   /* Determine unicast/multicast, TCP/UDP and timeout values. */
   if (SLPNetIsMCast(peeraddr))
   {
      /* Multicast or broadcast target address. */
      maxwait = SLPPropertyAsInteger("net.slp.multicastMaximumWait");
      SLPPropertyAsIntegerVector("net.slp.multicastTimeouts", 
            timeouts, MAX_RETRANSMITS);
      xmitcount = 0;          /* Only retry to specific listeners. */
      looprecv = 1;
      socktype = SOCK_DGRAM;  /* Broad/multicast are datagrams. */
   }
   else
   {
      /* Unicast/stream target address. */
      socklen_t stypesz = sizeof(socktype);
      maxwait = SLPPropertyAsInteger("net.slp.unicastMaximumWait");
      SLPPropertyAsIntegerVector("net.slp.unicastTimeouts",
            timeouts, MAX_RETRANSMITS);
      getsockopt(sock, SOL_SOCKET, SO_TYPE, (char *)&socktype, &stypesz);
      if (socktype == SOCK_DGRAM)
      {
         xmitcount = 0;                /* Datagrams must be retried. */
         looprecv  = 1;
      }
      else
      {
         xmitcount = MAX_RETRANSMITS;  /* Streams manage own retries. */
         looprecv  = 0;
      }
   }

   /* Special case for fake SLP_FUNCT_DASRVRQST. */
   if (buftype == SLP_FUNCT_DASRVRQST)
   {
      /* do something special for SRVRQST that will be discovering DAs */
      maxwait = SLPPropertyAsInteger("net.slp.DADiscoveryMaximumWait");
      SLPPropertyAsIntegerVector("net.slp.DADiscoveryTimeouts", 
            timeouts, MAX_RETRANSMITS);
      /* DASRVRQST is a fake function - change to SRVRQST. */
      buftype  = SLP_FUNCT_SRVRQST;
      looprecv = 1;  /* We're streaming replies in from the DA. */
   }

   /* Allocate memory for the prlist on datagram sockets for appropriate 
    * messages. Note that the prlist is as large as the MTU -- thus assuring
    * that there will not be any buffer overwrites regardless of how many 
    * previous responders there are. This is because the retransmit code 
    * terminates if ever MTU is exceeded for any datagram message. 
    */
   mtu = SLPPropertyAsInteger("net.slp.MTU");
   if (buftype == SLP_FUNCT_SRVRQST 
         || buftype == SLP_FUNCT_ATTRRQST 
         || buftype == SLP_FUNCT_SRVTYPERQST)
   {
      prlist = (char *)xmalloc(mtu);
      if (!prlist)
      {
         result = SLP_MEMORY_ALLOC_FAILED;
         goto CLEANUP;
      }
      *prlist = 0;
      prlistlen = 0; 
   }

   /* ----- Main Retransmission Loop ----- */
   while (xmitcount <= MAX_RETRANSMITS)
   {
      size_t size;
      struct timeval timeout;
      
      /* Setup receive timeout. */
      if (socktype == SOCK_DGRAM)
      {
         totaltimeout += timeouts[xmitcount];
         if (totaltimeout >= maxwait || !timeouts[xmitcount])
            break; /* Max timeout exceeded - we're done. */

         timeout.tv_sec = timeouts[xmitcount] / 1000;
         timeout.tv_usec = (timeouts[xmitcount] % 1000) * 1000;
      }
      else
      {
         timeout.tv_sec = maxwait / 1000;
         timeout.tv_usec = (maxwait % 1000) * 1000;
      }
      xmitcount++;

   /*  0                   1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |    Version    |  Function-ID  |            Length             |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      | Length, contd.|O|F|R|       reserved          |Next Ext Offset|
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |  Next Extension Offset, contd.|              XID              |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |      Language Tag Length      |         Language Tag          \
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

      /* Calculate the (new) size of the send buffer. */
      size = (int)(
            + 1               /* Version              */
            + 1               /* Function-ID          */
            + 3               /* Length               */
            + 2               /* Flags                */
            + 3               /* Extension Offset     */
            + 2               /* XID                  */
            + 2 + langtaglen  /* Lang Tag Len/Value   */
            + bufsize);       /* Message              */
   
      if (buftype == SLP_FUNCT_SRVRQST 
            || buftype == SLP_FUNCT_ATTRRQST 
            || buftype == SLP_FUNCT_SRVTYPERQST)
      {
      /*  0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |      length of <PRList>       |        <PRList> String        \
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

         size += (int)(
               + 2 + prlistlen); /* <PRList> Len/String  */
      }

      /* Ensure the send buffer size does not exceed MTU for datagrams. */
      if (socktype == SOCK_DGRAM && size > mtu)
      {
         if (!xmitcount)
            result = SLP_BUFFER_OVERFLOW;
         goto FINISHED;
      }

      /* (Re)Allocate the send buffer based on the (new) size. */
      if ((sendbuf = SLPBufferRealloc(sendbuf, size)) == 0)
      {
         result = SLP_MEMORY_ALLOC_FAILED;
         goto CLEANUP;
      }

      /* -- Begin SLP Header -- */

      /* Version */
      *sendbuf->curpos++ = 2;

      /* Function-ID */
      *sendbuf->curpos++ = buftype;

      /* Length */
      PutUINT24(&sendbuf->curpos, size);

      /* Flags */
      flags = (SLPNetIsMCast(peeraddr)? SLP_FLAG_MCAST : 0);
      if (buftype == SLP_FUNCT_SRVREG)
         flags |= SLP_FLAG_FRESH;
      PutUINT16(&sendbuf->curpos, flags);

      /* Extension Offset - TRICKY: The extoffset was passed into us 
       * relative to the start of the user's message, not the SLP header. 
       * We need to fix up the offset to be relative to the beginning of 
       * the SLP message.
       */
      if (extoffset != 0)
         PutUINT24(&sendbuf->curpos, extoffset + langtaglen + 14);
      else
         PutUINT24(&sendbuf->curpos, 0);

      /* XID */
      PutUINT16(&sendbuf->curpos, xid);

      /* Language Tag Length */
      PutUINT16(&sendbuf->curpos, langtaglen);

      /* Language Tag */
      memcpy(sendbuf->curpos, langtag, langtaglen);
      sendbuf->curpos += langtaglen;

      /* -- End SLP Header -- */

      /* -- Begin SLP Message -- */

      /* Add the prlist for appropriate message types. */
      if (prlist)
      {
         PutUINT16(&sendbuf->curpos, prlistlen);
         memcpy(sendbuf->curpos, prlist, prlistlen);
         sendbuf->curpos += prlistlen;
      }

      /* @todo Add scatter-gather so we don't have to copy buffers. */

      /* Add the rest of the message. */
      memcpy(sendbuf->curpos, buf, bufsize);
      sendbuf->curpos += bufsize;

      /* -- End SLP Message -- */

      /* Send the buffer. */
      if (SLPNetworkSendMessage(sock, socktype, sendbuf, 
            sendbuf->curpos - sendbuf->start, peeraddr, 
            &timeout) != 0)
      {
         if (errno == ETIMEDOUT)
            result = SLP_NETWORK_TIMED_OUT;
         else
            result = SLP_NETWORK_ERROR;    
         goto FINISHED;
      }

      /* ----- Main Receive Loop ----- */
      do
      {
         /* Set peer family to "not set" so we can detect if it was set. */
         addr.ss_family = AF_UNSPEC;

         /* Receive the response. */
         if (SLPNetworkRecvMessage(sock, socktype, &recvbuf, 
               &addr, &timeout) != 0)
         {
            if (errno == ETIMEDOUT)
               result = SLP_NETWORK_TIMED_OUT;
            else
               result = SLP_NETWORK_ERROR;
            break;
         }
         else
         {
            /* Sneek in and validate the XID. */
            if (AS_UINT16(recvbuf->start + 10) == xid)
            {
               rplycount += 1;

               /* Check the family type of the addr. If it is not set, 
                * assume the addr is the peeraddr. addr is only set on
                * datagram sockets in the SLPNetworkRecvMessage call. 
                */
               if (addr.ss_family == AF_UNSPEC)
                  memcpy(&addr, peeraddr, sizeof(addr));

               /* Call the callback with the result and recvbuf. */
               if (callback(result, &addr, recvbuf, cookie) == SLP_FALSE)
                  goto CLEANUP; /* Caller doesn't want any more info. */

               /* Add the peer to the prlist on appropriate message types. */
               if (prlist)
               {
                  /* Convert peeraddr to string and length. */
                  char addrstr[INET6_ADDRSTRLEN] = "";
                  saddr_ntop(&addr, addrstr, sizeof(addrstr));
                  if (*addrstr != 0)
                  {
                     size_t addrstrlen = strlen(addrstr);

                     /* Append to the prlist if we won't overflow. */
                     if (prlistlen + addrstrlen + 1 < mtu)
                     {
                        /* Append comma if necessary. */
                        if (prlistlen)
                           prlist[prlistlen++] = ',';

                        /* Append address string. */
                        strcpy(prlist + prlistlen, addrstr);
                        prlistlen += addrstrlen;
                     }
                  }
               }
            }
         }
      } while (looprecv);
   }

FINISHED:

   /* Notify the callback that we're done. */
   if (rplycount != 0 || (result == SLP_NETWORK_TIMED_OUT 
         && SLPNetIsMCast(peeraddr)))
      result = SLP_LAST_CALL; 

   callback(result, &addr, recvbuf, cookie);

   if (result == SLP_LAST_CALL)
      result = 0;

CLEANUP:

   /* Free resources. */
   xfree(prlist);
   SLPBufferFree(sendbuf);
   SLPBufferFree(recvbuf);

   return result;
}

/** Make a request and wait for a reply, or timeout.
 *
 * @param[in] handle - The SLP handle associated with this request.

 * param[in] langtag - Language tag to use in SLP message header

 * @param[in] buf - The pointer to the portion of the SLP message to
 *    send. The portion that should be pointed to is everything after
 *    the pr-list. NetworkXcastRqstRply() automatically generates the 
 *    header and the prlist.
 * @param[in] buftype - The function-id to use in the SLPMessage header.
 * @param[in] bufsize - The size of the buffer pointed to by buf.
 * @param[in] callback - The callback to use for reporting results.
 * @param[in] cookie - The cookie to pass to the callback.
 *
 * @return SLP_OK on success. SLP_ERROR on failure.
 */ 
SLPError NetworkMcastRqstRply(SLPHandleInfo * handle, void * buf, 
      char buftype, size_t bufsize, NetworkRplyCallback callback,
      void * cookie)
{
   struct timeval timeout;
   struct sockaddr_storage addr;
   SLPBuffer sendbuf = 0;
   SLPBuffer recvbuf = 0;
   SLPError result = 0;
   size_t langtaglen = 0;
   size_t prlistlen = 0;
   int size = 0;
   char * prlist = 0;
   int xid = 0;
   int mtu = 0;
   int xmitcount = 0;
   int rplycount = 0;
   int maxwait = 0;
   int totaltimeout = 0;
   int usebroadcast = 0;
   int timeouts[MAX_RETRANSMITS];
   SLPIfaceInfo dstifaceinfo;
   SLPIfaceInfo v4outifaceinfo;
   SLPIfaceInfo v6outifaceinfo;
   SLPXcastSockets xcastsocks;
   int currIntf = 0;
   int requestSent;

#if defined(DEBUG)
   /* This function only supports multicast or broadcast of these messages */
   if (buftype != SLP_FUNCT_SRVRQST && buftype != SLP_FUNCT_ATTRRQST 
         && buftype != SLP_FUNCT_SRVTYPERQST && buftype != SLP_FUNCT_DASRVRQST)
      return SLP_PARAMETER_BAD;
#endif

   /* save off a few things we don't want to recalculate */
   langtaglen = strlen(handle->langtag);

   xid = SLPXidGenerate();
   mtu = SLPPropertyAsInteger("net.slp.MTU");
   sendbuf = SLPBufferAlloc(mtu);
   if (!sendbuf)
   {
      result = SLP_MEMORY_ALLOC_FAILED;
      goto FINISHED;
   }

   v4outifaceinfo.iface_count = 0;
   v6outifaceinfo.iface_count = 0;
   xcastsocks.sock_count = 0;

#if !defined(MI_NOT_SUPPORTED)
   /* Determine which multicast addresses to send to. */
   NetworkGetMcastAddrs(buftype, buf, &dstifaceinfo);
   /* Determine which interfaces to send out on. */
   if(handle->McastIFList) 
   {
#if defined(DEBUG)
      fprintf(stderr, "McastIFList = %s\n", handle->McastIFList);
#endif
      SLPIfaceGetInfo(handle->McastIFList, &v4outifaceinfo, AF_INET);
      SLPIfaceGetInfo(handle->McastIFList, &v6outifaceinfo, AF_INET6);
   }
   else 
#endif /* MI_NOT_SUPPORTED */

   {
      char * iflist = SLPPropertyXDup("net.slp.interfaces");
      if (SLPNetIsIPV4())
         SLPIfaceGetInfo(iflist, &v4outifaceinfo, AF_INET);
      if (SLPNetIsIPV6())
         SLPIfaceGetInfo(iflist, &v6outifaceinfo, AF_INET6);
      xfree(iflist);
      if (!v4outifaceinfo.iface_count && !v6outifaceinfo.iface_count) 
      {
         result = SLP_NETWORK_ERROR;
         goto FINISHED;
      }
   }

   usebroadcast = SLPPropertyAsBoolean("net.slp.useBroadcast");

   /* multicast/broadcast wait timeouts */
   maxwait = SLPPropertyAsInteger("net.slp.multicastMaximumWait");
   SLPPropertyAsIntegerVector("net.slp.multicastTimeouts",
         timeouts, MAX_RETRANSMITS);

   /* special case for fake SLP_FUNCT_DASRVRQST */
   if (buftype == SLP_FUNCT_DASRVRQST)
   {
      /* do something special for SRVRQST that will be discovering DAs */
      maxwait = SLPPropertyAsInteger("net.slp.DADiscoveryMaximumWait");
      SLPPropertyAsIntegerVector("net.slp.DADiscoveryTimeouts",
            timeouts, MAX_RETRANSMITS);
      /* SLP_FUNCT_DASRVRQST is a fake function.  We really want a SRVRQST */
      buftype  = SLP_FUNCT_SRVRQST;
   }

   /* Allocate memory for the prlist for appropriate messages.
    * Notice that the prlist is as large as the MTU -- thus assuring that
    * there will not be any buffer overwrites regardless of how many
    * previous responders there are.   This is because the retransmit
    * code terminates if ever MTU is exceeded for any datagram message. 
    */
   prlist = (char *)xmalloc(mtu);
   if (!prlist)
   {
      result = SLP_MEMORY_ALLOC_FAILED;
      goto FINISHED;
   }
   *prlist = 0;
   prlistlen = 0; 

#if !defined(MI_NOT_SUPPORTED)
   /* iterate through each multicast scope until we found a provider. */
   while (currIntf < dstifaceinfo.iface_count) 
#endif
   {
      /* main retransmission loop */
      xmitcount = 0;
      totaltimeout = 0;
      requestSent = 0;
      while (xmitcount <= MAX_RETRANSMITS)
      {
         totaltimeout += timeouts[xmitcount];
         if (totaltimeout > maxwait || !timeouts[xmitcount])
            break; /* we are all done */

         timeout.tv_sec = timeouts[xmitcount] / 1000;
         timeout.tv_usec = (timeouts[xmitcount] % 1000) * 1000;

         xmitcount++;

         /* re-allocate buffer and make sure that the send buffer does not
          * exceed MTU for datagram transmission 
          */
         size = (int)(14 + langtaglen + bufsize);
         if (buftype == SLP_FUNCT_SRVRQST || buftype == SLP_FUNCT_ATTRRQST 
               || buftype == SLP_FUNCT_SRVTYPERQST)
            size += (int)(2 + prlistlen); /* add in room for the prlist */

         if (size > mtu)
         {
            if (!xmitcount)
               result = SLP_BUFFER_OVERFLOW;
            goto FINISHED;
         }
         if ((sendbuf = SLPBufferRealloc(sendbuf, size)) == 0)
         {
            result = SLP_MEMORY_ALLOC_FAILED;
            goto FINISHED;
         }

         /* Add the header to the send buffer */

         /* version */
         *sendbuf->curpos++ = 2;

         /* function id */
         *sendbuf->curpos++ = buftype;

         /* length */
         PutUINT24(&sendbuf->curpos, size);

         /* flags */
         PutUINT16(&sendbuf->curpos, SLP_FLAG_MCAST);

         /* ext offset */
         PutUINT24(&sendbuf->curpos, 0);

         /* xid */
         PutUINT16(&sendbuf->curpos, xid);

         /* lang tag len */
         PutUINT16(&sendbuf->curpos, langtaglen);

         /* lang tag */
         memcpy(sendbuf->curpos, handle->langtag, langtaglen);
         sendbuf->curpos += langtaglen;

         /* Add the prlist to the send buffer */
         if (prlist)
         {
            PutUINT16(&sendbuf->curpos, prlistlen);
            memcpy(sendbuf->curpos, prlist, prlistlen);
            sendbuf->curpos += prlistlen;
         }

         /* add the rest of the message */
         memcpy(sendbuf->curpos, buf, bufsize);
         sendbuf->curpos += bufsize;

         /* send the send buffer */
         if (usebroadcast)
            result = SLPBroadcastSend(&v4outifaceinfo,sendbuf,&xcastsocks);
         else
         {
            if (dstifaceinfo.iface_addr[currIntf].ss_family == AF_INET)
               result = SLPMulticastSend(&v4outifaceinfo, sendbuf, &xcastsocks, 
                     &dstifaceinfo.iface_addr[currIntf]);
            else if (dstifaceinfo.iface_addr[currIntf].ss_family == AF_INET6)
               result = SLPMulticastSend(&v6outifaceinfo, sendbuf, &xcastsocks, 
                     &dstifaceinfo.iface_addr[currIntf]);
         }

         if (!result)
            requestSent = 1;

         /* main recv loop */
         while(1)
         {
#if !defined(UNICAST_NOT_SUPPORTED)
            int retval = 0;
            if ((retval = SLPXcastRecvMessage(&xcastsocks, &recvbuf, 
                  &addr, &timeout)) != 0)
#else
            if (SLPXcastRecvMessage(&xcastsocks, &recvbuf, 
                  &addr, &timeout) != 0)
#endif
            {
               /* An error occured while receiving the message
                * probably a just time out error. break for re-send. 
                */
               if (errno == ETIMEDOUT)
                  result = SLP_NETWORK_TIMED_OUT;
               else
                  result = SLP_NETWORK_ERROR;

#if !defined(UNICAST_NOT_SUPPORTED)
               /* retval == SLP_ERROR_RETRY_UNICAST signifies that we 
                * received a multicast packet of size > MTU and hence we 
                * are now sending a unicast request to this IP-address.
                */
               if (retval == SLP_ERROR_RETRY_UNICAST) 
               {
                  sockfd_t tcpsockfd;
                  int retval1, retval2, unicastwait = 0;
                  unicastwait = SLPPropertyAsInteger("net.slp.unicastMaximumWait");
                  timeout.tv_sec = unicastwait / 1000;
                  timeout.tv_usec = (unicastwait % 1000) * 1000;

                  tcpsockfd = SLPNetworkConnectStream(&addr, &timeout);
                  if (tcpsockfd != SLP_INVALID_SOCKET) 
                  {
                     TO_UINT16(sendbuf->start + 5, SLP_FLAG_UCAST);
                     xid = SLPXidGenerate();
                     TO_UINT16(sendbuf->start + 10, xid);

                     retval1 = SLPNetworkSendMessage(tcpsockfd, SOCK_STREAM, 
                           sendbuf, sendbuf->curpos - sendbuf->start, &addr, 
                           &timeout);
                     if (retval1) 
                     {
                        if (errno == ETIMEDOUT) 
                           result = SLP_NETWORK_TIMED_OUT;
                        else 
                           result = SLP_NETWORK_ERROR;
                        closesocket(tcpsockfd);
                        break;
                     }

                     retval2 = SLPNetworkRecvMessage(tcpsockfd, SOCK_STREAM, 
                           &recvbuf, &addr, &timeout);
                     if (retval2) 
                     {
                        /* An error occured while receiving the message
                         *  probably a just time out error. break for re-send. 
                         */
                        if(errno == ETIMEDOUT) 
                           result = SLP_NETWORK_TIMED_OUT;
                        else 
                           result = SLP_NETWORK_ERROR;
                        closesocket(tcpsockfd);
                        break;
                     }
                     closesocket(tcpsockfd);
                     result = SLP_OK;
                     goto SNEEK;                               
                  } 
                  else 
                     break; /* Unsuccessful in opening a TCP conn - retry */
               }
               else
               {
#endif
                  break;
#if !defined(UNICAST_NOT_SUPPORTED)
               }
#endif
            }
#if !defined(UNICAST_NOT_SUPPORTED)
SNEEK:
#endif
            /* Sneek in and check the XID */
            if (AS_UINT16(recvbuf->start + 10) == xid)
            {
               char addrstr[INET6_ADDRSTRLEN] = "";

               saddr_ntop(&addr, addrstr, sizeof(addrstr));

               rplycount += 1;

               /* Call the callback with the result and recvbuf */
#if !defined(MI_NOT_SUPPORTED)
               if (!cookie)
                  cookie = (SLPHandleInfo *)handle;
#endif
               if (callback(result, &addr, recvbuf, cookie) == SLP_FALSE)
                  goto CLEANUP; /* Caller does not want any more info */

               /* add the peer to the previous responder list */
               if (prlistlen != 0)
                  strcat(prlist, ",");
               if (*addrstr != 0) 
               {
                  strcat(prlist, addrstr);
                  prlistlen = strlen(prlist);
               }
            }
         }
         SLPXcastSocketsClose(&xcastsocks);
      }
      currIntf++;
   }

FINISHED:

   /* notify the callback with SLP_LAST_CALL so that they know we're done */
   if (rplycount || result == SLP_NETWORK_TIMED_OUT)
      result = SLP_LAST_CALL;

#if !defined(MI_NOT_SUPPORTED)
   if (!cookie)
      cookie = (SLPHandleInfo *)handle;
#endif

   callback(result, 0, 0, cookie);

   if (result == SLP_LAST_CALL)
      result = SLP_OK;

CLEANUP:

   /* free resources */
   xfree(prlist);
   SLPBufferFree(sendbuf);
   SLPBufferFree(recvbuf);
   SLPXcastSocketsClose(&xcastsocks);

   return result;
}

#if !defined(UNICAST_NOT_SUPPORTED)
/** Unicasts SLP messages.
 *
 * @param[in] handle - A pointer to the SLP handle.
 * @param[in] buf - A pointer to the portion of the SLP message to send.
 * @param[in] buftype - The function-id to use in the SLPMessage header.
 * @param[in] bufsize - the size of the buffer pointed to by @p buf.
 * @param[in] callback - The callback to use for reporting results.
 * @param[in] cookie - The cookie to pass to the callback.
 *
 * @return SLP_OK on success. SLP_ERROR on failure.
 */
SLPError NetworkUcastRqstRply(SLPHandleInfo * handle, void * buf, 
      char buftype, size_t bufsize, NetworkRplyCallback callback, 
      void * cookie)
{
   struct timeval timeout;
   struct sockaddr_storage addr;
   SLPBuffer sendbuf = 0;
   SLPBuffer recvbuf = 0;
   SLPError result = 0;
   size_t langtaglen = 0;
   size_t prlistlen = 0;
   char * prlist = 0;
   int xid = 0;
   int mtu = 0;
   int size = 0;
   int rplycount = 0;
   int maxwait = 0;
   int timeouts[MAX_RETRANSMITS];
   int retval1, retval2;

#if defined(DEBUG)
   /* This function only supports unicast of the following messages */
   if (buftype != SLP_FUNCT_SRVRQST && buftype != SLP_FUNCT_ATTRRQST 
         && buftype != SLP_FUNCT_SRVTYPERQST && buftype != SLP_FUNCT_DASRVRQST)
      return SLP_PARAMETER_BAD;
#endif

   /* save off a few things we don't want to recalculate */
   langtaglen = strlen(handle->langtag);
   xid = SLPXidGenerate();
   mtu = SLPPropertyAsInteger("net.slp.MTU");
   sendbuf = SLPBufferAlloc(mtu);
   if (!sendbuf)
   {
      result = SLP_MEMORY_ALLOC_FAILED;
      goto FINISHED;
   }

   /* unicast wait timeouts */
   maxwait = SLPPropertyAsInteger("net.slp.unicastMaximumWait");
   SLPPropertyAsIntegerVector("net.slp.unicastTimeouts",
         timeouts, MAX_RETRANSMITS);

   /* special case for fake SLP_FUNCT_DASRVRQST */
   if (buftype == SLP_FUNCT_DASRVRQST)
   {
      /* do something special for SRVRQST that will be discovering DAs */
      maxwait = SLPPropertyAsInteger("net.slp.DADiscoveryMaximumWait");
      SLPPropertyAsIntegerVector("net.slp.DADiscoveryTimeouts", 
            timeouts, MAX_RETRANSMITS);
      /* SLP_FUNCT_DASRVRQST is a fake function. We really want to
         send a SRVRQST */
      buftype  = SLP_FUNCT_SRVRQST;
   }

   /* Allocate memory for the prlist for appropriate messages.
    *  Notice that the prlist is as large as the MTU -- thus assuring that
    *  there will not be any buffer overwrites regardless of how many
    *  previous responders there are.   This is because the retransmit
    *  code terminates if ever MTU is exceeded for any datagram message. 
    */
   prlist = (char *)xmalloc(mtu);
   if (!prlist)
   {
      result = SLP_MEMORY_ALLOC_FAILED;
      goto FINISHED;
   }

   *prlist = 0;
   prlistlen = 0;

   /* main unicast segment */
   timeout.tv_sec = timeouts[0] / 1000;
   timeout.tv_usec = (timeouts[0] % 1000) * 1000;

   size = (int)(14 + langtaglen + bufsize);
   if (buftype == SLP_FUNCT_SRVRQST || buftype == SLP_FUNCT_ATTRRQST 
         || buftype == SLP_FUNCT_SRVTYPERQST)
      size += (int)(2 + prlistlen);     /* add in room for the prlist */

   if ((sendbuf = SLPBufferRealloc(sendbuf,size)) == 0)
   {
      result = SLP_MEMORY_ALLOC_FAILED;
      goto FINISHED;
   }

   /* add the header to the send buffer */
   /* version */
   *sendbuf->curpos++ = 2;

   /* function id */
   *sendbuf->curpos++ = buftype;

   /* length */
   PutUINT24(&sendbuf->curpos, size);

   /* flags */
   PutUINT16(&sendbuf->curpos, SLP_FLAG_UCAST);  /* this is a unicast */

   /* ext offset */
   PutUINT24(&sendbuf->curpos, 0);

   /* xid */
   PutUINT16(&sendbuf->curpos, xid);

   /* lang tag len */
   PutUINT16(&sendbuf->curpos, langtaglen);

   /* lang tag */
   memcpy(sendbuf->curpos, handle->langtag, langtaglen);
   sendbuf->curpos += langtaglen;

   /* Add the prlist to the send buffer */
   if (prlist)
   {
      PutUINT16(&sendbuf->curpos, prlistlen);
      memcpy(sendbuf->curpos, prlist, prlistlen);
      sendbuf->curpos += prlistlen;
   }

   /* add the rest of the message */
   memcpy(sendbuf->curpos, buf, bufsize);
   sendbuf->curpos += bufsize;

   /* send the send buffer */
   handle->unicastsock  = SLPNetworkConnectStream(
         &handle->ucaddr, &timeout);
   if (handle->unicastsock != SLP_INVALID_SOCKET)
   {
      retval1 = SLPNetworkSendMessage(handle->unicastsock, SOCK_STREAM, 
            sendbuf, sendbuf->curpos - sendbuf->start, &handle->ucaddr, 
            &timeout);
      if (retval1)
      {
         if (errno ==   ETIMEDOUT)
            result = SLP_NETWORK_TIMED_OUT;
         else
            result = SLP_NETWORK_ERROR;
         closesocket(handle->unicastsock);
         goto FINISHED;
      }

      retval2 = SLPNetworkRecvMessage(handle->unicastsock, SOCK_STREAM, 
            &recvbuf, &handle->ucaddr, &timeout);
      if (retval2)
      {
         /* An error occured while receiving the message
          * probably just a time out error.
          * we close the TCP connection and break
          */
         if (errno ==   ETIMEDOUT)
         {
            result = SLP_NETWORK_TIMED_OUT;
         }
         else
         {
            result = SLP_NETWORK_ERROR;
         }
         closesocket(handle->unicastsock);
         goto FINISHED;
      }
      closesocket(handle->unicastsock);
      result = SLP_OK;
   }
   else
   {
      result = SLP_NETWORK_TIMED_OUT;
      goto FINISHED; /* Unsuccessful in opening a TCP connection just break */
   }

   /* Sneek in and check the XID */
   if (AS_UINT16(recvbuf->start + 10) == xid)
   {
      char addrstr[INET6_ADDRSTRLEN] = "";

      rplycount += 1;

      saddr_ntop(&addr, addrstr, sizeof(addrstr));

      /* Call the callback with the result and recvbuf */
      if (callback(result, &addr, recvbuf, cookie) == SLP_FALSE)
         goto CLEANUP; /* Caller doesn't want any more data. */

      /* Add the peer to the previous responder list. */
      if (prlistlen)
         strcat(prlist, ",");

      if (*addrstr != 0)
      {
         strcat(prlist, addrstr);
         prlistlen = strlen(prlist);
      }
   }

FINISHED:

   /* Notify the callback with SLP_LAST_CALL so that they know we're done */
   if (rplycount || result == SLP_NETWORK_TIMED_OUT)
      result = SLP_LAST_CALL;

   callback(result, 0, 0, cookie);
   if (result == SLP_LAST_CALL)
      result = SLP_OK;

CLEANUP:

   /* Free resources */
   xfree(prlist);
   SLPBufferFree(sendbuf);
   SLPBufferFree(recvbuf);

   return result;
}
#endif

/*===========================================================================
 *  TESTING CODE : compile with the following command lines:
 *
 *  $ gcc -g -DSLP_NETWORK_TEST -DDEBUG libslp_network.c 
 *
 *  C:\> cl -DSLP_NETWORK_TEST -DDEBUG libslp_network.c 
 */
#ifdef SLP_NETWORK_TEST

int main(int argc, char * argv[])
{
   // static routines
   int NetworkGetMcastAddrs(const char msgtype, uint8_t * msg, 
         SLPIfaceInfo * ifaceinfo)

   // non-static routines
   sockfd_t NetworkConnectToSlpd(void * peeraddr)
   void NetworkDisconnectDA(SLPHandleInfo * handle)
   void NetworkDisconnectSA(SLPHandleInfo * handle)
   sockfd_t NetworkConnectToDA(SLPHandleInfo * handle, const char * scopelist,
         size_t scopelistlen, void * peeraddr)
   sockfd_t NetworkConnectToSA(SLPHandleInfo * handle, const char * scopelist,
         size_t scopelistlen, void * saaddr)
   SLPError NetworkRqstRply(sockfd_t sock, void * peeraddr, 
         const char * langtag, size_t extoffset, void * buf, char buftype,
         size_t bufsize, NetworkRplyCallback callback, void * cookie)
   SLPError NetworkMcastRqstRply(SLPHandleInfo * handle, void * buf, 
         char buftype, size_t bufsize, NetworkRplyCallback callback,
         void * cookie)
   SLPError NetworkUcastRqstRply(SLPHandleInfo * handle, void * buf, 
         char buftype, size_t bufsize, NetworkRplyCallback callback, 
         void * cookie)
}
#endif

/*=========================================================================*/
