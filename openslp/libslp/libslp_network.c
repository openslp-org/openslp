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

/*#include <netinet/tcp.h>*/
#include "slp.h"
#include "libslp.h"
#include "slp_net.h"
#include "slp_network.h"
#include "slp_iface.h"
#include "slp_message.h"
#include "slp_v1message.h"
#include "slp_compare.h"
#include "slp_xmalloc.h"
#include "slp_xcast.h"
#include "slp_socket.h"

/*Maximum MTU value that can be specified in config. See getmtu() */
#define MAX_MTU 65535

/** Obtains the MTU value to be used from configuration.
 *
 * The maximum transmission unit size that is specified in configuration
 * determines many other parameter sizings.  This routines checks the
 * value for reasonability (against MAX_MTU) and either fails (debug)
 * or caps the value at MAX_MTU to enhance stability.
 *
 * @return The MTU use size from config.
 *
 * @internal
 */
static size_t getmtu(void)
{
   size_t mtu = SLPPropertyGetMTU();
   SLP_ASSERT(mtu <= MAX_MTU); /*Fail in debug*/
   if (mtu > MAX_MTU)          /*Cap in non debug*/
   {
      mtu = MAX_MTU;
   }
   return mtu;
}

#ifdef _WIN32

#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
struct timezone
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

/** A version of gettimeofday for Windows.
 *
 * @param[in] tp - timeval containing number of seconds/microseconds since Jan 1 1970
 * @param[in] tzp - time zone modifier -- Ignored, since the code isn't using it
 *
 * A version of gettimeofday for Windows.  Grabbed from MSDN.
 *
 * @internal
 */
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;
  static int tzflag;

  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);

    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;

    /*converting file time to unix epoch*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS;
    tmpres /= 10;  /*convert into microseconds*/
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }

  if (NULL != tz)
  {
    if (!tzflag)
    {
      _tzset();
      tzflag++;
    }
    tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }

  return 0;
}
#endif

/** Subtracts one timeval from another.
 *
 * @param[in] lhs - timeval to be subtracted from
 * @param[in] rhs - timeval to subtract
 *
 * Subtract the rhs from the lhs, putting the result into the lhs
 *
 * @internal
 */
static void timeval_subtract(struct timeval *lhs, struct timeval *rhs)
{
    lhs->tv_sec -= rhs->tv_sec;
    lhs->tv_usec -= rhs->tv_usec;
    if (lhs->tv_usec < 0)
    {
        lhs->tv_usec += 1000000;
        --lhs->tv_sec;
    }
}

/** Adds one timeval to another.
 *
 * @param[in] lhs - timeval to be added to
 * @param[in] rhs - timeval to add
 *
 * Add the rhs to the lhs, putting the result into the lhs
 *
 * @internal
 */
static void timeval_add(struct timeval *lhs, struct timeval *rhs)
{
    lhs->tv_sec += rhs->tv_sec;
    lhs->tv_usec += rhs->tv_usec;
    if (lhs->tv_usec >= 1000000)
    {
        lhs->tv_usec -= 1000000;
        ++lhs->tv_sec;
    }
}

/** Returns the appropriate buffer size for the various RequestReply functions
 *
 * @param[in] v1 - Whether or not this is a SLPv1 packet
 * @param[in] buftype - The message type
 * @param[in] langsize - the language tag size
 * @param[in] prlistlen - the current size of the prlist, only used for appropriate buftypes
 * @param[in] bufsize - The size of the message
 *
 * @return the size
 */
static size_t CalcBufferSize(int v1, char buftype, size_t langsize, size_t prlistlen, size_t bufsize)
{
   size_t size = 0;

   if(v1)
   {
      /*  1  Version
        + 1  Function-ID
        + 2  Length
        + 1  Flags/Reserved
        + 1  Dialect
        + 2  Language Code
        + 2  Char Encoding
        + 2  XID                  */
      size =  12;
   }
   else
   {
      /*  1  Version
        + 1  Function-ID
        + 3  Length
        + 2  Flags
        + 3  Extension Offset
        + 2  XID
        + 2  Lang Tag Len   */
      size = 14 + langsize;
   }

   size += bufsize;

   if (buftype == SLP_FUNCT_SRVRQST
         || buftype == SLP_FUNCT_ATTRRQST
         || buftype == SLP_FUNCT_SRVTYPERQST)
   {
         size += (2 + prlistlen); /* <PRList> Len/String  */
   }

   return size;
}

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
   uint16_t port;

   if (!ifaceinfo)
      return SLP_PARAMETER_BAD;

   port = (uint16_t)SLPPropertyAsInteger("net.slp.port");
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
                  &ifaceinfo->iface_addr[ifaceinfo->iface_count++]);
            SLPNetGetSrvMcastAddr(srvtype, srvtype_len, SLP_SCOPE_LINK_LOCAL,
                  &ifaceinfo->iface_addr[ifaceinfo->iface_count++]);
            SLPNetGetSrvMcastAddr(srvtype, srvtype_len, SLP_SCOPE_SITE_LOCAL,
                  &ifaceinfo->iface_addr[ifaceinfo->iface_count++]);
         }
         if (SLPNetIsIPV4())
         {
            struct in_addr mcastaddr;
            mcastaddr.s_addr = SLP_MCAST_ADDRESS;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET, port, &mcastaddr);
            ifaceinfo->iface_count++;
         }
         break;

      case SLP_FUNCT_ATTRRQST:
         if (SLPNetIsIPV6())
         {
            /* Add IPv6 multicast groups in order they should appear. */
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, port, &in6addr_srvloc_node);
            ifaceinfo->iface_count++;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, port, &in6addr_srvloc_link);
            ifaceinfo->iface_count++;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, port, &in6addr_srvloc_site);
            ifaceinfo->iface_count++;
         }
         if (SLPNetIsIPV4())
         {
            struct in_addr mcastaddr;
            mcastaddr.s_addr = SLP_MCAST_ADDRESS;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET, port, &mcastaddr);
            ifaceinfo->iface_count++;
         }
         break;

      case SLP_FUNCT_SRVTYPERQST:
         if (SLPNetIsIPV6())
         {
            /* Add IPv6 multicast groups in order they should appear. */
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, port, &in6addr_srvloc_node);
            ifaceinfo->iface_count++;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, port, &in6addr_srvloc_link);
            ifaceinfo->iface_count++;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, port, &in6addr_srvloc_site);
            ifaceinfo->iface_count++;
         }
         if (SLPNetIsIPV4())
         {
            struct in_addr mcastaddr;
            mcastaddr.s_addr = SLP_MCAST_ADDRESS;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET, port, &mcastaddr);
            ifaceinfo->iface_count++;
         }
         break;

      case SLP_FUNCT_DASRVRQST:
         if (SLPNetIsIPV6())
         {
            /* Add IPv6 multicast groups in order they should appear. */
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, port, &in6addr_srvlocda_node);
            ifaceinfo->iface_count++;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, port, &in6addr_srvlocda_link);
            ifaceinfo->iface_count++;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET6, port, &in6addr_srvlocda_site);
            ifaceinfo->iface_count++;
         }
         if (SLPNetIsIPV4())
         {
            struct in_addr mcastaddr;
            mcastaddr.s_addr = SLP_MCAST_ADDRESS;
            SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count],
                  AF_INET, port, &mcastaddr);
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

   /* Note that these don't actually test the connection to slpd.
    * They don't have to, since all code that calls this function eventually
    * does a NetworkRqstRply, which has retry logic for the datagram case.
    */
   struct timeval timeout;
   timeout.tv_sec = SLPPropertyAsInteger(SLPGetProperty("net.slp.randomWaitBound"));
   timeout.tv_usec = (timeout.tv_sec % 1000) * 1000;
   timeout.tv_sec = timeout.tv_sec / 1000;

   if (SLPNetIsIPV6())
      if (!SLPNetSetAddr(peeraddr, AF_INET6,
            (uint16_t)SLPPropertyAsInteger("net.slp.port"),
            &slp_in6addr_loopback))
         sock = SLPNetworkConnectStream(peeraddr, &timeout);                

   if (sock == SLP_INVALID_SOCKET && SLPNetIsIPV4())
   {
      int tempAddr = INADDR_LOOPBACK;
      if (SLPNetSetAddr(peeraddr, AF_INET,
            (uint16_t)SLPPropertyAsInteger("net.slp.port"), &tempAddr) == 0)
      {
         sock = SLPNetworkConnectStream(peeraddr, &timeout);
      }
   }
   return sock;
}

