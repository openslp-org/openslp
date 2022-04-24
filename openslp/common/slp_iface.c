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

/** Functions for obtaining network interface information.
 *
 * @todo The interface routines in slp_dhcp.c are similar - these should
 *    be merged.
 *
 * @file       slp_iface.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeNetIfc
 */

#include "slp_types.h"
#include "slp_iface.h"
#include "slp_xmalloc.h"
#include "slp_compare.h"
#include "slp_net.h"
#include "slp_property.h"
#include "slp_socket.h"
#include "slp_debug.h"

/** The max index for v6 address to test for valid scope ids. */
#define MAX_INTERFACE_TEST_INDEX 255
/** The max interface name lenght is 20 */
#define MAX_IFACE_LEN 20

int slp_max_ifaces = SLP_MAX_IFACES;

/**
 * A Windows implementation of getifaddrs/freeifaddrs.
 */
#ifdef _WIN32

struct ifa_data
{
   char ifa_name[IF_NAMESIZE];
   struct sockaddr_storage ifa_addr;
   struct sockaddr_storage ifa_netmask;
};

/* struct ifaddrs: kyped from linux man pages */
struct ifaddrs
{
   struct ifaddrs * ifa_next;
   char * ifa_name;
   unsigned int ifa_flags;
   struct sockaddr * ifa_addr;
   struct sockaddr * ifa_netmask;

/* NOTE: The following fields are not currently implemented:
   union
   {
      struct sockaddr * ifu_broadaddr;
      struct sockaddr * ifu_dstaddr;
   } ifa_ifu;
#define ifa_broadaddr ifa_ifu.ifu_broadaddr
#define ifa_dstaddr ifa_ifu.ifu_dstaddr
   void * ifa_data;
*/
};

/* freeifaddrs is a simple passthru to xfree */
#define freeifaddrs xfree

/** Return an allocated buffer of interface address structures.
 *
 * @param[out] ifap the address of storage for a list of interface address structures.
 *
 * @return zero on success and -1 (plus errno) on failure.
 */
static int getifaddrs(struct ifaddrs** ifap)
{
#define GAA_FLAGS (GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_MULTICAST)
#define DEFAULT_BUFFER_SIZE 15000
#define MAX_TRIES 3

   DWORD dwRet = 0;
   DWORD dwSize = DEFAULT_BUFFER_SIZE;
   IP_ADAPTER_ADDRESSES * pAdapterAddresses = 0;
   IP_ADAPTER_ADDRESSES * adapter;
   struct ifaddrs * ifa;
   struct ifaddrs * ift;
   unsigned i;
   int n = 0;

   for (i = MAX_TRIES; i && dwRet != ERROR_BUFFER_OVERFLOW; --i)
   {
      IP_ADAPTER_ADDRESSES * tmp_aa;
      if ((tmp_aa = (IP_ADAPTER_ADDRESSES*)xrealloc(pAdapterAddresses, dwSize)) == 0)
      {
         xfree(pAdapterAddresses);
         return (errno = ENOMEM), -1;
      }
      pAdapterAddresses = tmp_aa;
      dwRet = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAGS, 0, pAdapterAddresses, &dwSize);
   }

   switch (dwRet)
   {
      case ERROR_SUCCESS:
         break;
      case ERROR_BUFFER_OVERFLOW:
         xfree(pAdapterAddresses);
         return (errno = ENOMEM), -1;
      default:
         xfree(pAdapterAddresses);
         return (errno = EFAULT), -1;
   }

   for (adapter = pAdapterAddresses; adapter; adapter = adapter->Next)
   {
      IP_ADAPTER_UNICAST_ADDRESS * unicast;
      for (unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next)
         if (unicast->Address.lpSockaddr->sa_family == AF_INET || unicast->Address.lpSockaddr->sa_family == AF_INET6)
            ++n;
   }

   if ((ift = ifa = (struct ifaddrs*)xcalloc(sizeof(struct ifaddrs) + sizeof(struct ifa_data), n)) == 0)
   {
      xfree(pAdapterAddresses);
      return (errno = ENOMEM), -1;
   }

   for (adapter = pAdapterAddresses; adapter; adapter = adapter->Next)
   {
      int unicastIndex = 0;
      IP_ADAPTER_UNICAST_ADDRESS * unicast;
      for (unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next, ++unicastIndex)
      {
         struct ifa_data * data;
         ULONG prefixLength;

         /* is IP adapter? */
         if (unicast->Address.lpSockaddr->sa_family != AF_INET && unicast->Address.lpSockaddr->sa_family != AF_INET6)
            continue;

         /* internal data pointer */
         data = (struct ifa_data*)&ift[1];

         /* name */
         ift->ifa_name = data->ifa_name;
         strncpy_s(ift->ifa_name, IF_NAMESIZE, adapter->AdapterName, _TRUNCATE);

         /* flags */
         ift->ifa_flags = 0;
         if (adapter->OperStatus == IfOperStatusUp)
            ift->ifa_flags |= IFF_UP;
         if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
            ift->ifa_flags |= IFF_LOOPBACK;
         if (!(adapter->Flags & IP_ADAPTER_NO_MULTICAST))
            ift->ifa_flags |= IFF_MULTICAST;

         /* address */
         ift->ifa_addr = (struct sockaddr*)&data->ifa_addr;
         memcpy(ift->ifa_addr, unicast->Address.lpSockaddr, unicast->Address.iSockaddrLength);

         /* netmask */
         ift->ifa_netmask = (struct sockaddr*)&data->ifa_netmask;

         prefixLength = 0;

#if _WIN32_WINNT >= 0x0600
# define IN6_IS_ADDR_TEREDO(addr) (((uint32_t*)(addr))[0] == ntohl(0x20010000))

         if (unicast->Address.lpSockaddr->sa_family == AF_INET6
               && adapter->TunnelType == TUNNEL_TYPE_TEREDO
               && IN6_IS_ADDR_TEREDO(&((struct sockaddr_in6*)unicast->Address.lpSockaddr)->sin6_addr)
               && unicast->OnLinkPrefixLength != 32)
            prefixLength = 32;
         else
            prefixLength = unicast->OnLinkPrefixLength;
#else
# define IN_LINKLOCAL(a) ((((uint32_t)(a)) & 0xaffff0000) == 0xa9fe0000)
         {
            IP_ADAPTER_PREFIX * prefix;
            for (prefix = adapter->FirstPrefix; prefix; prefix = prefix->Next)
            {
               LPSOCKADDR lpSockaddr = prefix->Address.lpSockaddr;
               if (lpSockaddr->sa_family != unicast->Address.lpSockaddr->sa_family)
                  continue;

               if (lpSockaddr->sa_family == AF_INET && adapter->OperStatus != IfOperStatusUp)
               {
                  if (IN_LINKLOCAL(ntohl((struct sockaddr_in*)unicast->Address.lpSockaddr)->sin_addr.s_addr))
                     prefixLength = 16;
                  break;
               }
               if (lpSockaddr->sa_family == AF_INET6 && !prefix->PrefixLength
                     && IN6_IS_ADDR_UNSPECIFIED(&((struct sockaddr_in6*)lpSockaddr)->sin6_addr))
                  continue;

               prefixLength = prefix->PrefixLength;
               break;
            }
         }
#endif /* _WIN32_WINNT >= 0x0600 */

         ift->ifa_netmask->sa_family = unicast->Address.lpSockaddr->sa_family;
         switch (unicast->Address.lpSockaddr->sa_family)
         {
            case AF_INET:
               if (!prefixLength || prefixLength > 32)
                  prefixLength = 32;

#if _WIN32_WINNT >= 0x0600
               {
                  ULONG Mask;
                  ConvertLengthToIpv4Mask(prefixLength, &Mask);
                  ((struct sockaddr_in*)ift->ifa_netmask)->sin_addr.s_addr = htonl(Mask);
               }
#else
               ((struct sockaddr_in*)ift->ifa_netmask)->sin_addr.s_addr = htonl(0xffffffffU << (32 - prefixLength));
#endif
               break;

            case AF_INET6:
            {
               ULONG i, j;
               if (!prefixLength || prefixLength > 128)
                  prefixLength = 128;
               for (i = prefixLength, j = 0; i > 0; i -= 8, ++j)
                  ((struct sockaddr_in6*)ift->ifa_netmask)->sin6_addr.s6_addr[j] = i >= 8? 0xff: (ULONG)((0xffU << (8 - i)) & 0xffU);
               break;
            }
         }

         /* link and move to next structure in linked list */
         ift->ifa_next = (struct ifaddrs*)&data[1];
         ift = ift->ifa_next;
      }
   }

   /* cleanup and return structure */
   xfree(pAdapterAddresses);
   *ifap = ifa;
   return 0;
}

