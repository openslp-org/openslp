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

/** Network utility functions.
 *
 * These routines manage network address data structures, and provide host-
 * to-address conversion functionality.
 *
 * @file       slp_net.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#include "../libslp/slp.h"

#include "slp_net.h"
#include "slp_xmalloc.h"
#include "slp_property.h"

#include <assert.h>

#define slp_min(a,b) (((a)<(b))?(a):(b))

/* IPv6 SLP address constants */
const struct in6_addr in6addr_srvloc_node =
{
   {{ 0xFF,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x01,0x16 }}
};
const struct in6_addr in6addr_srvloc_link =
{
   {{ 0xFF,0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x01,0x16 }}
};
const struct in6_addr in6addr_srvloc_site =
{
   {{ 0xFF,0x5,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x01,0x16 }}
};
const struct in6_addr in6addr_srvlocda_node =
{
   {{ 0xFF,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x01,0x23 }}
};
const struct in6_addr in6addr_srvlocda_link =
{
   {{ 0xFF,0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x01,0x23 }}
};
const struct in6_addr in6addr_srvlocda_site =
{
   {{ 0xFF,0x5,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x01,0x23 }}
};
const struct in6_addr in6addr_service_node_mask =
{
   {{ 0xFF,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x10,0x00 }}
};
const struct in6_addr in6addr_service_link_mask =
{
   {{ 0xFF,0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x10,0x00 }}
};
const struct in6_addr in6addr_service_site_mask =
{
   {{ 0xFF,0x5,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x10,0x00 }}
};
const struct in6_addr slp_in6addr_any = SLP_IN6ADDR_ANY_INIT;
const struct in6_addr slp_in6addr_loopback = SLP_IN6ADDR_LOOPBACK_INIT;

/** Resolve a host name to an address.
 *
 * @param[in] af - The address family to resolve to.
 * @param[in] src - The host name to be resolved.
 * @param[out] dst - The address of storage for the resolved binary address.
 *
 * @return 1 if successful, 0 on failure, and -1 if @p af is unknown.
 *
 * @note The buffer pointed to by @p dst must be at least large enough to 
 *    hold an address of the family specified by @p af.
 *
 * @internal
 */
static int resolveHost(int af, const char * src, void * dst)
{
   struct addrinfo * res;
   struct addrinfo hints;

   memset(&hints, 0, sizeof(hints));

   if (af == AF_INET)
   {
      struct in_addr * d4Dst = (struct in_addr *) dst;
      hints.ai_family = PF_INET;
      if (getaddrinfo(src, 0, &hints, &res) == 0)
      {
         struct addrinfo * aicheck = res;
         while (aicheck != 0)
         {
            if (aicheck->ai_addr->sa_family == af)
            {
               struct in_addr * d4Src;
               d4Src = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
               memcpy(&d4Dst->s_addr, &d4Src->s_addr, 4);
               return 1;
            }
            else
               aicheck = aicheck->ai_next;
         }
         /* if aicheck was NULL, sts will still be 0, if not, sts will be 1 */
      }
      return 0;
   }
   if (af == AF_INET6)
   {
      struct in6_addr * d6Dst = (struct in6_addr *) dst;
      hints.ai_family = PF_INET6;
      if (getaddrinfo(src, 0, &hints, &res) == 0)
      {
         struct addrinfo * aicheck = res;
         while (aicheck != 0)
         {
            if (aicheck->ai_addr->sa_family == af)
            {
               struct in6_addr * d6Src;
               d6Src = &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr;
               memcpy(&d6Dst->s6_addr, &d6Src->s6_addr, 16); 
               return 1;
            }
            else
               aicheck = aicheck->ai_next;
         }
      }
      return 0;
   }
   return -1;
}

/** Determines if the specified IPv6 address is the IPv6 loopback address.
 *
 * @param[in] a - The IPv6 address to be checked.
 *
 * @return A boolean value; True (1) if @p a is the IPv6 loopback 
 *    address, or False (0) if not.
 *
 * @internal
 */
static int SLP_IN6_IS_ADDR_LOOPBACK(const struct in6_addr * a)
{
   return memcmp(a, &slp_in6addr_loopback, sizeof(struct in6_addr)) == 0;
}

