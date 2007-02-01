/*-------------------------------------------------------------------------
 * Copyright (C) 2001 Novell, Inc
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
 *    Neither the name of Novell nor the names of its contributors 
 *    may be used to endorse or promote products derived from this 
 *    software without specific prior written permission.
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

/** Functions related to acquiring specific DHCP parameters.
 *
 * @file       slp_dhcp.c
 * @author     John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeDHCP
 */

#include "slp_types.h"
#include "slp_dhcp.h"
#include "slp_message.h"
#include "slp_xmalloc.h"
#include "slp_net.h"

/** UDP port numbers, server and client. */
#define IPPORT_BOOTPS      67
#define IPPORT_BOOTPC      68

/** BOOTP header op codes */
#define BOOTREQUEST        1
#define BOOTREPLY          2

/** BOOTP header field value maximums */
#define MAXHTYPES          7     /*!< Number of htypes defined */
#define MAXHADDRLEN        6     /*!< Max hw address length in bytes */
#define MAXSTRINGLEN       80    /*!< Max string length */

/** Some other useful constants */
#define MAX_MACADDR_SIZE   64    /*!< Max hardware address length */
#define MAX_DHCP_RETRIES   2     /*!< Max dhcp request retries */

/** Timeout values */
#define USECS_PER_MSEC     1000
#define MSECS_PER_SEC      1000
#define USECS_PER_SEC      (USECS_PER_MSEC * MSECS_PER_SEC)
#define INIT_TMOUT_USECS   (250 * USECS_PER_MSEC)

/** DHCP vendor area cookie values */
#define DHCP_COOKIE1       99
#define DHCP_COOKIE2       130
#define DHCP_COOKIE3       83
#define DHCP_COOKIE4       99

/** DHCP Message Types for TAG_DHCP_MSG_TYPE */
#define DHCP_MSG_DISCOVER  1
#define DHCP_MSG_OFFER     2
#define DHCP_MSG_REQUEST   3
#define DHCP_MSG_DECLINE   4
#define DHCP_MSG_ACK       5
#define DHCP_MSG_NAK       6
#define DHCP_MSG_RELEASE   7
#define DHCP_MSG_INFORM    8

/** Create a DHCP broadcast socket.
 *
 * Creates a socket and provides a broadcast addr to which DHCP requests
 * should be sent. Also binds the socket to the DHCP client port.
 *
 * @param[out] peeraddr - A pointer to receive the address to which 
 *    DHCP requests are to be sent.
 *
 * @return A valid socket, or -1 if no DA connection can be made.
 * 
 * @internal
 */
static sockfd_t dhcpCreateBCSkt(void * peeraddr) 
{
   sockfd_t sockfd;
   so_bool_t on = 1;

   /* setup dhcp broadcast-to-server address structure */
   if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) != SLP_INVALID_SOCKET)
   {
      int addr = INADDR_ANY;
      struct sockaddr_storage localaddr;

      SLPNetSetAddr(&localaddr, AF_INET, IPPORT_BOOTPC, &addr);
      if (bind(sockfd, (struct sockaddr *)&localaddr, sizeof(localaddr))
            || setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,
                  (char *)&on, sizeof(on)))
      {
         closesocket(sockfd);
         return SLP_INVALID_SOCKET;
      }
      addr = INADDR_BROADCAST;
      SLPNetSetAddr(peeraddr, AF_INET, IPPORT_BOOTPS, &addr);
   }
   return sockfd;
}

/** Sends a DHCP request.
 *
 * Sends a DHCP request buffer to a specified DHCP server. Times out after 
 * a specified period.
 *
 * @param[in] sockfd - The socket on which to send.
 * @param[in] buf - The buffer to send.
 * @param[in] bufsz - The size of @p buf.
 * @param[in] addr - The target address.
 * @param[in] addrsz - The size of @p addr in bytes.
 * @param[in] timeout - The desired timeout value.
 *
 * @return Zero on success; non-zero on failure, and errno is set to 
 *    either EPIPE (for write error), or ETIMEDOUT (on timeout).
 * 
 * @internal
 */ 
