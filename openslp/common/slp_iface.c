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
 * @ingroup    CommonCode
 */

#include "slp_iface.h"
#include "slp_xmalloc.h"
#include "slp_compare.h"
#include "slp_net.h"
#include "slp_property.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef SOLARIS
# include <sys/sockio.h>
#endif

#ifndef _WIN32
# include <sys/ioctl.h>
# include <net/if.h>
# include <arpa/inet.h>
#else
# ifndef UINT32_T_DEFINED
#  define UINT32_T_DEFINED
typedef unsigned int uint32_t;
# endif
#endif

/* the max index for v6 address to test for valid scope ids */
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
int SLPIfaceContainsAddr(int listlen, const char * list, int stringlen,
      const char * string)
{
   char * listend = (char *)list + listlen;
   char * itembegin = (char *)list;
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

      if (itemend-itembegin < sizeof(buffer))
         buffer_len = itemend-itembegin;
      else
         buffer_len = sizeof(buffer);
      strncpy(buffer, itembegin, buffer_len);
      buffer[itemend-itembegin] = '\0';
      if (SLPNetIsIPV6() && inet_pton(AF_INET6, buffer, &addr) == 1)
      {
         inet_ntop(AF_INET6, &addr, buffer, sizeof(buffer));
         if (SLPCompareString(strlen(buffer), buffer, stringlen,
               string) == 0)
            return 1;
      }
      else if (SLPNetIsIPV4() && inet_pton(AF_INET, buffer, &addr) == 1)
      {
         inet_ntop(AF_INET, &addr, buffer, sizeof(buffer));
         if (SLPCompareString(strlen(buffer), buffer, stringlen,
               string) == 0)
            return 1;
      }
      itemend ++;
   }
   return 0;
}

/** Get the network interface addresses for this host.
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
   char * interfaceString;
   char * bcastString;
   int sts = 0;
   int useifaceslen;
   struct sockaddr_in v4addr;
   long hostAddr;
   struct sockaddr_in bcastAddr;
   struct sockaddr_in6 v6addr;
   struct sockaddr_in6 indexHack;
   unsigned int fd;
   int i;

   /* first try ipv6 addrs */
   ifaceinfo->iface_count = 0;
   ifaceinfo->bcast_count = 0;

   if (useifaces)
      useifaceslen = strlen(useifaces);
   else
      useifaceslen = 0;

   /* attempt to retrieve the interfaces from the configuration file */
   interfaceString = (char *)SLPPropertyGet("net.slp.interfaces");
   if (*interfaceString == 0)
   {
      /* They don't have any setting in their conf file, 
       * so put in the default info 
       */
      if (SLPNetIsIPV6() && ((family == AF_INET6) || (family == AF_UNSPEC)))
      {
         struct sockaddr_storage storageaddr_any;

         fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
         if (fd != -1)
         {
            for (i = 0; i < MAX_INTERFACE_TEST_INDEX; i++)
            {
               SLPNetSetAddr((struct sockaddr_storage *) &indexHack, AF_INET6, 
                     0, (char *)&slp_in6addr_any, sizeof(slp_in6addr_any));
               indexHack.sin6_scope_id = i;
               sts = bind(fd, (struct sockaddr *)&indexHack, sizeof(indexHack));
               if (sts == 0)
               {
                  SLPNetSetAddr(&storageaddr_any, AF_INET6, 0, 
                        (char *)&slp_in6addr_any, sizeof(slp_in6addr_any));
                  SLPNetCopyAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count], 
                        &storageaddr_any);
                  ifaceinfo->iface_count++;
                  break;
               }
            }
#ifdef _WIN32 
            closesocket(fd);
#else
            close(fd);
#endif
         }
      }
      if (SLPNetIsIPV4() && ((family == AF_INET) || (family == AF_UNSPEC)))
      {
         struct sockaddr_storage storageaddr_any;
         unsigned int addrAny = INADDR_ANY;

         fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
         if (fd != -1)
         {
            v4addr.sin_family = AF_INET;
            v4addr.sin_port = 0;
            v4addr.sin_addr.s_addr = addrAny;
            sts = bind(fd, (struct sockaddr *)&v4addr, sizeof(v4addr));
            if (sts == 0)
            {
               SLPNetSetAddr(&storageaddr_any, AF_INET, 0, 
                     (char *)&addrAny, sizeof(addrAny));
               SLPNetCopyAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count], 
                     &storageaddr_any);
               ifaceinfo->iface_count++;
            }
#ifdef _WIN32 
            closesocket(fd);