/** Formats an addrinfo structure as a displayable string.
 *
 * @param[in] src - The addrinfo structure to be foramtted.
 * @param[out] dst - The address of storage for the formatted string.
 * @param[in] dstLen - The size of @p dst in bytes.
 *
 * @return Zero on success, or a non-zero value on failure.
 *
 * @internal
 */
static int SLPNetAddrInfoToString(const struct addrinfo * src, 
      char * dst, size_t dstLen)
{
   if (src->ai_family == AF_INET)
   {
      struct sockaddr * addr = (struct sockaddr *)src->ai_addr;
      inet_ntop(src->ai_family, &addr->sa_data[2], dst, dstLen);
   }
   else if (src->ai_family == AF_INET6)
   {
      struct sockaddr_in6 * addr = (struct sockaddr_in6 *)src->ai_addr;
      inet_ntop(src->ai_family, &addr->sin6_addr, dst, dstLen);
   }
   else
      return -1;

   return 0;
}

/** Format that portion of an IPv6 shorthand address before the '::'.
 *
 * @param[in] start - Points to the start of a formatted IPv6 address
 *    thats in shorthand notation (eg., xxxx: ... :: ... :xxxx). 
 * @param[out] result - Points to the output buffer where the address
 *    in @p start is to be expanded.
 *
 * @return Zero.
 *
 * @remarks Copies character data from @p start into the pre-formatted 
 *    template in @p result till it comes to a double colon.
 *
 * @internal
 */
static int handlePreArea(const char * start, char * result)
{
   const char * slider1;
   const char * slider2;
   const char * end;

   end = strstr(start, "::");
   if (end == 0)
   {
      end = strchr(start, ',');
      if (end == 0)
         end = start + strlen(start);
   }
   if ((start == 0) || end == 0)
      return 0;

   slider1 = start;
   while (slider1 < end)
   {
      slider2 = strchr(slider1, ':');
      if (slider2)
      {
         strncpy(result + (4 - (slider2 - slider1)), slider1,
               slider2 - slider1);
         result += 5;
         slider1 = slider2 + 1;
      }
      else
      {
         strncpy(result + (4 - (end - slider1)), slider1, end - slider1);
         break;
      }
   }
   return 0;
}

/** Format that portion of an IPv6 shorthand address after the '::'.
 *
 * @param[in] start - Points to the second colon in the double-colon 
 *    of a shorthand-formatted IPv6 address.
 *    (eg., xxxx: ... :: ... :xxxx). 
 * @param[out] result - Points to the output buffer where the address
 *    is to be expanded.
 *
 * @return Zero.
 *
 * @remarks Copies character data backward from @p start into the 
 *    pre-formatted template in @p result till it returns to the first 
 *    character in @p start - the second colon in the double-colon mark.
 *
 * @internal
 */
static int handlePostArea(char * start, char * result)
{
   char * slider1;
   char * slider2;
   char ourCopy[256];

   if ((start == 0) || result == 0)
      return 0;
   strncpy(ourCopy, start, slp_min(strlen(start) + 1, sizeof(ourCopy)));
   result += strlen(result); /* we will work from the back */
   slider1 = ourCopy + strlen(ourCopy);
   while (slider1 > ourCopy)
   {
      slider2 = strrchr(ourCopy, ':');  /* find the last : */
      if ((slider2) && (slider2 < (slider1 - 1)))
      {
         slider2++; /* get past the colon */
         *(slider2 - 1) = '\0'; /* set the colon to null - for the next strrchr */
         strncpy(result - (slider1 - slider2), slider2, (slider1 - slider2));
         result -= 5;
         slider1 = slider2 - 1;
      }
      else
         break;
   }
   return 0;
}

/** Returns an address for the specified host name.
 *
 * @param[in] host - A pointer to hostname to resolve.
 * @param[out] addr - The address of storage for the returned address 
 *    data.
 *
 * @return Zero on success, or non-zero on error.
 *
 * @remarks This routine returns address data of unspecified family, so 
 *    the output buffer is purposely defined as a sockaddr_storage buffer.
 */
int SLPNetResolveHostToAddr(const char * host, 
      struct sockaddr_storage * addr)
{
   struct sockaddr_in6 * a6 = (struct sockaddr_in6 *)addr;
   struct sockaddr_in  * a4 = (struct sockaddr_in  *)addr;