/** Check if the socket is still alive - the server may have closed it.
 *
 * @param[in] fd - The socket decriptor to check.
 *
 * @return SLP_OK if socket is still alive; SLP_NETWORK_ERROR if not.
 */
static SLPError NetworkCheckConnection(sockfd_t fd)
{
   int r;
#ifdef HAVE_POLL
    struct pollfd readfd;
#else
    fd_set readfd;
    struct timeval tv;
#endif

#ifdef HAVE_POLL
    readfd.fd = fd;
    readfd.events = POLLIN;
    while ((r = poll(&readfd, 1, 0)) == -1 && errno == EINTR)
        ;
#else
   FD_ZERO(&readfd);
   FD_SET(fd, &readfd);
   tv.tv_sec = 0;
   tv.tv_usec = 0;
   while ((r = select((int)(fd + 1), &readfd, 0, 0, &tv)) == -1)
   {
#ifdef _WIN32
     if (WSAGetLastError() != WSAEINTR)
#else
     if (errno != EINTR)
#endif
        break;
   }
#endif
   /* r == 0 means timeout, everything else is an error */
   return r == 0 ? SLP_OK : SLP_NETWORK_ERROR;
}

/** Disconnect from the connected DA.
 *
 * Called after DA fails to respond.
 *
 * @param[in] handle - The SLP handle containing the socket to close.
 */
void NetworkDisconnectDA(SLPHandleInfo * handle)
{
   if (handle->dasock != SLP_INVALID_SOCKET)
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
   if (handle->sasock != SLP_INVALID_SOCKET)
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
         && SLPSubsetStringList(handle->dascopelen, handle->dascope,
               scopelistlen, scopelist) != 0
         && NetworkCheckConnection(handle->dasock) == SLP_OK)
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
         && SLPSubsetStringList(handle->sascopelen, handle->sascope,
               scopelistlen, scopelist) != 0
         && NetworkCheckConnection(handle->sasock) == SLP_OK)
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
 * @param[in] isV1 - Whether or not to use a V1 header.
 *
 * @return SLP_OK on success, or an SLP error code on failure.
 */