#else
            close(fd);
#endif
         }
      }
   }
   else
   {
      char * slider1, * slider2, * temp, * tempend;

      /* attemp to use the settings from the file */
      slider1 = slider2 = temp = xstrdup(SLPPropertyGet("net.slp.interfaces"));
      if (temp)
      {
         tempend = temp + strlen(temp);
         while (slider1 != tempend)
         {
            while (*slider2 && *slider2 != ',') 
               slider2++;
            *slider2 = 0;
            if (*slider1 == '\0')
               slider1++;

            /* Should have slider1 pointing to a 0 terminated string 
             * for the ip address 
             */
            if (SLPIfaceContainsAddr(useifaceslen, useifaces, 
                  strlen(slider1), slider1))
            {
               /* check if an ipv4 address was given */
               if (inet_pton(AF_INET, slider1, &v4addr.sin_addr) == 1)
               {
                  if (SLPNetIsIPV4() && ((family == AF_INET) 
                        || (family == AF_UNSPEC)))
                  {
                     fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                     if (fd != -1)
                     {
                        v4addr.sin_family = AF_INET;
                        v4addr.sin_port = 0;
                        sts = bind(fd, (struct sockaddr *)&v4addr, 
                              sizeof(v4addr));
                        if (sts == 0)
                        {
                           hostAddr = ntohl(v4addr.sin_addr.s_addr);
                           SLPNetSetAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count], 
                                 AF_INET, 0, (char *) &hostAddr, sizeof(v4addr.sin_addr));
                           ifaceinfo->iface_count++;
                        }
#ifdef _WIN32 
                        closesocket(fd);
#else
                        close(fd);
#endif
                     }
                  }
               }
               else if (inet_pton(AF_INET6, slider1, &v6addr.sin6_addr) == 1)
               {
                  if (SLPNetIsIPV6() && ((family == AF_INET6) 
                        || (family == AF_UNSPEC)))
                  {
                     /* try and bind to verify the address is okay */
                     fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
                     if (fd != -1)
                     {
                        for (i = 0; i < MAX_INTERFACE_TEST_INDEX; i++)
                        {
                           SLPNetSetAddr((struct sockaddr_storage *)&indexHack, 
                                 AF_INET6, 0, (char *) &v6addr.sin6_addr, 
                                 sizeof(v6addr.sin6_addr));
                           indexHack.sin6_scope_id = i;
                           sts = bind(fd, (struct sockaddr *) &indexHack, 
                                 sizeof(indexHack));
                           if (sts == 0)
                           {
                              memcpy(&ifaceinfo->iface_addr[ifaceinfo->iface_count], 
                                    &indexHack, sizeof(indexHack));
                              ifaceinfo->iface_count++;
                              break;
                           }
                        }
#ifdef _WIN32 
                        closesocket(fd);
#else
                        close(fd);
#endif
                     }
                  }
               }
               else
               {
                  /* not v4, not v6 */
                  errno = EINVAL;
                  sts = 1;
               }
            }
            slider1 = slider2;
            slider2++;
         }
      }
      xfree(temp);
   }

   /* now stuff in a broadcast address */
   if (SLPNetIsIPV4() && ((family == AF_INET) || (family == AF_UNSPEC)))
   {
      bcastString = (char *)SLPPropertyGet("net.slp.broadcastAddr");
      if (*bcastString == 0)
      {
         struct sockaddr_storage storageaddr_bcast;
         unsigned int broadAddr = INADDR_BROADCAST;

         SLPNetSetAddr(&storageaddr_bcast, AF_INET, 0, (char *)&broadAddr, 
               sizeof(broadAddr));
         SLPNetCopyAddr(&ifaceinfo->bcast_addr[ifaceinfo->bcast_count], 
               &storageaddr_bcast);
         ifaceinfo->bcast_count++;
      }
      else
      {
         struct sockaddr_storage storageaddr_bcast;

         if (inet_pton(AF_INET, bcastString, &bcastAddr.sin_addr) == 1)
         {
            SLPNetSetAddr(&storageaddr_bcast, AF_INET, 0, 
                  (char *)&bcastAddr.sin_addr, sizeof(bcastAddr.sin_addr));
            SLPNetCopyAddr(&ifaceinfo->bcast_addr[ifaceinfo->bcast_count], 
                  &storageaddr_bcast);
            ifaceinfo->bcast_count++;
         }
      }
   }
   return sts;
}

