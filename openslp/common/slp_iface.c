/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_iface.c                                                */
/*                                                                         */
/* Abstract:    Common code to obtain network interface information        */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*     Please submit patches to http://www.openslp.org                     */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/* Copyright (C) 2000 Caldera Systems, Inc                                 */
/* All rights reserved.                                                    */
/*                                                                         */
/* Redistribution and use in source and binary forms, with or without      */
/* modification, are permitted provided that the following conditions are  */
/* met:                                                                    */ 
/*                                                                         */
/*      Redistributions of source code must retain the above copyright     */
/*      notice, this list of conditions and the following disclaimer.      */
/*                                                                         */
/*      Redistributions in binary form must reproduce the above copyright  */
/*      notice, this list of conditions and the following disclaimer in    */
/*      the documentation and/or other materials provided with the         */
/*      distribution.                                                      */
/*                                                                         */
/*      Neither the name of Caldera Systems nor the names of its           */
/*      contributors may be used to endorse or promote products derived    */
/*      from this software without specific prior written permission.      */
/*                                                                         */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS     */
/* `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT      */
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   */
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CALDERA      */
/* SYSTEMS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, */
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON       */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    */
/*                                                                         */
/***************************************************************************/

#include "slp_iface.h"
#include "slp_xmalloc.h"
#include "slp_compare.h"
#include "slp_net.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef SOLARIS
#include <sys/sockio.h>
#endif

#ifndef _WIN32
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#else
#ifndef UINT32_T_DEFINED
#define UINT32_T_DEFINED
typedef unsigned int uint32_t; 
#endif
#endif

/* STILL NEED LINUX VERSION OF THIS FUNCTION!!! */
#ifdef _WIN32
/*=========================================================================*/
int SLPIfaceGetInfo(const char* useifaces,
                    SLPIfaceInfo* ifaceinfo, int family)
/* Description:
 *    Get the network interface addresses for this host.  Exclude the
 *    loopback interface
 *
 * Parameters:
 *     useifaces (IN) Pointer to comma delimited string of interface IPv4
 *                    addresses to get interface information for.  Pass
 *                    NULL or empty string to get all interfaces (except 
 *                    loopback)
 *     ifaceinfo (OUT) Information about requested interfaces.
 *     family    (IN) Hint for family to get info for - can be AF_INET, AF_INET6, 
 *                    or AF_UNSPEC for both
 *     scope     (IN) For IPV6 this specifies which scope to get an address for,
 *                this can be SCOPE_GLO
 *
 * Returns:
 *     zero on success, non-zero (with errno set) on error.
 *=========================================================================*/
{
    /*---------------------------------------------------*/
    /* Use gethostbyname(). Not necessarily the best way */
    /*---------------------------------------------------*/
    /* for windows only - but may work in linux, will try during linux development */
    char hostName[MAX_HOST_NAME];
    int sts = 0;
    int useifacesLen;
    IP_ADAPTER_ADDRESSES addresses[SLP_MAX_IFACES];
    IP_ADAPTER_ADDRESSES *networkInterface;
    DWORD addressesLen;
    IP_ADAPTER_UNICAST_ADDRESS *ipaddress;

    addressesLen = sizeof(addresses);
    if (useifaces) {
        useifacesLen = strlen(useifaces);
    }
    else {
        useifacesLen = 0;
    }
    sts = GetAdaptersAddresses(family,
            //GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_DNS_SERVER,
			0,
            NULL,
            addresses,
            &addressesLen);
	if (sts != ERROR_SUCCESS)
	{
		return(-1);
	}
	// Display interface information
    ifaceinfo->iface_count = 0;
    ifaceinfo->bcast_count = 0;
    networkInterface = addresses;
    while (networkInterface != NULL) {
        if ((networkInterface->IfType != IF_TYPE_TUNNEL) && (networkInterface->IfType != IF_TYPE_SOFTWARE_LOOPBACK)) {
            ipaddress = networkInterface->FirstUnicastAddress;
            while (ipaddress != NULL) {
                SLPNetSockAddrStorageToString((struct sockaddr_storage *) ipaddress->Address.lpSockaddr, hostName, sizeof(hostName));
                if ((useifacesLen == 0) || SLPContainsStringList(useifacesLen, useifaces, strlen(hostName), hostName)) {
                        if ((ipaddress->Address.lpSockaddr->sa_family == AF_INET) || (ipaddress->Address.lpSockaddr->sa_family == AF_INET6)) {
                        SLPNetCopyAddr(&ifaceinfo->iface_addr[ifaceinfo->iface_count], (struct sockaddr_storage *) ipaddress->Address.lpSockaddr);
                        ifaceinfo->iface_count++;
                        if (ipaddress->Address.lpSockaddr->sa_family == AF_INET) {  // no broadcast in ipv6
                            struct sockaddr_in *d4Src;
                            DWORD bcastaddr = INADDR_BROADCAST;

                            SLPNetCopyAddr(&ifaceinfo->bcast_addr[ifaceinfo->bcast_count], (struct sockaddr_storage *) ipaddress->Address.lpSockaddr);
                            d4Src = (struct sockaddr_in *)&ifaceinfo->bcast_addr[ifaceinfo->bcast_count];
                            memcpy(&d4Src->sin_addr, &bcastaddr, 4);
                            ifaceinfo->bcast_count++;
                        }
                    }
                }
                ipaddress = ipaddress->Next;
            }
        }
        networkInterface = networkInterface->Next;
    }
    return(sts);
}
#endif
/*=========================================================================*/
int SLPIfaceSockaddrsToString(const struct sockaddr_storage* addrs,
                              int addrcount,
                              char** addrstr)