static int dhcpSendRequest(sockfd_t sockfd, void * buf, size_t bufsz,
      void * addr, size_t addrsz, struct timeval * timeout)
{
   fd_set writefds;
   int xferbytes;
   int flags = 0;

#if defined(MSG_NOSIGNAL)
   flags = MSG_NOSIGNAL;
#endif

   FD_ZERO(&writefds);
   FD_SET(sockfd, &writefds);

   if ((xferbytes = select((int)sockfd + 1, 0, &writefds, 0, timeout)) > 0)
   {
      if ((xferbytes = sendto(sockfd, (char *)buf, (int)bufsz, flags, 
            addr, (int)addrsz)) <= 0)
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
   return 0;
}

/** Receives a DHCP response.
 *
 * Receives a DHCP response from a DHCP server. Since DHCP responses are
 * broadcasts, we compare XID with received response to ensure we are
 * returning the correct response from a prior request.
 *
 * @param[in] sockfd - The socket from which to receive.
 * @param[out] buf - The address of storage for the received packet.
 * @param[in] bufsz - The size of @p buf.
 * @param[in] timeout - The desired timeout value.
 *
 * @return Zero on success; non-zero on failure with errno set to
 *    ENOTCONN (socket not connected), or ETIMEDOUT (on timeout error).
 * 
 * @internal
 */
static int dhcpRecvResponse(sockfd_t sockfd, void * buf, size_t bufsz,
      struct timeval * timeout)
{
   int xferbytes;
   fd_set readfds;

   FD_ZERO(&readfds);
   FD_SET(sockfd, &readfds);

   if ((xferbytes = select((int)sockfd + 1, &readfds, 0 , 0, timeout)) > 0)
   {
      if ((xferbytes = recvfrom(sockfd, (char *)buf, (int)bufsz, 0, 0, 0)) <= 0)
      {
         errno = ENOTCONN;
         return -1;
      }
      return xferbytes;
   }
   else if (xferbytes == 0)
   {
      errno = ETIMEDOUT;
      return -1;
   }
   errno = ENOTCONN;
   return -1;
}

/** Calls a callback function once for each option returned by a DHCP server.
 *
 * @param[in] data - The DHCP response data buffer to parse.
 * @param[in] datasz - The length of @p data.
 * @param[in] dhcpInfoCB - The callback function to call for each option.
 * @param[in] context - Callback context data.
 *
 * @return Zero on success; non-zero on failure, with errno set to
 * ENOTCONN (error during read), ETIMEDOUT (read timed out), 
 * ENOMEM (out of memory error), or EINVAL (parse error).
 * 
 * @internal
 */
static int dhcpProcessOptions(unsigned char * data, size_t datasz,
      DHCPInfoCallBack * dhcpInfoCB, void * context)
{
   int err, taglen;
   unsigned char tag;

   /* validate vendor data header */
   if (datasz < 4 
         || *data++ != DHCP_COOKIE1 || *data++ != DHCP_COOKIE2 
         || *data++ != DHCP_COOKIE3 || *data++ != DHCP_COOKIE4) 
      return -1;        /* invalid dhcp response */

   datasz -= 4;         /* account for DHCP cookie values */

   /* validate and process each tag in the vendor data */
   while (datasz-- > 0 && (tag = *data++) != TAG_END) 
   {
      if (tag != TAG_PAD) 
      {
         if (!datasz-- || (taglen = *data++) > (int)datasz) 
            return -1;  /* tag length greater than total data length */

         if ((err = dhcpInfoCB(tag, data, taglen, context)) != 0)
            return err;

         datasz -= taglen;
         data += taglen;
      }
   }
   return 0;   
}

/** Determines the hardware MAC address for the specified IP address.
 *
 * @param[in] ipaddr - The IP address for which to determine MAC address.
 * @param[out] chaddr - The MAC address found for @p ipaddr.
 * @param[out] hlen - The length of the value returned in @p chaddr.
 * @param[out] htype - The address type returned in @p chaddr.
 *
 * @return Zero on success; non-zero on failure.
 * 
 * @internal
 */
static int dhcpGetAddressInfo(unsigned char * ipaddr, unsigned char * chaddr, 
      unsigned char * hlen, unsigned char * htype)
{
#ifdef _WIN32

   HMODULE hmod;  
   DWORD (WINAPI * pGetAdaptersInfo)(PIP_ADAPTER_INFO pAdapterInfo, 
         PULONG pOutBufLen);
   char ipastr[16];

   *hlen = 0;

   sprintf(ipastr, "%d.%d.%d.%d", ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
   
   if ((hmod = LoadLibrary("iphlpapi.dll")) != 0)
   {
      if ((pGetAdaptersInfo = (DWORD(WINAPI *)(PIP_ADAPTER_INFO, PULONG))
            GetProcAddress(hmod, "GetAdaptersInfo")) != 0)
      {
         DWORD dwerr;
         ULONG bufsz = 0;
         IP_ADAPTER_INFO *aip = 0;
         if ((dwerr = (*pGetAdaptersInfo)(aip, &bufsz)) == ERROR_BUFFER_OVERFLOW
               && (aip = (IP_ADAPTER_INFO *)xmalloc(bufsz)) != 0
               && (dwerr = (*pGetAdaptersInfo)(aip, &bufsz)) == ERROR_SUCCESS)
         {
            IP_ADAPTER_INFO * pcur;
            for (pcur = aip; pcur && !*hlen; pcur = pcur->Next)
            {
               IP_ADDR_STRING * caddrp;
               for (caddrp = &pcur->IpAddressList; 
                     caddrp && !*hlen; caddrp = caddrp->Next)
               {
                  if (strcmp(ipastr, caddrp->IpAddress.String) == 0)
                  {
                     *hlen = (uint8_t)pcur->AddressLength;
                     *htype = (uint8_t)pcur->Type; /* win32 returns iana ARP values */
                     memcpy(chaddr, pcur->Address, pcur->AddressLength);
                     break;
                  }
               }
            }
            if (!*hlen) /* couldn't find the one we wanted, just use the first */
            {
               *hlen = (uint8_t)aip->AddressLength;
               *htype = (uint8_t)aip->Type;
               memcpy(chaddr, aip->Address, aip->AddressLength);
            }
         }
         xfree(aip);
      }
      FreeLibrary(hmod);
   }

#elif defined(SIOCGARP)

   /* Query the ARP cache for our hardware address */
   int sockfd;
   struct arpreq arpreq;
   struct sockaddr_in * sin;

   if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == SLP_INVALID_SOCKET)
      return -1;

   *hlen = 0;
   
   sin = (struct sockaddr_in *)&arpreq.arp_pa;
   memset(sin, 0, sizeof(struct sockaddr_in));
   sin->sin_family = AF_INET;
   memcpy(&sin->sin_addr, ipaddr, sizeof(struct in_addr));
   
	/* We should check for an error from ioctl, but this call succeeds 
		on SuSE linux 10.2, but returns an error code (19 - no such 
		device). For now, we'll just say that the check for data length 
		below is sufficient for our needs. */

	ioctl(sockfd, SIOCGARP, &arpreq);
   *hlen = 6;  /* assume IEEE802 compatible */
   *htype = arpreq.arp_ha.sa_family;
   memcpy(chaddr, arpreq.arp_ha.sa_data, 6);
   closesocket(sockfd);

#else

   /* figure out another way ... */
   (void)ipaddr;
   (void)chaddr;
   (void)htype;

   *hlen = 0;

#endif

   return *hlen? 0: -1;
}

/** Queries a DHCP server and calls a callback once for each option.
 *
 * Queries the sub-net DHCP server for a specified set of DHCP options
 * and then calls a user-supplied callback function once for each of 
 * the desired options returned by the server.
 *
 * @param[in] dhcpOptCodes - An array of option codes to query for.
 * @param[in] dhcpOptCodeCnt - The number of elements in @p dhcpOptCodes.
 * @param[in] dhcpInfoCB - The callback function to call for each option.
 * @param[in] context - callback context.
 *
 * @return Zero on success; non-zero on failure, with errno set to 
 *    ENOTCONN (read error), ETIMEDOUT (read timeout), ENOMEM (out of
 *    memory), or EINVAL (on parse error).
 */
int DHCPGetOptionInfo(unsigned char * dhcpOptCodes, int dhcpOptCodeCnt, 
      DHCPInfoCallBack * dhcpInfoCB, void * context)
{
   uint32_t xid;
   time_t timer;
   struct timeval tv;
   sockfd_t sockfd; 
   int retries;
   struct sockaddr_storage sendaddr;
   unsigned char chaddr[MAX_MACADDR_SIZE];
   unsigned char hlen, htype;
   uint8_t sndbuf[512];
   uint8_t rcvbuf[512];
   struct hostent * hep;
   uint8_t * p;
   int rcvbufsz = 0;
   char host[256];

   /* Get our IP and MAC addresses */
   if (gethostname(host, (int)sizeof(host))
         || (hep = gethostbyname(host)) == 0
         || dhcpGetAddressInfo((unsigned char *)hep->h_addr, 
               chaddr, &hlen, &htype))
      return -1;

   /* get a reasonably random transaction id value */
   xid = (uint32_t)time(&timer);

   /* BOOTP request header */
   p = sndbuf;
   *p++ = BOOTREQUEST;        /* opcode */
   *p++ = htype;
   *p++ = hlen;
   *p++ = 0;                  /* hops */
   PutUINT32(&p, xid);
   PutUINT16(&p, 0);          /* seconds */
   PutUINT16(&p, 0);          /* flags */
   memcpy(p, hep->h_addr, 4); /* ciaddr */
   p += 4;
   PutUINT32(&p, 0);          /* yiaddr */
   PutUINT32(&p, 0);          /* siaddr */
   PutUINT32(&p, 0);          /* giaddr */
   memcpy(p, chaddr, hlen);   /* chaddr */
   p += hlen;
   memset(p, 0, 16-hlen);     /* remaining chaddr space */
   p += 16-hlen;
   memset(p, 0, 64 + 128);    /* server host name, boot file */
   p += 64 + 128;

   /* BOOTP options field */
   *p++ = DHCP_COOKIE1;       /* options, cookies 1-4 */
   *p++ = DHCP_COOKIE2;
   *p++ = DHCP_COOKIE3;
   *p++ = DHCP_COOKIE4;

   /* DHCP Message Type option */
   *p++ = TAG_DHCP_MSG_TYPE;
   *p++ = 1;                  /* option length */
   *p++ = DHCP_MSG_INFORM;    /* message type is DHCPINFORM */

   /* DHCP Parameter Request option */
   *p++ = TAG_DHCP_PARAM_REQ; /* request for DHCP parms */
   *p++ = (unsigned char)dhcpOptCodeCnt;
   memcpy(p, dhcpOptCodes, dhcpOptCodeCnt);
   p += dhcpOptCodeCnt;

   /* DHCP Client Identifier option */
   *p++ = TAG_CLIENT_IDENTIFIER;
   *p++ = hlen + 1;           /* option length */
   *p++ = htype;              /* client id is htype/haddr */
   memcpy(p, chaddr, hlen);
   p += hlen;

   /* End option */
   *p++ = TAG_END;

   /* get a broadcast send/recv socket and address */
   if ((sockfd = dhcpCreateBCSkt(&sendaddr)) == SLP_INVALID_SOCKET)
      return -1;

   /* setup select timeout */
   tv.tv_sec = 0;
   tv.tv_usec = INIT_TMOUT_USECS;

   retries = 0;
   srand((unsigned)time(&timer));
   while (retries++ < MAX_DHCP_RETRIES)
   {
      if (dhcpSendRequest(sockfd, sndbuf, p - sndbuf, 
            &sendaddr, sizeof(sendaddr), &tv) < 0)
      {
         if (errno != ETIMEDOUT)
         {
            closesocket(sockfd);
            return -1;
         }
      }
      else if ((rcvbufsz = dhcpRecvResponse(sockfd, rcvbuf, 
            sizeof(rcvbuf), &tv)) < 0)
      {
         if (errno != ETIMEDOUT)
         {
            closesocket(sockfd);
            return -1;
         }
      }
      else if (rcvbufsz >= 236 && AS_UINT32(&rcvbuf[4]) == xid)
         break;

      /* exponential backoff randomized by a 
       * uniform number between -1 and 1 
      */
      tv.tv_usec = tv.tv_usec * 2 + (rand() % 3) - 1;
      tv.tv_sec = tv.tv_usec / USECS_PER_SEC;
      tv.tv_usec %= USECS_PER_SEC;
   }
   closesocket(sockfd);
   return rcvbufsz? dhcpProcessOptions(rcvbuf + 236, rcvbufsz - 236, 
         dhcpInfoCB, context): -1;
}

/** A callback routine that tests each DA discovered from DHCP.
 *
 * Each DA that passes is added to the DA cache.
 *
 * @param[in] tag - The DHCP option tag for this call.
 * @param[in] optdata - A pointer to the DHCP option data for this call.
 * @param[in] optdatasz - The size of @p optdata.
 * @param[in] context - Callback context data. For this callback routine
 *    this parameter is actually a pointer to DHCPContext data.
 *
 * @return Zero on success, or a non-zero value to stop the caller from 
 *    continuing to parse the buffer and call this routine.
 */
int DHCPParseSLPTags(int tag, void * optdata, size_t optdatasz, 
      void * context)
{
   size_t cpysz, bufsz, dasize;
   DHCPContext * ctxp = (DHCPContext *)context;
   unsigned char * p = (unsigned char *)optdata;
   unsigned char flags;
   int encoding;

   /* filter out zero length options */
   if (!optdatasz)
      return 0;

   switch(tag)
   {
      case TAG_SLP_SCOPE:

         /* Draft 3 format is only supported for ASCII and UNICODE
         *  character encodings - UTF8 encodings must use rfc2610 format.
         *  To determine the format, we parse 2 bytes and see if the result
         *  is a valid encoding. If so it's draft 3, otherwise rfc2610. 
         */
         encoding = (optdatasz > 1)? AS_UINT16(p): 0;
         if (encoding != CT_ASCII && encoding != CT_UNICODE)
         {
            /* rfc2610 format */

            if (optdatasz == 1)
               break;      /* UA's should ignore statically configured
                           *  scopes for this interface - 
                           *  add code to handle this later... 
                           */

            flags = *p++;  /* pick up the mandatory flag... */
            optdatasz--;

            if (flags)
               ;           /* ...and add code to handle it later... */

            /* copy utf8 string into return buffer */
            ctxp->scopelistlen = optdatasz < sizeof(ctxp->scopelist)? 
                  optdatasz: sizeof(ctxp->scopelist);
            strncpy(ctxp->scopelist, (char*)p, ctxp->scopelistlen);
         }
         else
         {
            /* draft 3 format: defined to configure scopes for SA's only
            *  so we should flag the scopes to be used only as registration
            *  filter scopes - add code to handle this case later...
            *
            *  offs     len      name        description
            *  0        2        encoding    character encoding used 
            *  2        n        scopelist   list of scopes as asciiz string.
            *
            */
            optdatasz -= 2;   /* skip encoding bytes */
            p += 2;

            /* if UNICODE encoding is used convert to utf8 */
            if (encoding == CT_UNICODE)
               wcstombs(ctxp->scopelist, (wchar_t*)p, sizeof(ctxp->scopelist));
            else
            {
               ctxp->scopelistlen = optdatasz < sizeof(ctxp->scopelist)? 
                     optdatasz: sizeof(ctxp->scopelist);
               strncpy(ctxp->scopelist, (char*)p, ctxp->scopelistlen);
            }
         }
         break;

      case TAG_SLP_DA:
         flags = *p++;
         optdatasz--;

         /* If the flags byte has the high bit set, we know we are
         *  using draft 3 format, otherwise rfc2610 format. 
         */
         if (!(flags & DA_NAME_PRESENT))
         {
            /* rfc2610 format */
            if (flags)
            {
               /* If the mandatory byte is non-zero, indicate that 
               *  multicast is not to be used to dynamically discover 
               *  directory agents on this interface by setting the 
               *  LACBF_STATIC_DA flag in the LACB for this interface. 
               */

               /* skip this for now - deal with it later... */
            }

            bufsz = sizeof(ctxp->addrlist) - ctxp->addrlistlen;
            cpysz = optdatasz < bufsz? optdatasz: bufsz;
            memcpy(ctxp->addrlist + ctxp->addrlistlen, p, cpysz);
            ctxp->addrlistlen += cpysz;
         }
         else
         {
            /* pre-rfc2610 (draft 3) format:
            *     offs     len      name     description
            *     0        1        flags    contains 4 flags (defined above)
            *     1        1        dasize   name or addr length
            *     2        dasize   daname   da name or ip address (flags)
            */
            dasize = *p++;
            optdatasz--;

            if (dasize > optdatasz)
               dasize = optdatasz;
            if (flags & DA_NAME_IS_DNS)
               ;  /* DA name contains dns name - we have to resolve - later... */
            else
            {     /* DA name is one 4-byte ip address */
               if (dasize < 4)
                  break;   /* oops, bad option format */
               dasize = 4;
               bufsz = sizeof(ctxp->addrlist) - ctxp->addrlistlen;
               cpysz = dasize < bufsz? dasize: bufsz;
               memcpy(ctxp->addrlist + ctxp->addrlistlen, p, cpysz);
               ctxp->addrlistlen += cpysz;
            }
            if (flags & DISABLE_DA_MCAST)
               ;  /* this is the equivalent of the rfc2610 mandatory bit */
         }
         break;
   }
   return 0;
}

/*===========================================================================
 *  TESTING CODE : compile with the following command lines:
 *
 *  $ gcc -g -DSLP_DHCP_TEST -DDEBUG -DHAVE_CONFIG_H -I <path to config.h>
 *			slp_dhcp.c slp_net.c slp_xmalloc.c slp_property.c slp_linkedlist.c 
 *			slp_thread.c slp_debug.c slp_message.c slp_v2message.c 
 *			slp_v1message.c slp_utf8.c -lpthread
 *
 *  C:\> cl -Zi -DSLP_DHCP_TEST -DDEBUG -D_CRT_SECURE_NO_DEPRECATE 
 *       -DSLP_VERSION=\"2.0\" slp_dhcp.c slp_net.c slp_xmalloc.c 
 *       slp_property.c slp_linkedlist.c slp_thread.c slp_debug.c 
 *       slp_win32.c slp_message.c slp_v2message.c ws2_32.lib
 */
#ifdef SLP_DHCP_TEST 

# define FAIL (printf("FAIL: %s at line %d.\n", __FILE__, __LINE__), (-1))
# define PASS (printf("PASS: Success!\n"), (0))

int main(int argc, char * argv[])
{
   int err;
   DHCPContext ctx;
   unsigned char * alp, * ale;
   unsigned char dhcpOpts[] = {TAG_SLP_SCOPE, TAG_SLP_DA};

#ifdef _WIN32
   {
      WSADATA wsaData;
      if ((err = WSAStartup(MAKEWORD(2,2), &wsaData)) != 0)
         return FAIL;
   }
#endif

   *ctx.scopelist = 0;
   ctx.scopelistlen = 0;
   ctx.addrlistlen = 0;

   if ((err = DHCPGetOptionInfo(dhcpOpts, sizeof(dhcpOpts), 
         DHCPParseSLPTags, &ctx)) != 0)
      return FAIL;

   printf("ScopeList: [%.*s]\n", ctx.scopelistlen, ctx.scopelist);
   printf("AddrList: [");

   alp = ctx.addrlist;
   ale = ctx.addrlist + ctx.addrlistlen;
   while(alp + 3 <= ale)
   {
      printf("%u.%u.%u.%u", alp[0], alp[1], alp[2], alp[3]);
      alp += 4;
      if (alp + 3 <= ale)
         printf(", ");
   }
   printf("]\n");

#ifdef _WIN32
   WSACleanup();
#endif

   return PASS;
}

#endif

/*=========================================================================*/