/** Convert an array of sockaddr_storage buffers to a comma-delimited list.
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
int SLPIfaceSockaddrsToString(const struct sockaddr_storage * addrs,
      int addrcount, char ** addrstr)
{
   int i;

#ifdef DEBUG
   if (addrs == 0 || addrcount == 0 || addrstr == 0)
   {
      /* invalid paramaters */
      errno = EINVAL;
      return 1;
   }
#endif

   /* 40 is the maximum size of a string representation of
    * an IPv6 address (including the comman for the list)
    */
   *addrstr = (char *)xmalloc(addrcount * 40);
   *addrstr[0] = 0;

   for (i = 0; i < addrcount; i++)
   {
      char buf[1024];

      buf[0]= 0;

      SLPNetSockAddrStorageToString((struct sockaddr_storage *)&addrs[i], 
            buf, sizeof(buf));
      strcat(*addrstr, buf);
      if (i != addrcount-1)
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
      /* invalid parameters */
      errno = EINVAL;
      return 1;
   }
#endif

   str = xstrdup(addrstr);
   if (str == 0)
      return 1;

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
         struct sockaddr_in *d4 = (struct sockaddr_in *) &addrs[i];
         inet_pton(AF_INET, slider1, &d4->sin_addr);
         d4->sin_family = AF_INET;
      }
      else if (strchr(slider1, ':'))
      {
         struct sockaddr_in6 *d6 = (struct sockaddr_in6 *) &addrs[i];
         inet_pton(AF_INET6, slider1, &d6->sin6_addr);
         d6->sin6_family = AF_INET6;
      }
      else
         return -1;

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
 * TESTING CODE enabled by removing #define comment and compiling with the 
 * following command line:
 *
 * $ gcc -g -DDEBUG slp_iface.c slp_xmalloc.c slp_linkedlist.c slp_compare.c
 */
/* #define SLP_IFACE_TEST  */
#ifdef SLP_IFACE_TEST 
int main(int argc, char * argv[])
{
   int i;
   int addrscount =  10;
   struct sockaddr_storage addrs[10];
   SLPIfaceInfo ifaceinfo;
   char * addrstr;

#ifdef _WIN32
   WSADATA wsadata;
   WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif

   if (SLPIfaceGetInfo(0, &ifaceinfo, AF_INET) == 0)
   {
      for (i=0;i<ifaceinfo.iface_count;i++)
      {
         char myname[MAX_HOST_NAME];
         SLPNetSockAddrStorageToString(&ifaceinfo.iface_addr[i], myname, sizeof(myname));
         printf("v4 found iface = %s\n", myname);
         SLPNetSockAddrStorageToString(&ifaceinfo.bcast_addr[i], myname, sizeof(myname));
         printf("v4 bcast addr = %s\n", myname);
      }
   }

   if (SLPIfaceGetInfo(0,&ifaceinfo, AF_INET6) == 0)
   {
      for (i=0;i<ifaceinfo.iface_count;i++)
      {
         char myname[MAX_HOST_NAME];
         SLPNetSockAddrStorageToString(&ifaceinfo.iface_addr[i], myname, sizeof(myname));
         printf("v6 found iface = %s\n", myname);
      }
      for (i=0;i<ifaceinfo.bcast_count;i++)
      {
         char myname[MAX_HOST_NAME];
         SLPNetSockAddrStorageToString(&ifaceinfo.bcast_addr[i], myname, sizeof(myname));
         printf("v6 bcast addr = %s\n", myname);
      }
   }


   SLPIfaceGetInfo("fec0:0:0:0001:0:0:0:3,5:6::7,10.0.25.82", &ifaceinfo, AF_INET6);
   SLPIfaceGetInfo("fec0:0:0:0001:0:0:0:3,5:6::7,10.0.25.82", &ifaceinfo, AF_INET);
   if (SLPIfaceStringToSockaddrs("192.168.100.1,192.168.101.1",
         addrs,
         &addrscount) == 0)
   {
      if (SLPIfaceSockaddrsToString(addrs, 
            addrscount,
            &addrstr) == 0)
      {
         printf("sock addr string v4 = %s\n",addrstr);
         xfree(addrstr);
      }
   }

   if (SLPIfaceStringToSockaddrs("1:2:0:0:0::4,10:0:0:0:0:0:0:11",
         addrs,
         &addrscount) == 0)
   {
      if (SLPIfaceSockaddrsToString(addrs, 
            addrscount,
            &addrstr) == 0)
      {
         printf("sock addr string v6 = %s\n",addrstr);
         xfree(addrstr);
      }
   }

#ifdef _WIN32
   WSACleanup();
#endif

}
#endif

/*=========================================================================*/