   /* Quick check for dotted quad IPv4 address. */
   if (resolveHost(AF_INET, host, &a4->sin_addr) == 1)
   {
      addr->ss_family = AF_INET;
      return 0;
   }

   /* Try an IPv6 address. */
   if (resolveHost(AF_INET6, host, &a6->sin6_addr) == 1)
   {
      addr->ss_family = AF_INET6;
      return 0;
   }
   return -1;
}

/** Determine if we should use IPv6.
 *
 * @return A boolean value; True if IPv6 should be used, False if not.
 *
 * @todo Change this routine to get v6 net info, not property info.
 */
int SLPNetIsIPV6(void)
{
   return SLPPropertyAsBoolean(SLPPropertyGet("net.slp.useIPV6"))? 1: 0;
}

/** Determine if we should use IPv4.
 *
 * @return A boolean value; True if IPv4 should be used, False if not.
 *
 * @todo Change this routine to get v4 net info, not property info.
 */
int SLPNetIsIPV4(void)
{
   return SLPPropertyAsBoolean(SLPPropertyGet("net.slp.useIPV4"))? 1: 0;
}

/** Compare two address buffers.
 *
 * @param[in] addr1 - The first address to compare.
 * @param[in] addr2 - The second address to compare.
 *
 * @return Zero if @p addr1 is the same as @p addr2. Non-zero if not.
 *
 * @note The sizes of @p addr1 and @p addr2 must both be at least
 *    sizeof(struct sockaddr_storage) if the address families are not
 *    one of AF_INET or AF_INET6.
 */
int SLPNetCompareAddrs(const void * addr1, const void * addr2)
{
   const struct sockaddr * a1 = (const struct sockaddr *)addr1;
   const struct sockaddr * a2 = (const struct sockaddr *)addr2;

   if (a1->sa_family == a2->sa_family)
   {
      if (a1->sa_family == AF_INET)
      {
         struct sockaddr_in * v41 = (struct sockaddr_in *)addr1;
         struct sockaddr_in * v42 = (struct sockaddr_in *)addr2;
         if (v41->sin_family == v42->sin_family)
            return memcmp(&v41->sin_addr, &v42->sin_addr,
                        sizeof(v41->sin_addr));
      }
      else if (a1->sa_family == AF_INET6)
      {
         struct sockaddr_in6 * v61 = (struct sockaddr_in6 *)addr1;
         struct sockaddr_in6 * v62 = (struct sockaddr_in6 *)addr2;
         if (v61->sin6_family == v62->sin6_family)
            return memcmp(&v61->sin6_addr, &v62->sin6_addr,
                        sizeof(v61->sin6_addr));
      }
      else  /* don't know how to decode - use memcmp for now */
         return memcmp(addr1, addr2, sizeof(struct sockaddr_storage));
   }
   return -1;
}

/** Determines if the specified address is a multi-cast address.
 *
 * @param[in] addr - The address to be checked.
 *
 * @return A boolean value; True (1) if @p addr is a multi-cast address,
 *    or False (0) if not.
 */
int SLPNetIsMCast(const void * addr)
{
   const struct sockaddr * a = (const struct sockaddr *)addr;

   if (a->sa_family == AF_INET)
   {
      struct sockaddr_in * v4 = (struct sockaddr_in *)addr;
      if ((ntohl(v4->sin_addr.s_addr) & 0xff000000) >= 0xef000000)
         return 1;
      return 0;
   }
   if (a->sa_family == AF_INET6)
   {
      struct sockaddr_in6 * v6 = (struct sockaddr_in6 *)addr;
      return IN6_IS_ADDR_MULTICAST(&v6->sin6_addr);
   }
   return 0;
}

/** Determines if the specified address is on the local host.
 *
 * @param[in] addr - The address to be checked.
 *
 * @return A boolean value; True (1) if @p addr is a local host
 *    address, or False (0) if not.
 */
int SLPNetIsLocal(const void * addr)
{
   const struct sockaddr * a = (const struct sockaddr *)addr;

   if (a->sa_family == AF_INET)
   {
      struct sockaddr_in * v4 = (struct sockaddr_in *)addr;
      if ((ntohl(v4->sin_addr.s_addr) & 0xff000000) == 0x7f000000)
         return 1;
      return 0;
   }
   if (a->sa_family == AF_INET6)
   {
      struct sockaddr_in6 * v6 = (struct sockaddr_in6 *)addr;
      return IN6_IS_ADDR_LOOPBACK(&v6->sin6_addr);
   }
   return 0;
}