/** Parse a string into tokens separated by delimiter characters.
 *
 * @param[in] str the string to be parsed or NULL on successive iterations.
 * @param[in] delim the delimiter set used to separate tokens.
 * @param[out] saveptr the next str to use when str is NULL.
 *
 * @return Each call returns a pointer to the next token.
 */
char * strtok_r(char * str, const char * delim, char ** saveptr)
{
   char * token;

   /* restore save ptr on successive iterations */
   if (!str)
      str = *saveptr;

   /* skip leading delims */
   str += strspn(str, delim);
   if (!*str)
      return 0;

   /* locate end of token */
   token = str;
   str = strpbrk(token, delim);
   if (!str)
      *saveptr = strchr(token, 0);
   else
   {
      *str = 0;
      *saveptr = str + 1;
   }
   return token;
}

#endif  /* _WIN32 */

/** Custom designed wrapper for inet_pton to allow <ip>%<iface> format
 *
 * @param[in] af - A integer representing address family
 * @param[in] src - A pointer to source address
 * @param[out] dst - A pointer to structure in_addr6
 *
 * @return As per the lib call inet_pton()
 * @internal
 */
static int SLP_inet_pton(int af, char *src, void *dst)
{
   if (af == AF_INET6)
   {
      char *src_ptr = src;
      char tmp_addr[INET6_ADDRSTRLEN];
      char *tmp_ptr = tmp_addr;
      int bufleft = INET6_ADDRSTRLEN;

      while (*src_ptr && (*src_ptr != '%'))
      {
         /* Need to have at least one byte left for the trailing NUL */
         if (!--bufleft)
            return -1;

         *tmp_ptr++ = *src_ptr++;
      }
      *tmp_ptr = '\0';
      return inet_pton(af, (char *)&tmp_addr, dst);
   }
   return inet_pton(af, src, dst);
}

/** Checks a string-list for the occurence of a string
 *
 * @param[in] list - A pointer to the string-list to be checked.
 * @param[in] listlen - The length in bytes of @p list.
 * @param[in] string - A pointer to a string to find in @p list.
 * @param[in] stringlen - The length in bytes of @p string.
 *
 * @return Zero if @p string is NOT contained in @p list;
 *    A non-zero value if it is.
 *
 * @internal
 */
static int SLPIfaceContainsAddr(size_t listlen, const char * list,
      size_t stringlen, const char * string)
{
   char * listend = (char *) list + listlen;
   char * itembegin = (char *) list;
   char * itemend = itembegin;
   struct sockaddr_storage addr;
   char buffer[INET6_ADDRSTRLEN + MAX_IFACE_LEN]; /* must be at least 40 characters for address string and 20 for inetrface name */
   int buffer_len;

   while (itemend < listend)
   {
      itembegin = itemend;

      /* seek to the end of the next list item */
      while (1)
      {
         if (itemend == listend || *itemend == ',')
            if (*(itemend - 1) != '\\')
               break;
         itemend ++;
      }
      if (itemend - itembegin < sizeof(buffer))
         buffer_len = (int)(itemend - itembegin);
      else
         buffer_len = sizeof(buffer);
      strncpy(buffer, itembegin, buffer_len);
      buffer[itemend - itembegin] = '\0';
      if (SLPNetIsIPV6() && SLP_inet_pton(AF_INET6, buffer, &addr) == 1)
      {
         if (SLPCompareString(strlen(buffer), buffer, stringlen, string) == 0)
            return 1;
      }
      else if (SLPNetIsIPV4() && SLP_inet_pton(AF_INET, buffer, &addr) == 1)
      {
         if (SLPCompareString(strlen(buffer), buffer, stringlen, string) == 0)
            return 1;
      }
      itemend++;
   }
   return 0;
}

/** Given an IPv6 address, attempt to find the scope/network interface
 *
 * While there are a few better ways to do this (if_nametoindex, iphlpapi(windows)),
 * for now we'll use the method that was in this file, attempting to bind to the scope,
 * but this isn't a final solution.
 *
 * @param[in,out] addr - The ipv6 address, whose scopeid is modified
 *
 * @return Zero on success, non-zero on error
 *
 */
