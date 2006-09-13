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
   char buffer[INET6_ADDRSTRLEN]; /* must be at least 40 characters */
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
      if (SLPNetIsIPV6() && inet_pton(AF_INET6, buffer, &addr) == 1)
      {
         inet_ntop(AF_INET6, &addr, buffer, sizeof(buffer));
         if (SLPCompareString(strlen(buffer), buffer, stringlen, string) == 0)
            return 1;
      }
      else if (SLPNetIsIPV4() && inet_pton(AF_INET, buffer, &addr) == 1)
      {
         inet_ntop(AF_INET, &addr, buffer, sizeof(buffer));
         if (SLPCompareString(strlen(buffer), buffer, stringlen, string) == 0)
            return 1;
      }
      itemend ++;
   }
   return 0;
}

/** @todo: evaluate network interface discovery on AIX, Solaris, and Hpux */

#if defined(LINUX) || defined(AIX) || defined(SOLARIS) || defined(HPUX)

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
   int i;
   sockfd_t fd;
   struct sockaddr * sa;
   struct ifreq ifrlist[SLP_MAX_IFACES];
   struct ifreq ifrflags;

   struct ifconf ifc;
   ifc.ifc_len = sizeof(struct ifreq) * SLP_MAX_IFACES;
   ifc.ifc_req = ifrlist;

   if ((family == AF_INET6) || (family == AF_UNSPEC))
   {
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
            return -1;
         }

         for (i = 0; i < ifc.ifc_len/sizeof(struct ifreq); i++)
         {
            sa = (struct sockaddr *)&(ifrlist[i].ifr_addr);
            if (sa->sa_family == AF_INET6)
            {
               /* get interface flags, skip loopback addrs */
               memcpy(&ifrflags, &(ifrlist[i]), sizeof(struct ifreq));
               if (ioctl(fd, SIOCGIFFLAGS, &ifrflags) == 0
                     && (ifrflags.ifr_flags & IFF_LOOPBACK) == 0)
                  memcpy(&ifaceinfo->iface_addr[ifaceinfo->iface_count++], 
                        sa, sizeof(struct sockaddr_in6));
            }
         }
         closesocket(fd);
      }
   }

   /* reset ifc_len for next get */
   ifc.ifc_len = sizeof(struct ifreq) * SLP_MAX_IFACES;

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
            return -1;
         }

         for (i = 0; i < ifc.ifc_len/sizeof(struct ifreq); i++)
         {
            sa = (struct sockaddr *)&(ifrlist[i].ifr_addr);
            if (sa->sa_family == AF_INET)
            {
               /* get interface flags, skip loopback addrs */
               memcpy(&ifrflags, &(ifrlist[i]), sizeof(struct ifreq));
               if (ioctl(fd, SIOCGIFFLAGS, &ifrflags) == 0
                     && (ifrflags.ifr_flags & IFF_LOOPBACK) == 0)
                  memcpy(&ifaceinfo->iface_addr[ifaceinfo->iface_count++], 
                        sa, sizeof(struct sockaddr_in));
            }
         }
         closesocket(fd);
      }
   }
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

      fd = socket(AF_INET6, SOCK_DGRAM, 0);
      if (fd != INVALID_SOCKET)
      {
         /* We want to get a reasonable length buffer, so call empty first */
         if (WSAIoctl(fd, SIO_ADDRESS_LIST_QUERY, 0, 0, 
               buffer, buflen, &buflen, 0, 0) != 0)
            return (errno = WSAGetLastError()), -1;
         if (buflen > 0)
         {
            if ((buffer = xmalloc(buflen)) == 0)
               return (errno = ENOMEM), -1;
            if (WSAIoctl(fd, SIO_ADDRESS_LIST_QUERY, 0, 0, 
                  buffer, buflen, &buflen, 0, 0) != 0)
            {
               xfree(buffer);
               return (errno = WSAGetLastError()), -1;
            }
            plist = (SOCKET_ADDRESS_LIST*)buffer;
            for (i = 0; i < plist->iAddressCount; ++i)
               if (plist->Address[i].lpSockaddr->sa_family == AF_INET6)
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

      fd = socket(AF_INET, SOCK_DGRAM, 0);
      if (fd != INVALID_SOCKET)
      {
         /* We want to get a reasonable length buffer, so call empty first */
         if (WSAIoctl(fd, SIO_ADDRESS_LIST_QUERY, 0, 0, 
               buffer, buflen, &buflen, 0, 0) != 0)
            return (errno = WSAGetLastError()), -1;
         if (buflen > 0)
         {
            if ((buffer = xmalloc(buflen)) == 0)
               return (errno = ENOMEM), -1;
            if (WSAIoctl(fd, SIO_ADDRESS_LIST_QUERY, 0, 0, 
                  buffer, buflen, &buflen, 0, 0) != 0)
            {
               xfree(buffer);
               return (errno = WSAGetLastError()), -1;
            }
            plist = (SOCKET_ADDRESS_LIST*)buffer;
            for (i = 0; i < plist->iAddressCount; ++i)
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

      if (p)
      {
         char * ep = p + strlen(p);
         char * slider1 = p;
         char * slider2 = p;

         while (slider1 < ep)
         {
            while (*slider2 != 0 && *slider2 != ',') 
               slider2++;
            *slider2 = 0;

            if (SLPIfaceContainsAddr(useifaceslen, useifaces, 
                  strlen(slider1), slider1))
            {
               sockfd_t fd;
               struct sockaddr_in v4addr;
               struct sockaddr_in6 v6addr;

               /* check if an ipv4 address was given */
               if (inet_pton(AF_INET, slider1, &v4addr.sin_addr) == 1)
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
               else if (inet_pton(AF_INET6, slider1, &v6addr.sin6_addr) == 1)
               {
                  if (SLPNetIsIPV6() && ((family == AF_INET6) || (family == AF_UNSPEC)))
                  {
                     /* try and bind to verify the address is okay */
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
                           v6addr.sin6_scope_id = i;
                           if ((sts = bind(fd, (struct sockaddr *)&v6addr, sizeof(v6addr))) == 0)
                           {
                              memcpy(&ifaceinfo->iface_addr[ifaceinfo->iface_count++],
                                    &v6addr, sizeof(v6addr));
                              break;
                           }
                        }
                        closesocket(fd);
                     }
                  }
               }
               else
                  sts = (errno = EINVAL), -1;   /* not v4, not v6 */
            }
            slider1 = ++slider2;
         }
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
 * @remarks The caller must free @p addrstr when no longer needed.
 */
int SLPIfaceSockaddrsToString(struct sockaddr_storage const * addrs,
      int addrcount, char ** addrstr)
{
   int i;

   SLP_ASSERT(addrs && addrcount && addrstr);
   if (!addrs || !addrcount || !addrstr)
      return (errno = EINVAL), -1;

   /* 40 is the maximum size of a string representation of
    * an IPv6 address (including the comma for the list) 
    */
   if ((*addrstr = xmalloc(addrcount * 40)) == 0)
      return (errno = ENOMEM), -1;

   **addrstr = 0;

   for (i = 0; i < addrcount; i++)
   {
      char buf[1024] = "";

      SLPNetSockAddrStorageToString(&addrs[i], buf, sizeof(buf));

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
         return(-1);
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

#ifdef _WIN32
   WSACleanup();
#endif
}
#endif

/*=========================================================================*/