/** Determines if the specified address is a loopback address.
 *
 * @param[in] addr - The address to be checked.
 *
 * @return A boolean value; True (1) if @p addr is a local loopback
 *    address, False (0) if not.
 *
 * @remarks This version works on either IPv4 or IPv6.
 */
int SLPNetIsLoopback(const void * addr)
{
   const struct sockaddr * a = (const struct sockaddr *)addr;

   if (a->sa_family == AF_INET)
   {
      struct sockaddr_in * v4 = (struct sockaddr_in *)addr;
      if ((ntohl(v4->sin_addr.s_addr) == INADDR_LOOPBACK))
         return 1;
      return 0;
   }
   if (a->sa_family == AF_INET6)
   {
      struct sockaddr_in6 * v6 = (struct sockaddr_in6 *)addr;
      return SLP_IN6_IS_ADDR_LOOPBACK(&v6->sin6_addr);
   }
   return 0;
}

/** Fills in the fields of an address structure.
 *
 * @param[out] addr - The address structure to be filled.
 * @param[in] family - The address family value to set.
 * @param[in] port - The port number to set.
 * @param[in] address - The address data to set.
 *
 * @return Zero on success, or a non-zero value on failure.
 *
 * @note The size of @p addr is determined by @p family, and 
 *    both address buffers must be large enough to accommodate
 *    the address family specified in @p family.
 */
int SLPNetSetAddr(void * addr, int family,
      uint16_t port, const void * address)
{
   if (family == AF_INET)
   {
      struct sockaddr_in * v4 = (struct sockaddr_in *)addr;
      v4->sin_family = (short)family;
      v4->sin_port = htons(port);
      if (address == 0)
         v4->sin_addr.s_addr = INADDR_ANY;
      else
         v4->sin_addr.s_addr = htonl(*((int *)address));
   }
   else if (family == AF_INET6)
   {
      struct sockaddr_in6 * v6 = (struct sockaddr_in6 *)addr;
      v6->sin6_family = (short)family;
      v6->sin6_flowinfo = 0;
      v6->sin6_port = htons(port);
      v6->sin6_scope_id = 0;
      if (address == 0)
         memcpy(&v6->sin6_addr, &slp_in6addr_any, sizeof(struct in6_addr));
      else
         memcpy(&v6->sin6_addr, address, sizeof(v6->sin6_addr));
   }
   else
      return -1;
   return 0;
}

/** Sets the family and port of an address structure.
 *
 * @param[out] addr - The address structure to be set.
 * @param[in] family - The family value to set.
 * @param[in] port - The port value to set.
 *
 * @return Zero on success, or a non-zero value on failure.
 */
int SLPNetSetParams(void * addr, int family,
      uint16_t port)
{
   if (family == AF_INET)
   {
      struct sockaddr_in * v4 = (struct sockaddr_in *)addr;
      v4->sin_family = (short)family;
      v4->sin_port = htons(port);
      return 0;
   }
   if (family == AF_INET6)
   {
      struct sockaddr_in6 * v6 = (struct sockaddr_in6 *)addr;
      v6->sin6_family = (short)family;
      v6->sin6_flowinfo = 0;
      v6->sin6_port = htons(port);
      v6->sin6_scope_id = 0;
      return 0;
   }
   return -1;
}

/** Sets the port value of an address structure.
 *
 * @param[out] addr - The address structure whose port should be set.
 * @param[in] port - The port value to set in @p addr.
 *
 * @return Zero on success, or a non-zero value on failure.
 */
int SLPNetSetPort(void * addr, uint16_t port)
{
   const struct sockaddr * a = (const struct sockaddr *)addr;

   if (a->sa_family == AF_INET)
      ((struct sockaddr_in *)addr)->sin_port = htons(port);
   else if (a->sa_family == AF_INET6)
      ((struct sockaddr_in6 *)addr)->sin6_port = htons(port);
   else
      return -1;
   return 0;
}

/** Format an address structure as a displayable string.
 *     
 * @param[in] src - The source address to format.
 * @param[out] dst - The destination string buffer.
 * @param[in] dstLen - The size of @p dst in bytes.
 *
 * @return A pointer to @p dst on success, or zero on failure.
 *
 * @remarks The format for an IPv4 address is "x.x.x.x". The format for 
 *    an IPv6 address is "x:x:..:x".
 */