SLPError NetworkRqstRply(sockfd_t sock, void * peeraddr,
      const char * langtag, size_t extoffset, void * buf, char buftype,
      size_t bufsize, NetworkRplyCallback callback, void * cookie, int isV1)
{
   char * prlist = 0;
   unsigned short flags;
   char v1flags;

   size_t mtu = 0;
   size_t prlistlen = 0;
   SLPBuffer sendbuf = 0;
   SLPBuffer recvbuf = 0;
   SLPError result = SLP_OK;

   int looprecv;   /*If non-zero, keep receiving until you timeout/error, etc.*/
   int stoploopifrecv;   /*... but if this is non-zero, stop on the first valid receipt*/
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
      stoploopifrecv = 0;     /* Several responders would respond to the multicast */
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
         stoploopifrecv = 1;           /* The peer has sent the single response. */
      }
      else
      {
         xmitcount = MAX_RETRANSMITS -1;  /* Streams manage own retries, so we want the loop to iterate once. */
         looprecv  = 0;
         stoploopifrecv = 0;
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
      stoploopifrecv = 0;  /* These replies are separate datagram responses. */
   }

   /* Allocate memory for the prlist on datagram sockets for appropriate
    * messages. Note that the prlist is as large as the MTU -- thus assuring
    * that there will not be any buffer overwrites regardless of how many
    * previous responders there are. This is because the retransmit code
    * terminates if ever MTU is exceeded for any datagram message.
    */
   mtu = getmtu();
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
   while (xmitcount < MAX_RETRANSMITS)
   {
      size_t size;
      struct timeval timeout;
      result = SLP_OK;

      /* Setup receive timeout. */
      if (socktype == SOCK_DGRAM)
      {
         /*If we've already received replies from the peer, no need to resend.*/
         if((rplycount != 0) && (!SLPNetIsMCast(peeraddr)))
            break;

         totaltimeout += timeouts[xmitcount];
         if (totaltimeout >= maxwait || !timeouts[xmitcount])
         {
            result = SLP_NETWORK_TIMED_OUT;
            break; /* Max timeout exceeded - we're done. */
         }

         timeout.tv_sec = timeouts[xmitcount] / 1000;
         timeout.tv_usec = (timeouts[xmitcount] % 1000) * 1000;
      }
      else
      {
         timeout.tv_sec = maxwait / 1000;
         timeout.tv_usec = (maxwait % 1000) * 1000;
      }

      size = CalcBufferSize(isV1, buftype, langtaglen, prlistlen, bufsize);

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
      xmitcount++;

      /* -- Begin SLP Header -- */
      if(isV1)
      {
         /* Version */
         *sendbuf->curpos++ = 1;

         /* Function-ID */
         *sendbuf->curpos++ = buftype;

         /* Length */
         PutUINT16(&sendbuf->curpos, size);

         /* flags */
         v1flags = 0;
         if (buftype == SLP_FUNCT_SRVREG)
            v1flags |= SLPv1_FLAG_FRESH;
         *sendbuf->curpos++ = v1flags;

         /* dialect */
         *sendbuf->curpos++ = 0;

         /* Language code.  For now, we'll take the first two bytes of the tag */
         if(langtaglen < 2)
         {
            *sendbuf->curpos++ = 'e';
            *sendbuf->curpos++ = 'n';
         }
         else
         {
            *sendbuf->curpos++ = langtag[0];
            *sendbuf->curpos++ = langtag[1];
         }

         /* Character encoding -- assume UTF8 */
         PutUINT16(&sendbuf->curpos, SLP_CHAR_UTF8);

         /* XID */
         PutUINT16(&sendbuf->curpos, xid);
      }
      else
      {
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
      }
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
                  size_t addrstrlen;
                  char addrstr[INET6_ADDRSTRLEN] = "";
                  SLPNetSockAddrStorageToString(&addr,
                           addrstr, sizeof(addrstr));
                  addrstrlen = strlen(addrstr);
                  if (addrstrlen > 0 && SLPContainsStringList(prlistlen,
                           prlist, addrstrlen, addrstr) == 0)
                  {
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

               if(stoploopifrecv)
                  break;
            }
         }
      } while (looprecv);
   }

FINISHED:

   /* Notify the callback that we're done. */
   if (rplycount != 0 || (result == SLP_NETWORK_TIMED_OUT
         && SLPNetIsMCast(peeraddr)))
      result = SLP_LAST_CALL;

   if(recvbuf)
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
 * @param[in] isV1 - Whether or not to use a v1 header.
 *
 * @return SLP_OK on success. SLP_ERROR on failure.
 */