/* Description:
 *    Get the comma delimited string of addresses from an array of sockaddr_storages
 *
 * Parameters:
 *     addrs (IN) Pointer to array of sockaddr_storages to convert
 *     addrcount (IN) Number of sockaddr_storages in addrs.
 *     addrstr (OUT) pointer to receive malloc() allocated address string.
 *                   Caller must free() addrstr when no longer needed.
 *
 * Returns:
 *     zero on success, non-zero (with errno set) on error.
 *=========================================================================*/
{
    int i;
    
    #ifdef DEBUG
    if(addrs == NULL ||
       addrcount == 0 ||
       addrstr == NULL)
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
    
    for (i=0;i<addrcount;i++)
    {
        char buf[1024];

        buf[0]= '/0';

        SLPNetSockAddrStorageToString((struct sockaddr_storage *) (&addrs[i]), buf, sizeof(buf));
        strcat(*addrstr, buf);
        if (i != addrcount-1)
        {
            strcat(*addrstr,",");
        }
    }

    return 0;
}  


/*=========================================================================*/
int SLPIfaceStringToSockaddrs(const char* addrstr,
                              struct sockaddr_storage* addrs,
                              int* addrcount)
/* Description:
 *    Fill an array of struct sockaddr_storages from the comma delimited string of
 *    addresses.
 *
 * Parameters:
 *     addrstr (IN) Address string to convert.
 *     addrcount (OUT) sockaddr_storage array to fill.
 *     addrcount (INOUT) The number of sockaddr_storage stuctures in the addr array
 *                       on successful return will contain the number of
 *                       sockaddr_storages that were filled in the addr array
 *
 * Returns:
 *     zero on success, non-zero (with errno set) on error.
 *=========================================================================*/
{
    int i;
    char* str;
    char* slider1;
    char* slider2;

    #ifdef DEBUG
    if(addrstr == NULL ||
       addrs == NULL ||
       addrcount == 0)
    {
        /* invalid parameters */
        errno = EINVAL;
        return 1;
    }
    #endif

    str = xstrdup(addrstr);
    if(str == NULL)
    {
        /* out of memory */
        return 1;
    }
    
    i=0;
    slider1 = str;
    while(1)
    {
        slider2 = strchr(slider1,',');
        
        /* check for empty string */
        if(slider2 == slider1)
        {
            break;
        }

        /* stomp the comma and null terminate address */
        if(slider2)
        {
            *slider2 = 0;
        }
        // if it has a dot - try v4
        if (strchr(slider1, '.')) {
            struct sockaddr_in *d4 = (struct sockaddr_in *) &addrs[i];
            inet_pton(AF_INET, slider1, &d4->sin_addr);
            d4->sin_family = AF_INET;
        }
        else if (strchr(slider1, ':')) {
            struct sockaddr_in6 *d6 = (struct sockaddr_in6 *) &addrs[i];
            inet_pton(AF_INET6, slider1, &d6->sin6_addr);
            d6->sin6_family = AF_INET6;
        }
        else {
            return(-1);
        }
        i++;
        if(i == *addrcount)
        {
            break;
        }

        /* are we done? */
        if(slider2 == 0)
        {
            break;
        }

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
 *==========================================================================*/
/* #define SLP_IFACE_TEST */
#ifdef SLP_IFACE_TEST 
int main(int argc, char* argv[])
{
    int i;
    int addrscount =  10;
    struct sockaddr_storage addrs[10];
    SLPIfaceInfo ifaceinfo;
    char* addrstr;

#ifdef _WIN32
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2,2), &wsadata);
#endif

    if(SLPIfaceGetInfo(NULL,&ifaceinfo, AF_INET) == 0)
    {
        for(i=0;i<ifaceinfo.iface_count;i++)
        {
            char myname[MAX_HOST_NAME];


            SLPNetSockAddrStorageToString(&ifaceinfo.iface_addr[i], myname, sizeof(myname));
            printf("v4 found iface = %s\n", myname);
            SLPNetSockAddrStorageToString(&ifaceinfo.bcast_addr[i], myname, sizeof(myname));
            printf("v4 bcast addr = %s\n", myname);
        }
    }

    if(SLPIfaceGetInfo(NULL,&ifaceinfo, AF_INET6) == 0)
    {
        for(i=0;i<ifaceinfo.iface_count;i++)
        {
            char myname[MAX_HOST_NAME];


            SLPNetSockAddrStorageToString(&ifaceinfo.iface_addr[i], myname, sizeof(myname));
            printf("v6 found iface = %s\n", myname);
        }
        for(i=0;i<ifaceinfo.bcast_count;i++)
        {
            char myname[MAX_HOST_NAME];
            SLPNetSockAddrStorageToString(&ifaceinfo.bcast_addr[i], myname, sizeof(myname));
            printf("v6 bcast addr = %s\n", myname);
        }
    }


    SLPIfaceGetInfo("fec0:0:0:0001:0:0:0:3,5:6::7,10.0.25.82", &ifaceinfo, AF_INET6);
    SLPIfaceGetInfo("fec0:0:0:0001:0:0:0:3,5:6::7,10.0.25.82", &ifaceinfo, AF_INET);
    if(SLPIfaceStringToSockaddrs("192.168.100.1,192.168.101.1",
                                 addrs,
                                 &addrscount) == 0)
    {
        if(SLPIfaceSockaddrsToString(addrs, 
                                         addrscount,
                                         &addrstr) == 0)
        {
            printf("sock addr string v4 = %s\n",addrstr);
            xfree(addrstr);
        }
    }

    if(SLPIfaceStringToSockaddrs("1:2:0:0:0::4,10:0:0:0:0:0:0:11",
                                 addrs,
                                 &addrscount) == 0)
    {
        if(SLPIfaceSockaddrsToString(addrs, 
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