char * SLPNetSockAddrStorageToString(const void * src, 
      char * dst, size_t dstLen)
{
   const struct sockaddr * a = (const struct sockaddr *)src;

   if (a->sa_family == AF_INET)
   {
      struct sockaddr_in * v4 = (struct sockaddr_in *)src;
      inet_ntop(v4->sin_family, &v4->sin_addr, dst, dstLen);
   }
   else if (a->sa_family == AF_INET6)
   {
      struct sockaddr_in6 * v6 = (struct sockaddr_in6 *)src;
      inet_ntop(v6->sin6_family, &v6->sin6_addr, dst, dstLen);
   }
   else
      return 0;
   return dst;
}

/** Determines an IPv6 multi-cast address for a specified service type.
 *
 * The multi-cast address is determined from a hash value calculated on the
 * string name of the service type.
 *
 * @param[in] pSrvType - The service type string.
 * @param[in] len - The length of @p pSrvType in bytes.
 * @param[in] scope - The scope associated with this request.
 * @param[out] addr - The address buffer in which to return the address.
 *
 * @return Zero on success, or a non-zero value on failure.
 */
int SLPNetGetSrvMcastAddr(const char * pSrvType, size_t len,
      int scope, void * addr)
{
   struct sockaddr * a = (struct sockaddr *)addr;
   unsigned long group_id = 0;
   struct in6_addr * v6;

   if (a == 0 || pSrvType == 0)
      return -1;

   /* Run Hash to get group id */
   while (len-- != 0)
   {
      group_id *= 33;
      group_id += *pSrvType++;
   }
   group_id &= 0x3FF;

   v6 = &((struct sockaddr_in6 *)addr)->sin6_addr;
   if (scope == SLP_SCOPE_NODE_LOCAL)
      memcpy(v6, &in6addr_service_node_mask, sizeof(struct in6_addr));
   else if (scope == SLP_SCOPE_LINK_LOCAL)
      memcpy(v6, &in6addr_service_link_mask, sizeof(struct in6_addr));
   else if (scope == SLP_SCOPE_SITE_LOCAL)
      memcpy(v6, &in6addr_service_site_mask, sizeof(struct in6_addr));
   else
      return -1;

   v6->s6_addr[15] |= (group_id & 0xFF);
   v6->s6_addr[14] |= (group_id >> 8);
   a->sa_family = AF_INET6;

   return 0;
}

/** Expands an IPv6 address from shorthand to full format notation.
 *
 * @param[in] ipv6Addr - The shorthand address to be expanded.
 * @param[out] result - The address of storage for the expanded address.
 * @param[in] resultSize - The size in bytes of @p result.
 *
 * @return Zero on success, or a non-zero value on failure.
 *
 * @remarks The @p result buffer must be at least 40 bytes.
 */
int SLPNetExpandIpv6Addr(const char * ipv6Addr, char * result, 
      size_t resultSize)
{
   char templateAddr[] = "0000:0000:0000:0000:0000:0000:0000:0000";
   char * doublec;
   int sts;

   if (resultSize < sizeof(templateAddr))
      return -1;

   if ((ipv6Addr == 0) || (result == 0))
      return -1;

   strcpy(result, templateAddr);

   /* The stragety here is to divide the string up into a pre (before the
    * double colon), and a post (after the ::) area, and tackle each piece 
    * seperately. The pre-area will copy from the front, the post from the
    * rear.
    */
   sts = handlePreArea(ipv6Addr, result);
   if (sts == 0)
   {
      doublec = strstr(ipv6Addr, "::");
      if (doublec)
      {
         doublec += 1; /* get past the :: */
         sts = handlePostArea(doublec, result);
      }
   }
   return sts;

#if 0
   struct sockaddr_in6 in6;

   inet_pton(AF_INET6, ipv6Addr, &in6.sin6_addr);
   in6.sin6_family = AF_INET6;
   if (inet_ntop(in6.sin6_family, &in6.sin6_addr, result, resultSize)) 
      return 0;
   else 
      return -1;
#endif
}

/*=========================================================================
 * TEST MAIN
 */
