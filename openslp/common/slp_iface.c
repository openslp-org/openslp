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

#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <arpa/inet.h>




/*=========================================================================*/
int SLPInterfaceGetInformation(const char* useifaces,
                               SLPInterfaceInfo* ifaceinfo)
/* Description:
 *    Get the network interface addresses for this host.  Exclude the
 *    loopback interface
 *
 * Parameters:
 *     useifaces (IN) Pointer to comma delimited string of interface IPv4
 *                    addresses to get interface information for.  Pass
 *                    NULL to get all interfaces (excluding NULL)..
 *     ifaceinfo (OUT) Information about requested interfaces.
 *
 * Returns:
 *     zero on success, non-zero (with errno set) on error.
 *=========================================================================*/
{
    struct sockaddr* sa;
    struct sockaddr_in* sin;
    struct ifreq ifrlist[SLP_MAX_IFACES];
    struct ifreq ifrflags;
    struct ifconf ifc;
    int fd;
    int i;
    int useifaceslen;

    #ifdef DEBUG
    if(ifaceinfo == NULL )
    {
        errno = EINVAL;
        /* bad parameters */
        return 1;
    }
    #endif


    ifc.ifc_len = sizeof(struct ifreq) * SLP_MAX_IFACES ;
    ifc.ifc_req = ifrlist;
    
    fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd == -1)
    {
        /* failed to create socket */
        #ifdef DEBUG
        fprintf(stderr,"%s:%i Failed to created socket\n",__FILE__,__LINE__);
        #endif
        return 1;
    }

    if (ioctl(fd,SIOCGIFCONF,&ifc) == -1)
    {
        perror("ioctl failed");
        return 1;
    }

    if(useifaces)
    {
        useifaceslen = strlen(useifaces);
    }
    memset(ifaceinfo,0,sizeof(SLPInterfaceInfo));
    for (i = 0; i < ifc.ifc_len/sizeof(struct ifreq); i++)
    {
        sa = (struct sockaddr *)&(ifrlist[i].ifr_addr);
        if(sa->sa_family == AF_INET)
        {
            /* Get interface flags */
            memcpy(&ifrflags,&(ifrlist[i]),sizeof(struct ifreq));
            if(ioctl(fd,SIOCGIFFLAGS, &ifrflags) == 0)
            {
                /* skip the loopback interfaces */
                if((ifrflags.ifr_flags & IFF_LOOPBACK) == 0)
                {
                    /* Only include those interfaces in the requested list */
                    sin = (struct sockaddr_in*)sa;
                    if(useifaces == NULL ||
                       SLPContainsStringList(useifaceslen,
                                             useifaces,
                                             strlen(inet_ntoa(sin->sin_addr)),
                                             inet_ntoa(sin->sin_addr)))
                    {
                        memcpy(&(ifaceinfo->iface_addr[ifaceinfo->iface_count]),
                               sin,
                               sizeof(struct sockaddr_in));
                    
                        if(ioctl(fd,SIOCGIFBRDADDR,&(ifrlist[i])) == 0)
                        {
                            sin = (struct sockaddr_in *)&(ifrlist[i].ifr_broadaddr);
                            mempcpy(&(ifaceinfo->bcast_addr[ifaceinfo->iface_count]),
                                    sin,
                                    sizeof(struct sockaddr_in));
                        }
        
                        ifaceinfo->iface_count ++;
                    }
                }
            }
        }
    }

    return 0;
}


/*=========================================================================*/
int SLPInterfaceSockaddrsToString(const struct sockaddr_in* addrs, 
                                  int addrcount,
                                  char** addrstr)
/* Description:
 *    Get the comma delimited string of addresses from an array of sockaddrs
 *
 * Parameters:
 *     addrs (IN) Pointer to array of sockaddrs to convert
 *     addrcount (IN) Number of sockaddrs in addrs.
 *     addrstr (OUT) pointer to receive malloc() allocated address string.
 *                   Caller must free() addrstr when no longer needed.
 *
 * Returns:
 *     zero on success, non-zero (with errno set) on error.
 *=========================================================================*/
{
    /* TODO: to be finished by Satya */

    struct sockaddr_in  *itfaddr;  
    char *str;
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

    /* 16 is the maximum size of a string representation of
     * an IPv4 address (including the comman for the list)
     */
    *addrstr = (char *)xmalloc(addrcount * 16);
    *addrstr[0] = 0;
    
    for (i=0;i<addrcount;i++)
    {
        strcat(*addrstr,inet_ntoa(addrs[i].sin_addr));
        if (i != addrcount-1)
        {
            strcat(*addrstr,",");
        }
    }

    return 0;
}



/*=========================================================================*/
int SLPInterfaceStringToSockaddrs(const char* addrstr,
                                  struct sockaddr_in* addrs,
                                  int* addrcount)
/* Description:
 *    Fill an array of struct sockaddrs from the comma delimited string of 
 *    addresses.
 *
 * Parameters:
 *     addrstr (IN) Address string to convert.
 *     addrcount (OUT) sockaddr array to fill.
 *     addrcount (INOUT) The number of sockaddr stuctures in the addr array
 *                       on successful return will contain the number of 
 *                       sockaddrs that were filled in the addr array
 *
 * Returns:
 *     zero on success, non-zero (with errno set) on error.
 *=========================================================================*/
{
    int i;
    char  tmp;
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
        
        inet_aton(slider1, &(addrs[i].sin_addr));

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
/* #define SLP_IFACE_TEST  */
#ifdef SLP_IFACE_TEST 
int main(int argc, char* argv[])
{
    int i;
    int addrscount =  10;
    struct sockaddr_in* addrs[10];
    SLPInterfaceInfo ifaceinfo;
    char* addrstr;

    if(SLPInterfaceGetInformation(NULL,&ifaceinfo) == 0)
    {
        for(i=0;i<ifaceinfo.iface_count;i++)
        {
            printf("found iface = %s\n",inet_ntoa(ifaceinfo.iface_addr[i].sin_addr));
            printf("bcast addr = %s\n",inet_ntoa(ifaceinfo.bcast_addr[i].sin_addr));
        }
    }

    if(SLPInterfaceStringToSockaddrs("192.168.100.1,192.168.101.1",
                                     (struct sockaddr_in*)&addrs,
                                     &addrscount) == 0)
    {
        if(SLPInterfaceSockaddrsToString((struct sockaddr_in*)&addrs, 
                                         addrscount,
                                         &addrstr) == 0)
        {
            printf("sock addr string = %s\n",addrstr);
            xfree(addrstr);
        }
    }
}
#endif