static int GetV6Scope(struct sockaddr_in6 *addr, char *iface)
{
   int result = -1;
#if defined(LINUX)
   struct ifaddrs * ifa, * ifaddr;
   struct sockaddr_in6 *paddr;

   if (getifaddrs(&ifa))
      return result;

   ifaddr = ifa;

   if (!iface)
   {
      for (; ifa; ifa = ifa->ifa_next)
      {
         /* filter out NULL address interfaces */
         if (!ifa->ifa_addr)
            continue;

         /* filter out non-v6 interfaces */
         if (ifa->ifa_addr->sa_family != AF_INET6)
            continue;

         paddr = (struct sockaddr_in6 *)ifa->ifa_addr;
         if (!memcmp(&paddr->sin6_addr, &addr->sin6_addr, sizeof(struct in6_addr)))
         {
            addr->sin6_scope_id = ((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_scope_id;
            result = 0;
            break;
         }
      }
      freeifaddrs(ifaddr);
      return result;
   }

   for (; ifa; ifa = ifa->ifa_next)
   {
      /* filter out NULL address interfaces */
      if (!ifa->ifa_addr)
         continue;

      /* filter out non-v6 interfaces */
      if (ifa->ifa_addr->sa_family != AF_INET6)
         continue;

      paddr = (struct sockaddr_in6 *)ifa->ifa_addr;
      if ((!strcmp(iface, ifa->ifa_name)) && (!memcmp(&paddr->sin6_addr, &addr->sin6_addr, sizeof(struct in6_addr))))
      {
         addr->sin6_scope_id = ((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_scope_id;
         result = 0;
         break;
      }
   }
   freeifaddrs(ifaddr);
#elif defined _WIN32
   sockfd_t fd;
   DWORD retVal = 0;
   int i = 0;
   ULONG flg = GAA_FLAG_INCLUDE_PREFIX;
   ULONG family = AF_UNSPEC;
   PIP_ADAPTER_ADDRESSES pAddr = NULL;
   ULONG outBufLen = 0;
   PIP_ADAPTER_ADDRESSES pCurrAddr = NULL;
   PIP_ADAPTER_UNICAST_ADDRESS pUnicastAddr = NULL;
   family = AF_INET6;

   /* Check if address is a global address and if it is then assign a scope ID as zero */
   fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
   if (fd != INVALID_SOCKET)
   {
      if (addr != NULL )
      {
         addr->sin6_scope_id = 0;
         if(IN6ADDR_ISLOOPBACK(addr))
         {
            addr->sin6_scope_id = 1; //LoopBack Address
         }
         if ((result = bind(fd, (struct sockaddr *)addr, sizeof(struct sockaddr_in6))) == 0)
         {
            closesocket(fd);
            result = 0;
            return result;
         }
      }
      closesocket(fd);
   }

   outBufLen = sizeof (IP_ADAPTER_ADDRESSES);
   if ((pAddr = (IP_ADAPTER_ADDRESSES *)xmalloc(outBufLen)) == 0)
      return result;

   if (GetAdaptersAddresses(family, flg, NULL, pAddr, &outBufLen) == ERROR_BUFFER_OVERFLOW)
   {
      xfree(pAddr);
      if ((pAddr = (IP_ADAPTER_ADDRESSES *)xmalloc(outBufLen)) == 0)
         return result;
   }

   retVal = GetAdaptersAddresses(family, flg, NULL, pAddr, &outBufLen);
   if (retVal == NO_ERROR)
   {
      pCurrAddr = pAddr;
      while (pCurrAddr)
      {
         pUnicastAddr = pCurrAddr->FirstUnicastAddress;
         if (pUnicastAddr != NULL)
         {
            for (i = 0; pUnicastAddr != NULL; i++)
            {
               struct sockaddr_in6 * paddr = (struct sockaddr_in6 *)(pUnicastAddr->Address.lpSockaddr);
               if (!iface)
               {
                  if (!memcmp(&paddr->sin6_addr, &addr->sin6_addr, sizeof(struct in6_addr)))
                  {
                     if (IN6ADDR_ISLOOPBACK(paddr))
                     {
                        addr->sin6_scope_id = 1; // LoopBack Address
                        return 0;
                     }
                     else
                     {
                        addr->sin6_scope_id = paddr->sin6_scope_id;
                        return 0;
                     }
                  }
               }
               else
               {
                  // Convert to a wchar_t*
                  size_t origsize = strlen(iface) + 1;
                  size_t convertedChars = 0;
                  wchar_t wcstr[128];
                  PWCHAR pwcstr=wcstr;
                  mbstowcs_s(&convertedChars, wcstr, origsize, iface, _TRUNCATE);

                  if ((!wcscmp(pwcstr, pCurrAddr->FriendlyName)) &&
                     !memcmp(&paddr->sin6_addr, &addr->sin6_addr, sizeof(struct in6_addr)))
                  {
                     if (IN6ADDR_ISLOOPBACK(paddr))
                     {
                        addr->sin6_scope_id = 1; //LoopBack Address
                        result = 0;
                        return result;
                     }
                     else
                     {
                        addr->sin6_scope_id = paddr->sin6_scope_id;
                        result = 0;
                        return result;
                     }
                  }
               }
               pUnicastAddr = pUnicastAddr->Next;
            }
         }
         pCurrAddr = pCurrAddr->Next;
      }
   }
   if (pAddr != NULL)
      xfree(pAddr);
#else
   sockfd_t fd;
   fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
   if (fd != SLP_INVALID_SOCKET)
   {
      /* This loop attempts to find the proper scope value
         in case the tested address is an ipv6 link-local
         address. In case of a global address a scope value
         of zero will bind immediately so the loop causes
         no harm for global addresses. */

      int i;
      for (i = 0; i < MAX_INTERFACE_TEST_INDEX; i++)
      {
         addr->sin6_scope_id = i;
         if ((result = bind(fd, (struct sockaddr *)addr, sizeof(struct sockaddr_in6))) == 0)
            break;
      }
      closesocket(fd);
   }
#endif
   return result;
}

#if defined(LINUX)
/** Fetches the interface IPv6 address information of a linux host
 *
 * @param[in,out] ifaceinfo - The address of a buffer in which to return
 *    information about the requested interfaces. Note that the address
 *    count should be passed in pointing to the next available slot in the
 *    address list of this parameter.
 *
 * @return Zero on success; A non-zero value (with errno set) on error.
 *
 * @remarks Does NOT return the loopback interface.
 *
 * @remarks This routine APPENDS to @p ifaceinfo by assuming that its
 *    address count field is set to the position of the next available
 *    slot in the address array.
 */
static int SLPIfaceGetV6Addr(SLPIfaceInfo * ifaceinfo)
{
   struct sockaddr_in6* paddr, *ifaddr;
   struct ifaddrs *ifa, *ifaddrs;

   if (getifaddrs(&ifaddrs))
      return -1;

   for (ifa = ifaddrs; ifa && ifaceinfo->iface_count < slp_max_ifaces; ifa = ifa->ifa_next)
   {
      /* filter out NULL address interfaces */
      if (!ifa->ifa_addr)
         continue;

      /* filter out non-v6 interfaces */
      if (ifa->ifa_addr->sa_family != AF_INET6)
          continue;

      /* filter out loopback interfaces */
      if (!strcmp("lo", ifa->ifa_name))
          continue;

      paddr = (struct sockaddr_in6*)&ifaceinfo->iface_addr[ifaceinfo->iface_count];
      memset(paddr, 0, sizeof(struct sockaddr_in6));
      ifaddr = (struct sockaddr_in6 *)ifa->ifa_addr;

      memcpy(&paddr->sin6_addr, &ifaddr->sin6_addr, sizeof(struct in6_addr));
      paddr->sin6_scope_id = ifaddr->sin6_scope_id;

      paddr->sin6_family = AF_INET6;
#ifdef HAVE_SOCKADDR_STORAGE_SS_LEN
      paddr->sin6_len = sizeof(struct sockaddr_in6);
#endif
      ++ifaceinfo->iface_count;
   }
   freeifaddrs(ifaddrs);
   if (ifa)
   {
      errno = ENOBUFS;
      return -1;
   }
   return 0;
}

#endif /*LINUX, pt1*/

/** @todo: evaluate network interface discovery on AIX, Solaris, and Hpux */
#if defined(LINUX) || defined(AIX) || defined(SOLARIS) || defined(HPUX) || defined(DARWIN)

/** Small helper for SLPIfaceGetDefaultInfo
 *
 * Returns the size of the ifr structure, which is dependent upon
 * whether or not the socket address contains a length.
 * The size includes the size of the ifr_name field
 *
 * @param[in] ifr - the ifr structure to size
 *
 * @return The calculated size of the structure
 *
 * @internal
 */
static int sizeof_ifreq(struct ifreq* ifr)
{
#ifdef HAVE_SOCKADDR_STORAGE_SS_LEN
  int len = ifr->ifr_addr.sa_len + sizeof(ifr->ifr_name);
  if (len < sizeof(struct ifreq))
    len = sizeof(struct ifreq);
  return len;
#else
  return sizeof(struct ifreq);
#endif
}

/** Get all network interface addresses for this host.
 *
 * Returns the list of network interface addresses for this host matching
 * the specified address family or all families if AF_UNSPEC is passed.
 *
 * @param[in,out] ifaceinfo - The address of a buffer in which to return
 *    information about the requested interfaces. Note that the address
 *    count should be passed in pointing to the next available slot in the
 *    address list of this parameter.
 * @param[in] family - A hint indicating the address family to get info
 *    for - can be AF_INET, AF_INET6, or AF_UNSPEC for both.
 *
 * @return Zero on success; A non-zero value (with errno set) on error.
 *
 * @remarks Does NOT return the loopback interface.
 *
 * @remarks This routine APPENDS to @p ifaceinfo by assuming that its
 *    address count field is set to the position of the next available
 *    slot in the address array.
 */
int SLPIfaceGetDefaultInfo(SLPIfaceInfo * ifaceinfo, int family)
{
   sockfd_t fd;
   struct ifreq *ifrlist;
   struct ifreq ifrflags;
   struct ifreq * ifr;
   struct sockaddr* sa;
   char * p;

   struct ifconf ifc;
   if ((ifrlist = xmalloc(sizeof(struct ifreq) * slp_max_ifaces)) == 0)
      return (errno = ENOMEM), -1;

   ifc.ifc_len = sizeof(struct ifreq) * slp_max_ifaces;
   ifc.ifc_req = ifrlist;

   if ((family == AF_INET6) || (family == AF_UNSPEC))
   {
#ifdef LINUX
      SLPIfaceGetV6Addr(ifaceinfo);
#else
      fd = socket(AF_INET6, SOCK_DGRAM, 0);
      if (fd != -1)
      {
#ifdef AIX
         if (ioctl(fd, OSIOCGIFCONF, &ifc) < 0)
#else
         if (ioctl(fd, SIOCGIFCONF, &ifc) < 0)
#endif
         {
            closesocket(fd);
            xfree(ifrlist);
            return (errno = EFAULT), -1;
         }

         for (p = ifc.ifc_buf; p < ifc.ifc_buf + ifc.ifc_len && ifaceinfo->iface_count < slp_max_ifaces;)
         {
            ifr = (struct ifreq*) p;
            sa = (struct sockaddr*)&(ifr->ifr_addr);
            p += sizeof_ifreq(ifr);  /*for next iteration*/

            if (sa->sa_family == AF_INET6)
            {
               /* get interface flags, skip loopback addrs */
               memcpy(&ifrflags, ifr, sizeof(struct ifreq));
               if (ioctl(fd, SIOCGIFFLAGS, &ifrflags) == 0
                     && (ifrflags.ifr_flags & IFF_LOOPBACK) == 0
                     && (GetV6Scope((struct sockaddr_in6*)sa, NULL) == 0))
                  memcpy(&ifaceinfo->iface_addr[ifaceinfo->iface_count++],
                        sa, sizeof(struct sockaddr_in6));
            }
         }
         closesocket(fd);
      }
#endif /*Non-linux IPv6*/
   }

   /* reset ifc_len for next get */
   ifc.ifc_len = sizeof(struct ifreq) * slp_max_ifaces;

   if ((family == AF_INET) || (family == AF_UNSPEC))
   {
      fd = socket(AF_INET, SOCK_DGRAM, 0);
      if (fd != -1)
      {
#ifdef AIX
         if (ioctl(fd, OSIOCGIFCONF, &ifc) < 0)
#else
         if (ioctl(fd, SIOCGIFCONF, &ifc) < 0)
#endif
         {
            closesocket(fd);
            xfree(ifrlist);
            return (errno = EFAULT), -1;
         }

         for (p = ifc.ifc_buf; p < ifc.ifc_buf + ifc.ifc_len && ifaceinfo->iface_count < slp_max_ifaces;)
         {
            ifr = (struct ifreq*) p;
            sa = (struct sockaddr*)&(ifr->ifr_addr);
            p += sizeof_ifreq(ifr);  /*for next iteration*/

            if (sa->sa_family == AF_INET)
            {
               /* get interface flags, skip loopback addrs */
               memcpy(&ifrflags, ifr, sizeof(struct ifreq));
               if (ioctl(fd, SIOCGIFFLAGS, &ifrflags) == 0
                     && (ifrflags.ifr_flags & IFF_LOOPBACK) == 0)
                  memcpy(&ifaceinfo->iface_addr[ifaceinfo->iface_count++],
                        sa, sizeof(struct sockaddr_in));
            }
         }
         closesocket(fd);
      }
   }
   xfree(ifrlist);
   return 0;
}

#elif defined(_WIN32)

/** Get all network interface addresses for this host.
 *
 * Returns the list of network interface addresses for this host matching
 * the specified address family or all families if AF_UNSPEC is passed.
 *
 * @param[in,out] ifaceinfo - The address of a buffer in which to return
 *    information about the requested interfaces. Note that the address
 *    count should be passed in pointing to the next available slot in the
 *    address list of this parameter.
 * @param[in] family - A hint indicating the address family to get info
 *    for - can be AF_INET, AF_INET6, or AF_UNSPEC for both.
 *
 * @return Zero on success; A non-zero value (with errno set) on error.
 *
 * @remarks Does NOT return the loopback interface.
 *
 * @remarks This routine APPENDS to @p ifaceinfo by assuming that its
 *    address count field is set to the position of the next available
 *    slot in the address array.
 *
 * @remarks This uses the Winsock2 WSAIoctl call of SIO_ADDRESS_LIST_QUERY,
 *    which is supported by the recommended windows compiler and latest
 *    platform sdk (currently visual studio 2005 express).
 */
int SLPIfaceGetDefaultInfo(SLPIfaceInfo* ifaceinfo, int family)
{
   sockfd_t fd;

   if ((family == AF_INET6) || (family == AF_UNSPEC))
   {
      DWORD buflen = 0;
      char * buffer = 0;
      SOCKET_ADDRESS_LIST * plist = 0;
      int i;

      if ((fd = socket(AF_INET6, SOCK_DGRAM, 0)) != INVALID_SOCKET)
      {
         /* We want to get a reasonable length buffer, so call empty first to fill in buflen, ignoring errors*/
         WSAIoctl(fd, SIO_ADDRESS_LIST_QUERY, 0, 0, buffer, buflen, &buflen, 0, 0);
         if (buflen > 0)
         {
            if ((buffer = xmalloc(buflen)) == 0)
               return (errno = ENOMEM), -1;
            if (WSAIoctl(fd, SIO_ADDRESS_LIST_QUERY, 0, 0,
                  buffer, buflen, &buflen, 0, 0) != 0)
            {
               closesocket(fd);
               xfree(buffer);
               return (errno = WSAGetLastError()), -1;
            }
            plist = (SOCKET_ADDRESS_LIST*)buffer;
            for (i = 0; i < plist->iAddressCount && ifaceinfo->iface_count < slp_max_ifaces; ++i)
               if ((plist->Address[i].lpSockaddr->sa_family == AF_INET6) &&
                   (0 == GetV6Scope((struct sockaddr_in6*)plist->Address[i].lpSockaddr, NULL)) &&
                   /*Ignore Teredo and loopback pseudo-interfaces*/
                   ((2 < ((struct sockaddr_in6*)plist->Address[i].lpSockaddr)->sin6_scope_id) ||
               0 == ((struct sockaddr_in6*)plist->Address[i].lpSockaddr)->sin6_scope_id))
                  memcpy(&ifaceinfo->iface_addr[ifaceinfo->iface_count++],
                        plist->Address[i].lpSockaddr, sizeof(struct sockaddr_in6));
            xfree(buffer);
         }
         closesocket(fd);
      }
   }

   if ((family == AF_INET) || (family == AF_UNSPEC))
   {
      DWORD buflen = 0;
      char * buffer = 0;
      SOCKET_ADDRESS_LIST * plist = 0;
      int i;

      if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) != INVALID_SOCKET)
      {
         /* We want to get a reasonable length buffer, so call empty first to fill in buflen, ignoring errors */
         WSAIoctl(fd, SIO_ADDRESS_LIST_QUERY, 0, 0, buffer, buflen, &buflen, 0, 0);
         if (buflen > 0)
         {
            if ((buffer = xmalloc(buflen)) == 0)
               return (errno = ENOMEM), -1;
            if (WSAIoctl(fd, SIO_ADDRESS_LIST_QUERY, 0, 0,
                  buffer, buflen, &buflen, 0, 0) != 0)
            {
               closesocket(fd);
               xfree(buffer);
               return (errno = WSAGetLastError()), -1;
            }
            plist = (SOCKET_ADDRESS_LIST*)buffer;
            for (i = 0; i < plist->iAddressCount && ifaceinfo->iface_count < slp_max_ifaces; ++i)
               if (plist->Address[i].lpSockaddr->sa_family == AF_INET)
                  memcpy(&ifaceinfo->iface_addr[ifaceinfo->iface_count++],
                        plist->Address[i].lpSockaddr, sizeof(struct sockaddr_in));
            xfree(buffer);
         }
         closesocket(fd);
      }
   }
   return 0;
}

#else

/** Get the default network interface addresses for this host.
 *
 * @param[in,out] ifaceinfo - The address of a buffer in which to return
 *    information about the requested interfaces.
 * @param[in] family - A hint indicating the address family to get info
 *    for - can be AF_INET, AF_INET6, or AF_UNSPEC for both. Note: ignored
 *    on this platform.
 *
 * @return Zero on success; A non-zero value (with errno set) on error.
 *    Note: This implementation always fails.
 *
 * @remarks Does NOT return the loopback interface.
 */
int SLPIfaceGetDefaultInfo(SLPIfaceInfo* ifaceinfo, int family)
{
   (void)family;  /* prevents compiler warnings about unused parameters */

   ifaceinfo->bcast_count = 0;
   ifaceinfo->iface_count = 0;

   /* This is the default case, where we don't know how to get the info.
      This platform MUST define the interfaces in slp.conf */

   return -1;
}

#endif

/** Extract the interface name from ip for the format
 * <ip address>%<inetface>
 *
 * @param[in] ip - Ip address with interface name format
 * @param[in,out] iface - Pointer to store the extracted interface info
 *
 * @return Zero on success; -1 on failure
 */
static int SLPD_Get_Iface_From_Ip(char *ip, char *iface)
{
   char *p;

   if (iface == NULL)
      return -1;

   p = strchr(ip, '%');
   if (p == NULL)
      return -1;
   ++p;
   if (strlen(p) >= MAX_IFACE_LEN)
      return -1;
   (void) strcpy(iface, p);
   return 0;
}

/** Get an IP address for an interface
 *
 * Fills in sockaddr with a valid address on the given interface and returns
 * the length of the socket address
 *
 * @param[in] ifaddrs0 - pointer to ifaddrlist, or NULL
 * @param[in] ifname - Pointer to interface name
 * @param[in] family - A hint indicating the address family to get info
 *    for - can be AF_INET, AF_INET6, or AF_UNSPEC for both.
 * @param[out] sa - sockaddr containing an address on interface "ifname"
 *
 * @return Zero on failure, sizeof(sockaddr_in) for AF_INET,
 *     sizeof(sockaddr_in6) for AF_INET6
 */
static int ifname_to_addr(struct ifaddrs *ifaddrs0, char *ifname, int family,
      struct sockaddr *sa)
{
   int salen = 0;
   struct ifaddrs *ifaddrs, *ifa;

   ifaddrs = ifaddrs0;

   if (ifaddrs == NULL && getifaddrs(&ifaddrs) < 0)
      return 0;
   for (ifa = ifaddrs; ifa; ifa = ifa->ifa_next)
   {
      if (strcmp(ifa->ifa_name, ifname) != 0)
         continue;
      if (family != AF_UNSPEC && ifa->ifa_addr->sa_family != family)
         continue;
      switch (ifa->ifa_addr->sa_family)
      {
         case AF_INET:
            salen = sizeof(struct sockaddr_in);
            break;
         case AF_INET6:
            salen = sizeof(struct sockaddr_in6);
            break;
         default:
            continue;
      }
      memcpy(sa, ifa->ifa_addr, salen);
      break;
   }
   if (!ifaddrs0)
      freeifaddrs(ifaddrs);
   return salen;
}

/** Get the network interface addresses for this host.
 *
 * Returns either a complete list or a subset of the list of network interface
 * addresses for this host. If the user specifies a list, then network interfaces
 *
 * @param[in] useifaces - Pointer to comma delimited string of interface
 *    IPv4 addresses to get interface information for. Pass 0 or the empty
 *    string to get all interfaces (except the loopback interface).
 * @param[out] ifaceinfo - The address of a buffer in which to return
 *    information about the requested interfaces.
 * @param[in] family - A hint indicating the address family to get info
 *    for - can be AF_INET, AF_INET6, or AF_UNSPEC for both.
 *
 * @return Zero on success; A non-zero value (with errno set) on error.
 *
 * @remarks Does NOT return the loopback interface.
 */
int SLPIfaceGetInfo(const char * useifaces, SLPIfaceInfo * ifaceinfo,
      int family)
{
   size_t useifaceslen = useifaces? strlen(useifaces): 0;
   int sts = 0;

   ifaceinfo->iface_count = 0;
   ifaceinfo->bcast_count = 0;

   if (!useifaceslen)
   {
      /* no specified list - get all available interface addresses */
      if (SLPIfaceGetDefaultInfo(ifaceinfo, family) != 0)
         return -1;
   }
   else
   {
      /* list specified: parse it and use it */
      /* only allow addresses in configured address set */
      char * p = SLPPropertyXDup("net.slp.interfaces");

      /* If there are no configured addresses, use the passed in addresses*/
      if (p && strlen(p) == 0)
      {
         xfree(p);
         p = xstrdup(useifaces);
      }

      if (p)
      {
         char *token, *saveptr;
         char iface[MAX_IFACE_LEN];
         struct ifaddrs *ifaddrs;
         int ret = 0;

         if (getifaddrs(&ifaddrs) < 0)
            ifaddrs = NULL;

         for (token = strtok_r(p, ",", &saveptr); token;
               token = strtok_r(NULL, ",", &saveptr))
         {
            ret = SLPD_Get_Iface_From_Ip(token, (char *)&iface);

            if (SLPIfaceContainsAddr(useifaceslen, useifaces,
                  strlen(token), token))
            {
               sockfd_t fd;
               struct sockaddr_in v4addr;
               struct sockaddr_in6 v6addr;

               /* check if an ipv4 address was given */
               if (SLP_inet_pton(AF_INET, token, &v4addr.sin_addr) == 1)
               {
                  if (SLPNetIsIPV4() && ((family == AF_INET) || (family == AF_UNSPEC)))
                  {
                     fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                     if (fd != SLP_INVALID_SOCKET)
                     {
                        v4addr.sin_family = AF_INET;
                        v4addr.sin_port = 0;
                        memset(v4addr.sin_zero, 0, sizeof(v4addr.sin_zero));
                        if ((sts = bind(fd, (struct sockaddr *)&v4addr, sizeof(v4addr))) == 0)
                           memcpy(&ifaceinfo->iface_addr[ifaceinfo->iface_count++],
                                 &v4addr, sizeof(v4addr));
                        closesocket(fd);
                     }
                  }
               }
               else if (SLP_inet_pton(AF_INET6, token, &v6addr.sin6_addr) == 1)
               {
                  if (SLPNetIsIPV6() && ((family == AF_INET6) || (family == AF_UNSPEC)))
                  {
                     v6addr.sin6_family = AF_INET6;
                     v6addr.sin6_port = 0;
                     v6addr.sin6_flowinfo = 0;
                     if (!ret)
                     {
                        if ((sts = GetV6Scope(&v6addr, iface)) == 0)
                           memcpy(&ifaceinfo->iface_addr[ifaceinfo->iface_count++], &v6addr, sizeof(v6addr));
                     }
                     else
                     {
                        if ((sts = GetV6Scope(&v6addr, NULL)) == 0)
                           memcpy(&ifaceinfo->iface_addr[ifaceinfo->iface_count++], &v6addr, sizeof(v6addr));
                     }
                  }
               }
               else
                  sts = ((errno = EINVAL), -1); /* not v4, not v6 */
            }
            else if (if_nametoindex(token))
            {
               struct sockaddr_storage ss;
               int salen;

               salen = ifname_to_addr(ifaddrs, token, family,
                                      (struct sockaddr *)&ss);
               if (salen > 0)
                  memcpy(&ifaceinfo->iface_addr[ifaceinfo->iface_count++],
                         &ss, salen);
            }
         }
         if (ifaddrs)
            freeifaddrs(ifaddrs);
      }
      xfree(p);
   }

   /* now stuff in a v4 broadcast address */
   if (SLPNetIsIPV4() && ((family == AF_INET) || (family == AF_UNSPEC)))
   {
      struct sockaddr_storage sa;
      char * str = SLPPropertyXDup("net.slp.broadcastAddr");

      if (!str || !*str)
      {
         unsigned long addr = INADDR_BROADCAST;

         SLPNetSetAddr(&sa, AF_INET, 0, &addr);
         memcpy(&ifaceinfo->bcast_addr[ifaceinfo->bcast_count++], &sa, sizeof(sa));
      }
      else
      {
         unsigned long addr;

         if (inet_pton(AF_INET, str, &addr) == 1)
         {
            SLPNetSetAddr(&sa, AF_INET, 0, &addr);
            memcpy(&ifaceinfo->bcast_addr[ifaceinfo->bcast_count++], &sa, sizeof(sa));
         }
      }
      xfree(str);
   }
   return sts;
}

/** Extract Interface Name from scope id. This function is called by v6 code only.
 *
 * @param[in] scope_id - The scope id of interface
 * @param[in,out] iface - The interface name got from scope id
 *
 * @return Zero on success, -1 on error.
 */
static int SLPGetIfaceNameFromScopeId(unsigned int scope_id, char *iface)
{
#ifdef LINUX
   struct ifaddrs *ifa, *ifaddr;

   if(!scope_id)
        return -1;

   if (getifaddrs(&ifa)) {
        return -1;
   }
   ifaddr = ifa;
   for (; ifa; ifa = ifa->ifa_next)
   {
      /* filter out NULL address interfaces */
      if (!ifa->ifa_addr)
         continue;

      /* filter out non-v6 interfaces */
      if (ifa->ifa_addr->sa_family != AF_INET6)
         continue;

      if (((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_scope_id == scope_id)
      {
         if (strlen(ifa->ifa_name) >= MAX_IFACE_LEN)
         {
            freeifaddrs(ifaddr);
            return -1;
         }
         else
         {
            strcpy(iface, ifa->ifa_name);
            freeifaddrs(ifaddr);
            return 0;
         }
      }
   }
   freeifaddrs(ifaddr);
   return -1;
#elif defined _WIN32
   DWORD retVal = 0;
   int i = 0;
   ULONG flg = GAA_FLAG_INCLUDE_PREFIX;
   ULONG family = AF_UNSPEC;
   PIP_ADAPTER_ADDRESSES pAddr = NULL;
   ULONG outBufLen = 0;
   PIP_ADAPTER_ADDRESSES pCurrAddr = NULL;
   PIP_ADAPTER_UNICAST_ADDRESS pUnicastAddr = NULL;
   family = AF_INET6;

   outBufLen = sizeof (IP_ADAPTER_ADDRESSES);
   if ((pAddr = (IP_ADAPTER_ADDRESSES *)xmalloc(outBufLen)) == 0)
      return (errno = ENOMEM), -1;

   if (GetAdaptersAddresses(family, flg, NULL, pAddr, &outBufLen) == ERROR_BUFFER_OVERFLOW)
   {
      xfree(pAddr);
      if ((pAddr = (IP_ADAPTER_ADDRESSES *)xmalloc(outBufLen)) == 0)
         return (errno = ENOMEM), -1;
   }

   retVal = GetAdaptersAddresses(family, flg, NULL, pAddr, &outBufLen);

   if (retVal == NO_ERROR)
   {
      pCurrAddr = pAddr;
      while (pCurrAddr)
      {
         pUnicastAddr = pCurrAddr->FirstUnicastAddress;
         if (pUnicastAddr != NULL)
         {
            for (i = 0; pUnicastAddr != NULL; i++)
            {
               struct sockaddr_in6 *paddr = (struct sockaddr_in6 *)(pUnicastAddr->Address.lpSockaddr);
               if (paddr->sin6_scope_id == scope_id)
               {
                  wchar_t widestring[128];
                  char stringbuf[128];

                  wcscpy(widestring, pCurrAddr->FriendlyName);
                  wcstombs(stringbuf, widestring, sizeof(stringbuf));
                  memcpy((void *)iface,(void *)stringbuf, strlen((const char *)stringbuf)+1);
                  xfree(pAddr);
                  return 0;
               }
               pUnicastAddr = pUnicastAddr->Next;
            }
         }
         pCurrAddr = pCurrAddr->Next;
      }
   }
   if(pAddr != NULL)
      xfree(pAddr);
   return -1;
#else
   return -1;
#endif
}

/** Convert an array of sockaddr_storage buffers to a comma-delimited list.
 *
 * Converts an array of sockaddr_storage buffers to a comma-delimited list of
 * addresses in presentation (string) format.
 *
 * @param[in] addrs - A pointer to array of sockaddr_storages to convert.
 * @param[in] addrcount - The number of elements in @p addrs.
 * @param[out] addrstr - The address in which to return a pointer to an
 *    allocated comma-delimited list of addresses.
 *
 * @return Zero on success, non-zero (with errno set) on error.
 *
 * @remarks The caller must xfree @p addrstr when no longer needed.
 */
int SLPIfaceSockaddrsToString(struct sockaddr_storage const * addrs,
      int addrcount, char ** addrstr)
{
   int i;
   struct sockaddr *addr;
   struct sockaddr_in6 *addr6;

   SLP_ASSERT(addrs && addrcount && addrstr);
   if (!addrs || !addrcount || !addrstr)
      return (errno = EINVAL), -1;

   /* 40 is the maximum size of a string representation of
    * an IPv6 address (including the comma for the list)
    * 20 is MAX size of interface name
    */
   if ((*addrstr = xmalloc(addrcount * (INET6_ADDRSTRLEN + MAX_IFACE_LEN))) == 0)
      return (errno = ENOMEM), -1;

   **addrstr = 0;

   for (i = 0; i < addrcount; i++)
   {
      char buf[1024] = "";
      char iface[MAX_IFACE_LEN] = "";

      SLPNetSockAddrStorageToString(&addrs[i], buf, sizeof(buf));
      addr = (struct sockaddr *)&addrs[i];
      if (addr->sa_family == AF_INET6)
      {
         addr6 = (struct sockaddr_in6 *)addr;
         if (!SLPGetIfaceNameFromScopeId(addr6->sin6_scope_id, (char *)&iface))
         {
            strcat(*addrstr, buf);
            strcat(*addrstr, "%");
            strcat(*addrstr, iface);
         }
         else
            strcat(*addrstr, buf);
      }
      else
         strcat(*addrstr, buf);

      if (i != addrcount - 1)
         strcat(*addrstr, ",");
   }
   return 0;
}

/** Converts a comma-delimited list of address to address buffers.
 *
 * @param[in] addrstr - The comma-delimited string to convert.
 * @param[out] addrs - The address of a buffer to fill with
 *    sockaddr_storage entries.
 * @param[in,out] addrcount - On entry, contains the number of
 *    sockaddr_storage buffers in @p addrs; on exit, returns the number
 *    of buffers written.
 *
 * @return Zero on success, non-zero (with errno set) on error.
 */
int SLPIfaceStringToSockaddrs(const char * addrstr,
      struct sockaddr_storage * addrs, int * addrcount)
{
   int i;
   char * str;
   char * slider1;
   char * slider2;

#ifdef DEBUG
   if (addrstr == 0 || addrs == 0 || addrcount == 0)
   {
      errno = EINVAL;   /* invalid parameters */
      return 1;
   }
#endif

   str = xstrdup(addrstr);
   if (str == 0)
      return 1;   /* out of memory */

   i = 0;
   slider1 = str;
   while (1)
   {
      slider2 = strchr(slider1, ',');

      /* check for empty string */
      if (slider2 == slider1)
         break;

      /* stomp the comma and null terminate address */
      if (slider2)
         *slider2 = 0;

      /* if it has a dot - try v4 */
      if (strchr(slider1, '.'))
      {
         struct sockaddr_in * d4 = (struct sockaddr_in *)&addrs[i];
         inet_pton(AF_INET, slider1, &d4->sin_addr);
         d4->sin_family = AF_INET;
      }
      else if (strchr(slider1, ':'))
      {
         struct sockaddr_in6 * d6 = (struct sockaddr_in6 *)&addrs[i];
         inet_pton(AF_INET6, slider1, &d6->sin6_addr);
         d6->sin6_family = AF_INET6;
      }
      else
      {
         xfree(str);
         return(-1);
      }
      i++;
      if (i == *addrcount)
         break;

      /* are we done? */
      if (slider2 == 0)
         break;

      slider1 = slider2 + 1;
   }

   *addrcount = i;
   xfree(str);
   return 0;
}

/*===========================================================================
 * TESTING CODE enabled by compiling with the following command line:
 *
 * $ gcc -g -DDEBUG -DSLP_IFACE_TEST slp_iface.c slp_xmalloc.c \
 *    slp_linkedlist.c slp_compare.c slp_debug.c
 */
#ifdef SLP_IFACE_TEST
int main(int argc, char * argv[])
{
   int i;
   int addrscount = 10;
   struct sockaddr_storage addrs[10];
   SLPIfaceInfo ifaceinfo;
   char * addrstr;

#ifdef _WIN32
   WSADATA wsadata;
   WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif

   if ((ifaceinfo.iface_addr = xmalloc(slp_max_ifaces * sizeof(struct sockaddr_storage))) == 0)
   {
      fprintf(stderr, "iface_addr xmalloc(%d) failed\n",
         slp_max_ifaces * sizeof(struct sockaddr_storage));
      exit(1);
   }
   if ((ifaceinfo.bcast_addr = xmalloc(slp_max_ifaces * sizeof(struct sockaddr_storage))) == 0)
   {
      fprintf(stderr, "bcast_addr xmalloc(%d) failed\n",
         slp_max_ifaces * sizeof(struct sockaddr_storage));
      exit(1);
   }

   if (SLPIfaceGetInfo(0, &ifaceinfo, AF_INET) == 0)
      for (i = 0; i < ifaceinfo.iface_count; i++)
      {
         char myname[MAX_HOST_NAME];

         SLPNetSockAddrStorageToString(&ifaceinfo.iface_addr[i], myname,
               sizeof(myname));
         printf("v4 found iface = %s\n", myname);
         SLPNetSockAddrStorageToString(&ifaceinfo.bcast_addr[i], myname,
               sizeof(myname));
         printf("v4 bcast addr = %s\n", myname);
      }

   if (SLPIfaceGetInfo(0, &ifaceinfo, AF_INET6) == 0)
   {
      for (i = 0; i < ifaceinfo.iface_count; i++)
      {
         char myname[MAX_HOST_NAME];

         SLPNetSockAddrStorageToString(&ifaceinfo.iface_addr[i], myname,
               sizeof(myname));
         printf("v6 found iface = %s\n", myname);
      }
      for (i = 0; i < ifaceinfo.bcast_count; i++)
      {
         char myname[MAX_HOST_NAME];

         SLPNetSockAddrStorageToString(&ifaceinfo.bcast_addr[i], myname,
               sizeof(myname));
         printf("v6 bcast addr = %s\n", myname);
      }
   }

   SLPIfaceGetInfo("fec0:0:0:0001:0:0:0:3,5:6::7,10.0.25.82", &ifaceinfo,
         AF_INET6);
   SLPIfaceGetInfo("fec0:0:0:0001:0:0:0:3,5:6::7,10.0.25.82", &ifaceinfo,
         AF_INET);
   if (SLPIfaceStringToSockaddrs("192.168.100.1,192.168.101.1", addrs,
            &addrscount) == 0)
      if (SLPIfaceSockaddrsToString(addrs, addrscount, &addrstr) == 0)
      {
         printf("sock addr string v4 = %s\n", addrstr);
         xfree(addrstr);
      }

   if (SLPIfaceStringToSockaddrs("1:2:0:0:0::4,10:0:0:0:0:0:0:11", addrs,
            &addrscount) == 0)
      if (SLPIfaceSockaddrsToString(addrs, addrscount, &addrstr) == 0)
      {
         printf("sock addr string v6 = %s\n", addrstr);
         xfree(addrstr);
      }
   xfree(ifaceinfo.iface_addr);
   xfree(ifaceinfo.bcast_addr);

#ifdef _WIN32
   WSACleanup();
#endif
}
#endif

/*=========================================================================*/