/*#define SLP_NET_TEST*/
#ifdef SLP_NET_TEST
int main(int argc, char * argv[])
{
   char addrString[1024];
   int sts;
   int errorCount = 0;
   struct sockaddr_storage addr;

#ifdef _WIN32
   WSADATA wsadata;
   WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif

   sts = SLPNetResolveHostToAddr("localhost", &addr);
   if (sts != 0)
   {
      printf("error %d with SLPNetResolveHostToAddr.\r\n", sts);
      errorCount++;
   }
   else
   {
      printf("addr family = %d\r\n", addr.ss_family);
      SLPNetSockAddrStorageToString(&addr, addrString, sizeof(addrString));
      printf("address = %s\r\n", addrString);
   }

   sts = SLPNetResolveHostToAddr("::1", &addr);
   if (sts != 0)
   {
      printf("error %d with SLPNetResolveHostToAddr.\r\n", sts);
      errorCount++;
   }
   else
   {
      printf("addr family = %d\r\n", addr.ss_family);
      SLPNetSockAddrStorageToString(&addr, addrString, sizeof(addrString));
      printf("address = %s\r\n", addrString);
   }

   sts = SLPPropertyReadFile("e:\\source\\Hogwarts_ActiveX\\OpenSLP\\"
                         "ipv6\\win32\\slpd\\slp.conf");
   if (sts == 0)
      printf("Read config file\r\n");
   else
      printf("No config file found - using defaults.\r\n");

   sts = SLPNetIsIPV6();
   if (sts == 0)
      printf("Not using ipv6\r\n");
   else
      printf("Using ipv6\r\n");

   sts = SLPNetIsIPV4();
   if (sts == 0)
      printf("Not using ipv4\r\n");
   else
      printf("Using ipv4\r\n");
   {
      struct sockaddr_storage a1;
      struct sockaddr_storage a2;
      char testaddr[] = "1:2:3:4:5::6";
      struct sockaddr_in * p41 = (struct sockaddr_in *)&a1;
      struct sockaddr_in6 * p61 = (struct sockaddr_in6 *)&a1;
      struct sockaddr_in * p42 = (struct sockaddr_in *)&a2;
      struct sockaddr_in6 * p62 = (struct sockaddr_in6 *)&a2;

      memset(&a1, 0, sizeof(a1));
      memset(&a2, 0, sizeof(a2));
      SLPNetSetAddr(&a1, AF_INET6, 2, testaddr, sizeof(testaddr));
      memcpy(&a2, &a1, sizeof(a1));
      sts = SLPNetCompareAddrs(&a1, &a2);
      if (sts != 0)
         printf("Error, address a1 does not equal a2 - copy failed\r\n");
      memset(&a2, 0, sizeof(a2));
      a2.ss_family = AF_INET6;
      memcpy(p62->sin6_addr.s6_addr, testaddr, sizeof(testaddr));
      p62->sin6_family = AF_INET6;
      p62->sin6_port = htons(2);
      sts = SLPNetCompareAddrs(&a1, &a2);
      if (sts != 0)
         printf("Error, address a1 does not equal a2\r\n");
   }
   /* now test the ipv6 expansion */
   {
      char t1[] = "::";
      char a1[] = "0000:0000:0000:0000:0000:0000:0000:0000";

      char t2[] = "1::";
      char a2[] = "0001:0000:0000:0000:0000:0000:0000:0000";

      char t3[] = "::1";
      char a3[] = "0000:0000:0000:0000:0000:0000:0000:0001";

      char t4[] = "12::34";
      char a4[] = "0012:0000:0000:0000:0000:0000:0000:0034";

      char t5[] = "1111:2222:3333::5555:6666:7777:8888";
      char a5[] = "1111:2222:3333:0000:5555:6666:7777:8888";

      char t6[] = "1:02::003:0004";
      char a6[] = "0001:0002:0000:0000:0000:0000:0003:0004";

      char t7[] = "0001:0002:0003:0004:0005:0006:0007:0008";
      char a7[] = "0001:0002:0003:0004:0005:0006:0007:0008";

      char t8[] = "1:02:003:0004:0005:006:07:8";
      char a8[] = "0001:0002:0003:0004:0005:0006:0007:0008";

      char i1[] = "1::2::3";
      char i2[] = "1:::3";

      char buf[40];  /* min buf size - 8*4 + 7 + null */
      int sts;

      sts = SLPNetExpandIpv6Addr(t1, buf, sizeof(buf));
      if ((sts != 0) || (strcmp(buf, a1) != 0))
         printf("Error expanding ipv6 address t1\r\n");

      sts = SLPNetExpandIpv6Addr(t2, buf, sizeof(buf));
      if ((sts != 0) || (strcmp(buf, a2) != 0))
         printf("Error expanding ipv6 address t2\r\n");

      sts = SLPNetExpandIpv6Addr(t3, buf, sizeof(buf));
      if ((sts != 0) || (strcmp(buf, a3) != 0))
         printf("Error expanding ipv6 address t3\r\n");

      sts = SLPNetExpandIpv6Addr(t4, buf, sizeof(buf));
      if ((sts != 0) || (strcmp(buf, a4) != 0))
         printf("Error expanding ipv6 address t4\r\n");

      sts = SLPNetExpandIpv6Addr(t5, buf, sizeof(buf));
      if ((sts != 0) || (strcmp(buf, a5) != 0))
         printf("Error expanding ipv6 address t5\r\n");

      sts = SLPNetExpandIpv6Addr(t6, buf, sizeof(buf));
      if ((sts != 0) || (strcmp(buf, a6) != 0))
         printf("Error expanding ipv6 address t6\r\n");

      sts = SLPNetExpandIpv6Addr(t7, buf, sizeof(buf));
      if ((sts != 0) || (strcmp(buf, a7) != 0))
         printf("Error expanding ipv6 address t7\r\n");

      sts = SLPNetExpandIpv6Addr(t8, buf, sizeof(buf));
      if ((sts != 0) || (strcmp(buf, a8) != 0))
         printf("Error expanding ipv6 address t8\r\n");

      sts = SLPNetExpandIpv6Addr(i1, buf, sizeof(buf));
      sts = SLPNetExpandIpv6Addr(i2, buf, sizeof(buf));
      sts = SLPNetExpandIpv6Addr(t6, buf, 5);
      if (sts == 0)
         printf("Error, size not checked for expansion\r\n");
   }

   /* int SLPNetIsMCast(const struct sockaddr_storage *addr);
    * int SLPNetIsLocal(const struct sockaddr_storage *addr);
    */

#ifdef _WIN32
   WSACleanup();
#endif
}
#endif