SLPError NetworkMcastRqstRply(SLPHandleInfo * handle, void * buf,
      char buftype, size_t bufsize, NetworkRplyCallback callback,
      void * cookie, int isV1)
{
   struct timeval timeout;
   struct sockaddr_storage addr;
   SLPBuffer sendbuf = 0;
   SLPBuffer recvbuf = 0;
   SLPError result = 0;
   size_t langtaglen = 0;
   size_t prlistlen = 0;
   size_t prlistsize = 0;
   int size = 0;
   char * prlist = 0;
   int xid = 0;
   size_t mtu = 0;
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
   int alistsize;
   int currIntf = 0;

#if defined(DEBUG)
   /* This function only supports multicast or broadcast of these messages */
   if (buftype != SLP_FUNCT_SRVRQST && buftype != SLP_FUNCT_ATTRRQST
         && buftype != SLP_FUNCT_SRVTYPERQST && buftype != SLP_FUNCT_DASRVRQST)
      return SLP_PARAMETER_BAD;
#endif

   /* save off a few things we don't want to recalculate */
   langtaglen = strlen(handle->langtag);

   /* initialize pointers freed on error */
   dstifaceinfo.iface_addr = NULL;
   dstifaceinfo.bcast_addr = NULL;
   v4outifaceinfo.iface_addr = NULL;
   v4outifaceinfo.bcast_addr = NULL;
   v6outifaceinfo.iface_addr = NULL;
   v6outifaceinfo.bcast_addr = NULL;
   xcastsocks.sock = NULL;
   xcastsocks.peeraddr = NULL;

   xid = SLPXidGenerate();
   mtu = getmtu();
   sendbuf = SLPBufferAlloc(mtu);
   if (!sendbuf)
   {
      result = SLP_MEMORY_ALLOC_FAILED;
      goto FINISHED;
   }

   alistsize = slp_max_ifaces * sizeof(struct sockaddr_storage);

   dstifaceinfo.iface_count = 0;
   dstifaceinfo.iface_addr = malloc(alistsize);
   if (dstifaceinfo.iface_addr == NULL)
   {
      result = SLP_MEMORY_ALLOC_FAILED;
      goto FINISHED;
   }
   dstifaceinfo.bcast_addr = malloc(alistsize);
   if (dstifaceinfo.bcast_addr == NULL)
   {
      result = SLP_MEMORY_ALLOC_FAILED;
      goto FINISHED;
   }
   v4outifaceinfo.iface_count = 0;
   v4outifaceinfo.iface_addr = malloc(alistsize);
   if (v4outifaceinfo.iface_addr == NULL)
   {
      result = SLP_MEMORY_ALLOC_FAILED;
      goto FINISHED;
   }
   v4outifaceinfo.bcast_addr = malloc(alistsize);
   if (v4outifaceinfo.bcast_addr == NULL)
   {
      result = SLP_MEMORY_ALLOC_FAILED;
      goto FINISHED;
   }
   v6outifaceinfo.iface_count = 0;
   v6outifaceinfo.iface_addr = malloc(alistsize);
   if (v6outifaceinfo.iface_addr == NULL)
   {
      result = SLP_MEMORY_ALLOC_FAILED;
      goto FINISHED;
   }
   v6outifaceinfo.bcast_addr = malloc(alistsize);
   if (v6outifaceinfo.bcast_addr == NULL)
   {
      result = SLP_MEMORY_ALLOC_FAILED;
      goto FINISHED;
   }
   xcastsocks.sock_count = 0;
   xcastsocks.sock = malloc(slp_max_ifaces * sizeof(sockfd_t));
   if (xcastsocks.sock == NULL)
   {
      result = SLP_MEMORY_ALLOC_FAILED;
      goto FINISHED;
   }
   xcastsocks.peeraddr = malloc(alistsize);
   if (xcastsocks.peeraddr == NULL)
   {
      result = SLP_MEMORY_ALLOC_FAILED;
      goto FINISHED;
   }

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
   prlistsize = mtu;
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
      while (xmitcount < MAX_RETRANSMITS)
      {
         int replies_this_period = 0;
         totaltimeout += timeouts[xmitcount];
         if (totaltimeout > maxwait || !timeouts[xmitcount])
            break; /* we are all done */

         timeout.tv_sec = timeouts[xmitcount] / 1000;
         timeout.tv_usec = (timeouts[xmitcount] % 1000) * 1000;

         /* re-allocate buffer and make sure that the send buffer does not
          * exceed MTU for datagram transmission
          */
         size = (int)CalcBufferSize(isV1, buftype, langtaglen, prlistlen, bufsize);

         if (size > (int)mtu)
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
         xmitcount++;

         /* Add the header to the send buffer */
         if(isV1)
         {
            /* Version */
            *sendbuf->curpos++ = 1;

            /* Function-ID */
            *sendbuf->curpos++ = buftype;

            /* Length */
            PutUINT16(&sendbuf->curpos, size);

            /* flags */
            *sendbuf->curpos++ = 0;

            /* dialect */
            *sendbuf->curpos++ = 0;

            /* Language code.  For now, we'll take the first two bytes of the tag */
            if(langtaglen < 2)
            {
               *sendbuf->curpos++ = 'e';
               *sendbuf->curpos++ = 'n';
            }
            else
            {
               *sendbuf->curpos++ = handle->langtag[0];
               *sendbuf->curpos++ = handle->langtag[1];
            }

            /* Character encoding -- assume UTF8 */
            PutUINT16(&sendbuf->curpos, SLP_CHAR_UTF8);

            /* XID */
            PutUINT16(&sendbuf->curpos, xid);
         }
         else
         {
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
         }

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

         /* main recv loop */
         while(1)
         {
#if !defined(UNICAST_NOT_SUPPORTED)
            int retval = 0;
            if ((retval = SLPXcastRecvMessage(&xcastsocks, &recvbuf,
                  &addr, &timeout)) != SLP_ERROR_OK)
#else
            if (SLPXcastRecvMessage(&xcastsocks, &recvbuf,
                  &addr, &timeout) != SLP_ERROR_OK)
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
                  /* Use a local timeout variable here so we don't corrupt the multicast timeout */
                  struct timeval timeout;
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
            /* Sneek in and check the XID -- it's in the same place in v1 and v2*/
            if (AS_UINT16(recvbuf->start + 10) == xid)
            {
               char addrstr[INET6_ADDRSTRLEN] = "";
               size_t addrstrlen;

               SLPNetSockAddrStorageToString(&addr, addrstr, sizeof(addrstr));

               ++replies_this_period;
               rplycount += 1;

               /* Call the callback with the result and recvbuf */
#if !defined(MI_NOT_SUPPORTED)
               if (!cookie)
                  cookie = (SLPHandleInfo *)handle;
#endif
               if (callback(result, &addr, recvbuf, cookie) == SLP_FALSE)
                  goto CLEANUP; /* Caller does not want any more info */

               /* add the peer to the previous responder list */
               addrstrlen = strlen(addrstr);
               if (addrstrlen > 0 && SLPContainsStringList(prlistlen,
                           prlist, addrstrlen, addrstr) == 0)
               {
                  if (prlistlen + 1 + addrstrlen >= prlistsize)
                  {
                     prlist = xrealloc(prlist, prlistsize + mtu);
                     prlistsize += mtu;
                  }
                  if (prlistlen != 0)
                     strcat(prlist, ",");
                  strcat(prlist, addrstr);
                  prlistlen = strlen(prlist);
               }
            }
         }
         SLPXcastSocketsClose(&xcastsocks);
         if (!replies_this_period && (xmitcount > 1))
         {
             /* stop after a period with no replies, but wait at least two periods */
             break;
         }
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
   xfree(xcastsocks.sock);
   xfree(xcastsocks.peeraddr);
   xfree(dstifaceinfo.iface_addr);
   xfree(dstifaceinfo.bcast_addr);
   xfree(v4outifaceinfo.iface_addr);
   xfree(v4outifaceinfo.bcast_addr);
   xfree(v6outifaceinfo.iface_addr);
   xfree(v6outifaceinfo.bcast_addr);

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
 * @param[in] isV1 - Whether or not to use a v1 header.
 *
 * @return SLP_OK on success. SLP_ERROR on failure.
 */
SLPError NetworkUcastRqstRply(SLPHandleInfo * handle, void * buf,
      char buftype, size_t bufsize, NetworkRplyCallback callback,
      void * cookie, int isV1)
{
   /*In reality, this function just sets things up for NetworkRqstRply to operate*/

   if(handle->unicastsock == SLP_INVALID_SOCKET) /*The unicast code will certainly reuse this socket*/
      handle->unicastsock  = SLPNetworkCreateDatagram(handle->ucaddr.ss_family);

   if (handle->unicastsock == SLP_INVALID_SOCKET)
      return SLP_NETWORK_ERROR;

   return NetworkRqstRply(handle->unicastsock, &handle->ucaddr, handle->langtag, 0, buf, buftype, bufsize, callback, cookie, isV1);
}

/** Set the given socket to be non-blocking
 *
 * @param[in] sock - the socket to be set up
 *
 * @internal
 */
static int SetNonBlocking(int sock)
{
#ifdef _WIN32
   u_long fdflags = 1;
   return ioctlsocket(sock, FIONBIO, &fdflags);
#else
   int fdflags = fcntl(sock, F_GETFL, 0);
   return fcntl(sock, F_SETFL, fdflags | O_NONBLOCK);
#endif
}

/** Transmit and receive a unicast SLP message to the list of addresses given
 *
 * @param[in] destaddr - pointer to the array of IP addresses of the hosts
 *               to send the message to
 * @param[in] langtag - language tag to use in SLP message header
 * @param[in] buf - pointer to the portion of the SLP message to send,
 *               excluding the SLP header  ie. everything after the prlist
 * @param[in] buftype - the function-id to use in the SLPMessage header
 * @param[in] bufsize - the size of the buffer pointed to by buf
 * @param[in] callback - the callback to use for reporting results
 * @param[in] cookie - the cookie to pass to the callback
 * @param[in] isV1 - Whether or not to use a V1 header.
 *
 * @return SLP_OK on success
 *
 * @internal
 */
SLPError NetworkMultiUcastRqstRply(
                         struct sockaddr_in* destaddr,		// This is an array of addresses
                         const char* langtag,
                         char* buf,
                         char buftype,
                         size_t bufsize,
                         NetworkRplyCallback callback,
                         void * cookie,
                         int isV1)
{
    /* Minimum amount of data we need to read to get the packet length */
    #define             MIN_RECEIVE_SIZE  5

    /* Connection state values */
    #define CONN_UDP                      0
    #define CONN_TCP_CONNECT              1
    #define CONN_TCP_SEND                 2
    #define CONN_TCP_RECEIVE              3
    #define CONN_COMPLETE                 4
    #define CONN_FAILED                   5

    int                 i;
    SLPBuffer           sendbuf         = 0;
    SLPBuffer           udp_recvbuf     = 0;
    SLPError            result          = SLP_OK;
    size_t              langtaglen      = 0;
    int                 prlistlen       = 0;
    int                 xid             = 0;
    size_t              mtu             = 0;
    int                 ndests          = 0;    // Number of destinations
    sockfd_t            nfds            = 0;    // Number of file descriptors in the FD_SET
    size_t              send_size       = 0;
    int                 selected        = 0;
    int                 xmitcount       = 0;
    int                 rplycount       = 0;
    int                 maxwait         = 0;
    int                 timeouts[MAX_RETRANSMITS];
    sockfd_t            udp_socket      = (sockfd_t)-1;
    int                 udp_active      = 0;
    int                 do_send         = 1;
    unsigned short      flags;
    char                v1flags;
    unsigned int        msglen;
    struct timeval      now;
    struct timeval      timeout;
    struct timeval      timeout_end;
    struct timeval      max_timeout_end;
    struct sockaddr_in  udp_bind_address;
    char                peek[MIN_RECEIVE_SIZE];
    struct sockaddr_in  peeraddr;
    socklen_t           peeraddrlen = sizeof (struct sockaddr_in);
    struct _UcastConnection {
        int         state;
        sockfd_t    socket;
        int         send_offset;
        int         recv_offset;
        int         recv_size;
        SLPBuffer   read_buffer;
    }                   *pconnections   = 0;

#ifdef DEBUG
    /* This function only supports unicast of the following messages
     */
    if(buftype != SLP_FUNCT_SRVRQST &&
       buftype != SLP_FUNCT_ATTRRQST &&
       buftype != SLP_FUNCT_SRVTYPERQST &&
       buftype != SLP_FUNCT_DASRVRQST)
    {
        return SLP_PARAMETER_BAD;
    }
#endif

#ifdef _WIN32
    /*windows gives a warning that timeout_end may not be initted --
      apparently it doesn't understand while(1)*/
    timeout_end.tv_sec = 0;
    timeout_end.tv_usec = 0;
#endif

    /*----------------------------------------------------*/
    /* Save off a few things we don't want to recalculate */
    /*----------------------------------------------------*/
    langtaglen = (int)strlen(langtag);
    xid = SLPXidGenerate();
    mtu = getmtu();
    sendbuf = SLPBufferAlloc(mtu);
    if(sendbuf == 0)
    {
        result = SLP_MEMORY_ALLOC_FAILED;
        goto CLEANUP;
    }
    udp_recvbuf = SLPBufferAlloc(mtu);
    if(udp_recvbuf == 0)
    {
        result = SLP_MEMORY_ALLOC_FAILED;
        goto CLEANUP;
    }
    maxwait = SLPPropertyAsInteger("net.slp.unicastMaximumWait");
    SLPPropertyAsIntegerVector("net.slp.unicastTimeouts",
                               timeouts,
                               MAX_RETRANSMITS );

    /* Special case for fake SLP_FUNCT_DASRVRQST */
    if(buftype == SLP_FUNCT_DASRVRQST)
    {
        /* do something special for SRVRQST that will be discovering DAs */
        maxwait = SLPPropertyAsInteger("net.slp.DADiscoveryMaximumWait");
        SLPPropertyAsIntegerVector("net.slp.DADiscoveryTimeouts",
                                   timeouts,
                                   MAX_RETRANSMITS );
        /* SLP_FUNCT_DASRVRQST is a fake function.  We really want to */
        /* send a SRVRQST                                             */
        buftype  = SLP_FUNCT_SRVRQST;
    }

    /*---------------------------------------------------------------------*/
    /* Don't need to allocate memory for the prlist, as the message is     */
    /* only unicast to each of the destinations                            */
    /*---------------------------------------------------------------------*/
    prlistlen = 0;

    /*------------------------------------------*/
    /* Allocate the connection structures array */
    /*------------------------------------------*/
    /* First find out how many there are        */
    for (ndests = 0; ; ndests++)
    {
        if (destaddr[ndests].sin_addr.s_addr == 0)
        {
            break;
        }
    }
    /* Now allocate the array                   */
    pconnections = (struct _UcastConnection*)xmalloc(ndests * sizeof pconnections[0]);
    if(pconnections == 0)
    {
        result = SLP_MEMORY_ALLOC_FAILED;
        goto CLEANUP;
    }

    /*--------------------------------------------*/
    /* Initialise the connection structures array */
    /* and initiate the connections               */
    /*--------------------------------------------*/
    for (i = 0; i < ndests; i++)
    {
        struct _UcastConnection *pconn = &pconnections[i];
        pconn->state = CONN_UDP;
        pconn->socket = (sockfd_t)-1;
        pconn->send_offset = 0;
        pconn->recv_offset = 0;
        pconn->recv_size = MIN_RECEIVE_SIZE;            /* enough to receive the length field */
        pconn->read_buffer = NULL;
    }
    if (result != SLP_OK)
        goto FINISHED;

    /*----------------------------------------*/
    /* Create a UDP socket to use             */
    /*----------------------------------------*/
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0)
    {
        result = SLP_NETWORK_ERROR;
        goto FINISHED;
    }

    SLPNetworkSetSndRcvBuf(udp_socket);

    udp_bind_address.sin_family = AF_INET;
    udp_bind_address.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_bind_address.sin_port = htons(0);
    if (bind(udp_socket, (struct sockaddr *)&udp_bind_address, sizeof udp_bind_address) < 0)
    {
        result = SLP_NETWORK_ERROR;
        goto FINISHED;
    }

    /*----------------------------------------*/
    /* re-allocate buffer if necessary        */
    /*----------------------------------------*/
    send_size = (int)CalcBufferSize(isV1, buftype, langtaglen, prlistlen, bufsize);
    if (send_size > mtu)
    {
        if((sendbuf = SLPBufferRealloc(sendbuf,send_size)) == 0)
        {
            result = SLP_MEMORY_ALLOC_FAILED;
            goto CLEANUP;
        }
    }

    /*-----------------------------------*/
    /* Add the header to the send buffer */
    /*-----------------------------------*/
    if(isV1)
    {
         /* Version */
         *sendbuf->curpos++ = 1;
         /* Function-ID */
         *sendbuf->curpos++ = buftype;
         /* Length */
         PutUINT16(&sendbuf->curpos, send_size);
         /* flags */
         v1flags = 0;
         if (buftype == SLP_FUNCT_SRVREG)
            v1flags |= SLPv1_FLAG_FRESH;
         *sendbuf->curpos++ = v1flags;
         /* dialect */
         *sendbuf->curpos++ = 0;
         /* Language code.  For now, we'll take the first two bytes of the tag */
         if(langtaglen < 2)
         {
            *sendbuf->curpos++ = 'e';
            *sendbuf->curpos++ = 'n';
         }
         else
         {
            *sendbuf->curpos++ = langtag[0];
            *sendbuf->curpos++ = langtag[1];
         }
         /* Character encoding -- assume UTF8 */
         PutUINT16(&sendbuf->curpos, SLP_CHAR_UTF8);
         /* XID */
         PutUINT16(&sendbuf->curpos, xid);
    }
    else
    {
       /*version*/
       sendbuf->curpos = sendbuf->start ;
       *(sendbuf->curpos++)       = 2;
       /*function id*/
       *(sendbuf->curpos++)   = buftype;
       /*length*/
       PutUINT24(&sendbuf->curpos, send_size);
       /*flags*/
       flags = 0;
       if (buftype == SLP_FUNCT_SRVREG)
       {
           flags |= SLP_FLAG_FRESH;
       }
       PutUINT16(&sendbuf->curpos, flags);
       /*ext offset*/
       PutUINT24(&sendbuf->curpos, 0);
       /*xid*/
       PutUINT16(&sendbuf->curpos, xid);
       /*lang tag len*/
       PutUINT16(&sendbuf->curpos,langtaglen);
       /*lang tag*/
       memcpy(sendbuf->curpos, langtag, langtaglen);
       sendbuf->curpos = sendbuf->curpos + langtaglen ;
    }

    /*-----------------------------------------------*/
    /* Add the zero length prlist to the send buffer */
    /*-----------------------------------------------*/
    PutUINT16(&sendbuf->curpos,prlistlen);

    /*-----------------------------*/
    /* Add the rest of the message */
    /*-----------------------------*/
    memcpy(sendbuf->curpos, buf, bufsize);
    sendbuf->curpos = sendbuf->curpos + bufsize;

    /*----------------------------------------*/
    /* Calculate the max timeout end          */
    /*----------------------------------------*/
    timeout.tv_sec = maxwait / 1000;
    timeout.tv_usec = (maxwait % 1000) * 1000;
    gettimeofday(&max_timeout_end, 0);
    timeval_add(&max_timeout_end, &timeout);

    /*--------------------------*/
    /* Main processing loop     */
    /*--------------------------*/
    while(1)
    {
        fd_set read_fds;
        fd_set write_fds;

        if (do_send)
        {
            /*---------------------------------*/
            /* Send the UDP messages out       */
            /*---------------------------------*/
            for (i = 0; i < ndests; i++)
            {
                struct _UcastConnection *pconn = &pconnections[i];
                if (pconn->state == CONN_UDP)
                {
                    int flags = 0;

#if defined(MSG_NOSIGNAL)
                    flags = MSG_NOSIGNAL;
#endif
                    if (sendto(udp_socket,
                               (char*)sendbuf->start,
                               (int)(sendbuf->curpos - sendbuf->start),
                               flags,
                               (struct sockaddr *)&destaddr[i],
                               sizeof destaddr[i]) < 0)
                    {
                        result = SLP_NETWORK_ERROR;
                        goto FINISHED;
                    }
                }
            }

            /*----------------------------------------*/
            /* Calculate the end point of the timeout */
            /* for this period                        */
            /*----------------------------------------*/
            timeout.tv_sec = timeouts[xmitcount] / 1000;
            timeout.tv_usec = (timeouts[xmitcount] % 1000) * 1000;
            gettimeofday(&timeout_end, 0);
            timeval_add(&timeout_end, &timeout);

            do_send = 0;
        }

        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        /*---------------------------------*/
        /* Mark the active sockets         */
        /*---------------------------------*/
        nfds = 0;
        udp_active = 0;
        for (i = 0; i < ndests; i++)
        {
            struct _UcastConnection *pconn = &pconnections[i];
            if ((pconn->state == CONN_TCP_CONNECT) || (pconn->state == CONN_TCP_SEND))
            {
                FD_SET(pconn->socket, &write_fds);
                if (nfds <= pconn->socket)
                    nfds = pconn->socket + 1;
            }
            else if (pconn->state == CONN_TCP_RECEIVE)
            {
                FD_SET(pconn->socket, &read_fds);
                if (nfds <= pconn->socket)
                    nfds = pconn->socket + 1;
            }
            else if (pconn->state == CONN_UDP)
            {
                udp_active = 1;
            }
        }
        if (udp_active)
        {
            FD_SET(udp_socket, &read_fds);
            if (nfds <= udp_socket)
                nfds = udp_socket + 1;
        }

        if (nfds == 0)
        {
            /* No active sockets - must all have failed or completed */
            break;
        }

        /*---------------------------------*/
        /* Calculate the remaining timeout */
        /* for this period                 */
        /*---------------------------------*/
        gettimeofday(&now, 0);
        memcpy(&timeout, &timeout_end, sizeof (struct timeval));
        timeval_subtract(&timeout, &now);
        if ((timeout.tv_sec < 0) || (timeout.tv_sec > (maxwait/1000)))
        {
            /* timeout has passed */
            timeout.tv_sec  = 0;
            timeout.tv_usec = 0;
        }

        /*---------------------------------*/
        /* Wait for something to do        */
        /*---------------------------------*/
        selected = select((int)nfds,
                          &read_fds,
                          &write_fds,
                          0,
                          &timeout);

        /*----------------------------------*/
        /* Check for termination conditions */
        /*----------------------------------*/
        if (selected == 0)
        {
            /* Nothing in the remainder of the timeout period */
            ++xmitcount;
            gettimeofday(&now, NULL);

            /* First check whether the max timeout has been exceeded */
            if (now.tv_sec >= max_timeout_end.tv_sec)
            {
                if ((now.tv_sec > max_timeout_end.tv_sec) || (now.tv_usec >= max_timeout_end.tv_usec))
                {
                    /* Timed out */
                    result = SLP_NETWORK_TIMED_OUT;
                    break;
                }
            }

            /* Next check whether we've used up all the retry periods */
            if (xmitcount >= MAX_RETRANSMITS)
            {
                /* Timed out */
                result = SLP_NETWORK_TIMED_OUT;
                break;
            }

            /* OK - resend to all the UDP destinations that haven't replied yet */
            do_send = 1;

        }
        if (selected < 0)
        {
#ifdef _WIN32
            if (WSAGetLastError() == WSAEINTR)
#else
            if (errno == EINTR)
#endif
                /* interrupted - just carry on */
                continue;
            /* error - can't carry on */
            result = SLP_NETWORK_ERROR;
            break;
        }

        /*---------------------------------------*/
        /* Check whether the UDP socket has data */
        /* and from whom                         */
        /*---------------------------------------*/
        udp_active = 0;
        if (FD_ISSET(udp_socket, &read_fds))
        {
            ssize_t bytesread;

            /* Peek at the first few bytes of the header */
            bytesread = recvfrom(udp_socket,
                                 peek,
                                 MIN_RECEIVE_SIZE,
                                 MSG_PEEK,
                                 (struct sockaddr *)&peeraddr,
                                 &peeraddrlen);
            if (!bytesread)
            {
               result = SLP_NETWORK_ERROR;
               break;
            }
            if (bytesread == MIN_RECEIVE_SIZE
#ifdef _WIN32
            /* Win32 returns WSAEMSGSIZE if the message is larger than
             * the requested size, even with MSG_PEEK. But if this is the
             * error code we can be sure that the message is at least the
             * required number of bytes
             */
               || (bytesread == (size_t)-1 && WSAGetLastError() == WSAEMSGSIZE)
#endif
               )
            {
                msglen = PEEK_LENGTH(peek);

                if(msglen <=  (unsigned int)mtu)
                {
                    udp_recvbuf = SLPBufferRealloc(udp_recvbuf, msglen);
                    if (udp_recvbuf == 0)
                    {
                       result = SLP_MEMORY_ALLOC_FAILED;
                       goto CLEANUP;
                    }
                    bytesread = recv(udp_socket,
                                     (char*)udp_recvbuf->curpos,
                                     (int)(udp_recvbuf->end - udp_recvbuf->curpos),
                                     0);
                    if(bytesread != (int32_t)msglen)
                    {
                        /* This should never happen but we'll be paranoid*/
                        udp_recvbuf->end = udp_recvbuf->curpos + bytesread;
                    }

                    /* Message read. We're done! */
                    udp_active = 1;
                    result = SLP_OK;
                }
                else
                {
                    /* we got a bad message, or one that is too big! */
#ifdef UNICAST_NOT_SUPPORTED
                    /* We need to read the message to clear the UDP socket */
                    bytesread = recv(udp_socket,
                                     udp_recvbuf->start,
                                     1,
                                     0);
#else
                    /* Reading MTU bytes on the socket */
                    if ((udp_recvbuf = SLPBufferRealloc(udp_recvbuf, mtu)) == 0)
                    {
                       result = SLP_MEMORY_ALLOC_FAILED;
                       goto CLEANUP;
                    }
                    bytesread = recv(udp_socket,
                                     (char*)udp_recvbuf->curpos,
                                     (int)(udp_recvbuf->end - udp_recvbuf->curpos),
                                     0);
                    if (bytesread != (int)mtu)
                    {
                        /* This should never happen but we'll be paranoid*/
                        udp_recvbuf->end = udp_recvbuf->curpos + bytesread;
                    }
                    udp_active = 1;
                    result = SLP_ERROR_RETRY_UNICAST;
#endif
                }
            }
            else
            {
                /* Not even the minimum bytes available - read and discard */
                bytesread = recv(udp_socket,
                                 (char*)udp_recvbuf->start,
                                 1,
                                 0);
            }
        }

        /*---------------------------------------*/
        /* Check which connections need handling */
        /*---------------------------------------*/
        for (i = 0; (i < ndests) && selected; i++)
        {
            struct _UcastConnection *pconn = &pconnections[i];
            if ((pconn->socket >= 0) && FD_ISSET(pconn->socket, &write_fds))
            {
                /*---------------------------------------*/
                /* Handle a TCP write or connect         */
                /*---------------------------------------*/
                --selected;
                if ((pconn->state == CONN_TCP_CONNECT) || (pconn->state == CONN_TCP_SEND))
                {
                    int bytes_sent;

                    pconn->state = CONN_TCP_SEND;
                     bytes_sent = send(pconn->socket,
                                      (char*)sendbuf->start+pconn->send_offset,
                                      (int)(send_size-pconn->send_offset),
                                      0);
                    if (bytes_sent > 0)
                    {
                        pconn->send_offset += bytes_sent;
                        if (pconn->send_offset >= (int)send_size)
                        {
                            pconn->state = CONN_TCP_RECEIVE;
                        }
                    }
                    else if (bytes_sent < 0)
                    {
                        /* Shouldn't get this if the socket is not in error */
                        pconn->state = CONN_FAILED;
                    }
                }
                else
                {
                    /* Should never get here */
                    pconn->state = CONN_FAILED;
                }
            }
            else if ((pconn->socket >= 0) && FD_ISSET(pconn->socket, &read_fds))
            {
                /*---------------------------------------*/
                /* Handle received TCP data              */
                /*---------------------------------------*/
                --selected;
                if (pconn->state == CONN_TCP_RECEIVE)
                {
                    int size_to_read = pconn->recv_size - pconn->recv_offset;
                    int bytes_read = recv(pconn->socket, (char*)pconn->read_buffer->start+pconn->recv_offset, size_to_read, 0);
                    if (bytes_read > 0)
                    {
                        pconn->recv_offset += bytes_read;
                        if ((pconn->recv_offset == pconn->recv_size) && (pconn->recv_size == MIN_RECEIVE_SIZE))
                        {
                            /* Determine the full message size */
                            int full_size = PEEK_LENGTH(pconn->read_buffer->start);

                            if (full_size > MIN_RECEIVE_SIZE)
                            {
                                SLPBuffer new_buffer = SLPBufferAlloc(full_size);
                                if (new_buffer == 0)
                                {
                                    pconn->state = CONN_FAILED;
                                    result = SLP_MEMORY_ALLOC_FAILED;
                                    goto FINISHED;
                                }
                                memcpy(new_buffer->start, pconn->read_buffer->start, pconn->recv_size);
                                SLPFree(pconn->read_buffer);
                                pconn->read_buffer = new_buffer;
                                pconn->recv_size = full_size;
                            }
                        }
                        if (pconn->recv_offset == pconn->recv_size)
                        {
                            /* Complete message read successfully */
                            if(AS_UINT16((const char *)pconn->read_buffer->start+10) == xid)
                            {
                                ++rplycount;
                                pconn->read_buffer->curpos = pconn->read_buffer->end;
                                pconn->state = CONN_COMPLETE;

                                /* Call the callback with the result and the receive buffer */
                                if(callback(result,&destaddr[i],pconn->read_buffer,cookie) == SLP_FALSE)
                                {
                                    /* Caller does not want any more info */
                                    /* We are done!                       */
                                    goto CLEANUP;
                                }
                            }
                        }
                    }
                }
                else
                {
                    /* Should never get here */
                    pconn->state = CONN_FAILED;
                }
            }
            else if (udp_active && (pconn->state == CONN_UDP) && (memcmp(&peeraddr.sin_addr, &destaddr[i].sin_addr, sizeof peeraddr.sin_addr) == 0))
            {
                /*---------------------------------------*/
                /* Handle received UDP data              */
                /*---------------------------------------*/
                --selected;
                if (result == SLP_ERROR_RETRY_UNICAST)
                {
                    result = SLP_OK;
                    pconn->socket = socket(AF_INET, SOCK_STREAM, 0);
                    if (pconn->socket < 0)
                    {
                        result = SLP_NETWORK_ERROR;
                        pconn->state = CONN_FAILED;
                    }
                    else
                    {
                        if (SetNonBlocking((int)pconn->socket) < 0)
                        {
                            result = SLP_NETWORK_ERROR;
                            closesocket(pconn->socket);
                            pconn->socket = (sockfd_t)-1;
                            pconn->state = CONN_FAILED;
                        }
                    }
                    if (result == SLP_OK)
                    {
                        pconn->read_buffer = SLPBufferAlloc(pconn->recv_size);
                        if (pconn->read_buffer == 0)
                        {
                            result = SLP_MEMORY_ALLOC_FAILED;
                            pconn->state = CONN_FAILED;
                        }
                        else
                        {
                            /* We now have a non-blocking socket and a buffer to read into */
                            /* Initiate a connect to the destination                */
                            if (connect(pconn->socket, (struct sockaddr *)&destaddr[i], sizeof destaddr[i]) < 0)
                            {
#ifdef _WIN32
                                if (WSAGetLastError() != WSAEINPROGRESS && WSAGetLastError() != WSAEWOULDBLOCK)
#else
                                if ((errno != 0) && (errno != EINPROGRESS))
#endif
                                {
                                    /* Connect operation failed immediately ! */
                                    result = SLP_NETWORK_ERROR;
                                    pconn->state = CONN_FAILED;
                                }
                                else
                                    pconn->state = CONN_TCP_CONNECT;
                            }
                            else
                            {
                                /* Connection succeeded immediately */
                                pconn->state = CONN_TCP_SEND;
                            }
                        }
                    }
                }
                else
                {
                    /* UDP reply from this DA */
                    if(AS_UINT16((const char *)udp_recvbuf->start+10) == xid)
                    {
                        ++rplycount;
                        udp_recvbuf->curpos = udp_recvbuf->end;
                        pconn->state = CONN_COMPLETE;

                        /* Call the callback with the result and the receive buffer */
                        if(callback(result,&destaddr[i],udp_recvbuf,cookie) == SLP_FALSE)
                        {
                            /* Caller does not want any more info */
                            /* We are done!                       */
                            goto CLEANUP;
                        }
                    }
                }
            }
        }
    }

    FINISHED:

    /*-----------------------------------------------*/
    /* Mark any failed DAs as bad so we don't use    */
    /* them again                                    */
    /*-----------------------------------------------*/

    for (i = 0; i < ndests; i++)
    {
        struct _UcastConnection *pconn = &pconnections[i];
        if (pconn->state != CONN_COMPLETE)
        {
            /* this DA failed or timed out, so mark it as bad */
            KnownDABadDA(&destaddr[i].sin_addr);
        }
    }

    /*-----------------------------------------------*/
    /* Notify the last time callback that we're done */
    /*-----------------------------------------------*/

    if(rplycount)
    {
        result = SLP_LAST_CALL;
    }

    callback(result, NULL, NULL, cookie);

    if(result == SLP_LAST_CALL)
    {
        result = SLP_OK;
    }

    /*----------------*/
    /* Free resources */
    /*----------------*/
    CLEANUP:
    if (udp_socket >= 0)
    {
#ifdef _WIN32
        closesocket(udp_socket);
#else
        close(udp_socket);
#endif
    }
    if(pconnections)
    {
        for (i = 0; i < ndests; i++)
        {
            struct _UcastConnection *pconn = &pconnections[i];
            if (pconn->socket >= 0)
#ifdef _WIN32
                closesocket(pconn->socket);
#else
                close(pconn->socket);
#endif
            if (pconn->read_buffer)
                SLPBufferFree(pconn->read_buffer);
        }
        xfree(pconnections);
    }
    SLPBufferFree(sendbuf);
    SLPBufferFree(udp_recvbuf);

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