#if 0
/** Returns local hostname.
 *
 * @param[out] hostfdn - A pointer to char pointer that is set to buffer 
 *    contining this machine's FDN.
 * @param[in] hostfdnLen - The length of @p hostfdn. 
 * @param[in] numeric_only - A flag that forces the return of numeric 
 *    addresss.
 * @param[in] family - A hint: The family to get info for - can be 
 *    AF_INET, AF_INET6, or AF_UNSPEC for both.
 *
 * @remarks Caller must free returns @p hostfdn string with xfree.
 */
static int SLPNetGetThisHostname(char * hostfdn, size_t hostfdnLen,
      int numeric_only, int family)
{
   char host[MAX_HOST_NAME];
   struct addrinfo * ifaddr;
   struct addrinfo hints;
   int sts = 0;

   *hostfdn = 0;

   memset(&hints, 0, sizeof(hints));
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_family = family;
   if (gethostname(host, MAX_HOST_NAME) == 0)
   {
      sts = getaddrinfo(host, 0, &hints, &ifaddr);
      if (sts == 0)
      {
         /* If the hostname has a '.' then it is probably a qualified 
          * domain name. If it is not then we better use the IP address.
          */ 
         if (!numeric_only && strchr(host, '.'))
            strncpy(hostfdn, host, hostfdnLen);
         else
            sts = SLPNetAddrInfoToString(ifaddr, hostfdn, hostfdnLen);
         freeaddrinfo(ifaddr);
      }
      else
         assert(1);  /* TODO: what? assert(1) does nothing by definition! */
   }
   return(sts);
}

/** Compare two address structures.
 *
 * If the host addresses, ports, and other local configuration are the
 * same, then return zero.
 *
 * @param[in] addr1 - The first address to compare.
 * @param[in] addr2 - The second address to compare.
 *
 * @return Zero if @p addr1 is the same as @p addr2. Non-zero if not.
 *
 * @note The sizes of @p addr1 and @p addr2 must be at least 
 *    sizeof(struct sockaddr_storage) if the address families are not
 *    one of AF_INET or AF_INET6.
 *
 * @internal
 */
static int SLPNetCompareStructs(const void * addr1, const void * addr2)
{
   const struct sockaddr * a1 = (const struct sockaddr *)addr1;
   const struct sockaddr * a2 = (const struct sockaddr *)addr2;

   if (a1->sa_family == a2->sa_family)
   {
      if (a1->sa_family == AF_INET)
      {
         struct sockaddr_in * v41 = (struct sockaddr_in *)addr1;
         struct sockaddr_in * v42 = (struct sockaddr_in *)addr2;
         if (v41->sin_family == v42->sin_family
               && v41->sin_port == v42->sin_port)
            return memcmp(&v41->sin_addr, &v42->sin_addr, 
                  sizeof(v41->sin_addr));
      }
      else if (a1->sa_family == AF_INET6)
      {
         struct sockaddr_in6 * v61 = (struct sockaddr_in6 *)addr1;
         struct sockaddr_in6 * v62 = (struct sockaddr_in6 *)addr2;
         if (v61->sin6_family == v62->sin6_family
               && v61->sin6_port == v62->sin6_port
               && v61->sin6_scope_id == v62->sin6_scope_id)
            /* && v61->sin6_flowinfo == v62->sin6_flowinfo) */
            return memcmp(&v61->sin6_addr, &v62->sin6_addr,
                        sizeof(v61->sin6_addr));
      }
      else  /* don't know how to decode - use memcmp for now */
         return memcmp(addr1, addr2, sizeof(struct sockaddr_storage));
   }
   return -1;
}

/** Determines the IPv6 scope of a specified address.
 *
 * @param[in] v6Addr - The IPv6 address to be checked.
 *
 * @return The ipv6 scope of the address.
 *
 * @remarks The @p v6Addr parameter must be pointer to a 16-byte IPv6 
 *    address in binary form.
 * 
 * @internal
 */
static int setScopeFromAddress(const struct in6_addr * v6Addr)
{
   if (IN6_IS_ADDR_MULTICAST(v6Addr))
   {
      if (IN6_IS_ADDR_MC_GLOBAL(v6Addr))
         return SLP_SCOPE_GLOBAL;

      if (IN6_IS_ADDR_MC_ORGLOCAL(v6Addr))
         return SLP_SCOPE_ORG_LOCAL;

      if (IN6_IS_ADDR_MC_SITELOCAL(v6Addr))
         return SLP_SCOPE_SITE_LOCAL;

      if (IN6_IS_ADDR_MC_NODELOCAL(v6Addr))
         return SLP_SCOPE_NODE_LOCAL;

      if (IN6_IS_ADDR_MC_LINKLOCAL(v6Addr))
         return SLP_SCOPE_LINK_LOCAL;
   }
   if (IN6_IS_ADDR_SITELOCAL(v6Addr))
      return SLP_SCOPE_SITE_LOCAL;

   if (SLP_IN6_IS_ADDR_LOOPBACK(v6Addr))
      return SLP_SCOPE_NODE_LOCAL;

   if (IN6_IS_ADDR_LINKLOCAL(v6Addr))
      return SLP_SCOPE_LINK_LOCAL;

   return 0;
}

/** Copies an addrinfo structure into a address structure.
 *
 * @param[out] dst - The destination address structure.
 * @param[in] src - The source addrinfo structure.
 *
 * @return Zero on success, or a non-zero value on failure.
 */
static int SLPNetSetSockAddrStorageFromAddrInfo(
      struct sockaddr_storage * dst,
      const struct addrinfo * src)
{
   dst->ss_family = (short)src->ai_family;
   if (src->ai_family == AF_INET)
      memcpy(dst, src->ai_addr, sizeof(struct sockaddr_in));
   else if (src->ai_family == AF_INET6)
   {
      struct sockaddr_in6 * v6 = (struct sockaddr_in6 *)dst;
      v6->sin6_family = AF_INET6;
      v6->sin6_flowinfo = 0;
      v6->sin6_port = 0;
      v6->sin6_scope_id = 0;
      memcpy(&v6->sin6_addr,
            &((struct sockaddr_in6 *)src->ai_addr)->sin6_addr,
            sizeof(struct in6_addr));
   }
   else
      return -1;
   return 0;
}
#endif

/*=========================================================================*/
