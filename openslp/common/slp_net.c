
/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_net.h                                                  */
/*                                                                         */
/* Abstract:    Network utility functions                                  */
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

#include "..\libslp\slp.h"
#include "slp_net.h"
#include "slp_xmalloc.h"
#include "slp_property.h"
#include <assert.h>
/*-------------------------------------------------------------------------*/
int SLPNetGetThisHostname(char* hostfdn, unsigned int hostfdnLen, int numeric_only, int family)
/* 
 * Description:
 *    Returns a string represting this host (the FDN) or null.                                                     
 *
 * Parameters:
 *    hostfdn   (OUT) pointer to char pointer that is set to buffer 
 *                    contining this machine's FDN.  Caller must free
 *                    returned string with call to xfree()
 *    numeric_only (IN) force return of numeric address.  
 *
 *     family    (IN) Hint family to get info for - can be AF_INET, AF_INET6, 
 *                    or AF_UNSPEC for both
 *     
 *-------------------------------------------------------------------------*/
{
    char host[MAX_HOST_NAME];
    struct addrinfo *ifaddr;
    struct addrinfo hints;
    int sts = 0;

    *hostfdn = 0;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = family;
    if(gethostname(host, MAX_HOST_NAME) == 0)
    {
        sts = getaddrinfo(host, NULL, &hints, &ifaddr);
        if (sts == 0) {
            /* if the hostname has a '.' then it is probably a qualified 
             * domain name.  If it is not then we better use the IP address
            */ 
            if(!numeric_only && strchr(host, '.'))
            {
                 strncpy(hostfdn, host, hostfdnLen);
            }
            else
            {   
                sts = SLPNetAddrInfoToString(ifaddr,  hostfdn, hostfdnLen);
            }
            freeaddrinfo(ifaddr);
        }
        else {
            assert(1);
        }
    }

    return(sts);
}

/*-------------------------------------------------------------------------*/
int SLPNetResolveHostToAddr(const char* host,
                            struct sockaddr_storage* addr)
/* 
 * Description:
 *    Returns a string represting this host (the FDN) or null.                                                    
 *
 * Parameters:
 *    host  (IN)  pointer to hostname to resolve
 *    addr  (OUT) pointer to in_addr that will be filled with address
 *
 * Returns: zero on success, non-zero on error;
 *-------------------------------------------------------------------------*/
{
    struct hostent* he;
    
    /* quick check for dotted quad IPv4 address */
    if(inet_pton(AF_INET, host, addr) != 1) {
        // try a IPv6 address
        if(inet_pton(AF_INET6, host, addr) != 1) {
            return -1;
        }
    }
    /* Use resolver */
    he = gethostbyname(host);
    if(he != 0)
    {
        if(he->h_addrtype == AF_INET)
        {
            SLPNetSetAddr(addr, AF_INET, 0, he->h_addr_list[0], sizeof(struct in_addr));
            return 0;
        }
        else if (he->h_addrtype == AF_INET6) {
            SLPNetSetAddr(addr, AF_INET6, 0, he->h_addr_list[0], sizeof(struct in_addr));
            return 0;
        }
    }
    return -1;
}

int SLPNetIsIPV6() {
    int isv6 = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.useIPV6"));
    if (isv6) {
        return(1);
    }
    else {
    	return(0);
    }
}

int SLPNetIsIPV4() {
    int isv4 = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.useIPV4"));
    if (isv4) {
        return(1);
    }
    else {
    	return(0);
    }
}

int SLPNetCompareAddrs(const struct sockaddr_storage *addr1, const struct sockaddr_storage *addr2) {
    if (memcmp(addr1, addr2, sizeof(struct sockaddr_storage)) == 0) {
        return(1);
    }
    else {
	    return(0);
    }
}


int SLPNetIsMCast(const struct sockaddr_storage *addr) {
    if (addr->ss_family == AF_INET) {
        struct sockaddr_in *v4 = (struct sockaddr_in *) addr;
        if ((ntohl(v4->sin_addr.S_un.S_addr) & 0xff000000) >= 0xef000000) {
            return(1);
        }
        else {
            return(0);
        }
    }
    else if (addr->ss_family == AF_INET6) {
        IN6_IS_ADDR_MULTICAST((struct in6_addr *)addr);
    }
	return(0);
}

int SLPNetIsLocal(const struct sockaddr_storage *addr) {
    if (addr->ss_family == AF_INET) {
        struct sockaddr_in *v4 = (struct sockaddr_in *) addr;
        if ((ntohl(v4->sin_addr.S_un.S_addr) & 0xff000000) == 0x7f000000) {
            return(1);
        }
        else {
            return(0);
        }
    }
    else if (addr->ss_family == AF_INET6) {
        IN6_IS_ADDR_LINKLOCAL((struct in6_addr *)addr);
    }
	return(0);
}

int SLPNetSetAddr(struct sockaddr_storage *addr, const int family, const short port, const unsigned char *address, const int addrLen) {
    int sts = 0;
    addr->ss_family = family;
    if (family == AF_INET) {
        struct sockaddr_in *v4 = (struct sockaddr_in *) addr;
        v4->sin_family = family;
        v4->sin_port = htons(port);
        memcpy(&(v4->sin_addr), address, min(addrLen, sizeof(v4->sin_addr)));
    }
    else if (family == AF_INET6) {
        struct sockaddr_in6 *v6 = (struct sockaddr_in6 *) addr;
        v6->sin6_family = family;
        v6->sin6_flowinfo = 0;
        v6->sin6_port = htons(port);
        v6->sin6_scope_id = 0;
        memcpy(&v6->sin6_addr, address, min(addrLen, sizeof(v6->sin6_addr)));
    }
    else {
        sts = -1;
    }
	return(sts);
}

int SLPNetCopyAddr(struct sockaddr_storage *dst, const struct sockaddr_storage *src) {
    memcpy(dst, src, sizeof(struct sockaddr_storage));
	return(0);
}


int SLPNetSetSockAddrStorageFromAddrInfo(struct sockaddr_storage *dst, struct addrinfo *src) {
    dst->ss_family = src->ai_family;
    if (src->ai_family == AF_INET) {
        memcpy(dst, src->ai_addr, sizeof(struct sockaddr_in));
    }
    else if (src->ai_family == AF_INET6) {
        memcpy(dst, src->ai_addr, sizeof(struct sockaddr_in6));
    }
    else {
        return(-1);
    }
    return(0);
}

/*
 * Description:
 *    Used to obtain a string representation of the network portion of a sockaddr_storage struct
 *    
 *
 * Parameters:
 *  (in) src        Source address to grab the network address from
 *  (in/out) dst    Destination address to be filled in with the address ("x.x.x.x" for ipv4,
 *                   ("x:x:..:x" for IPV6)
 *  (out) dstLen    The number of bytes that may be copied into dst
 *
 * Returns: dst if address set correctly, NULL if there were errors setting dst
 *-------------------------------------------------------------------------*/
char * SLPNetSockAddrStorageToString(struct sockaddr_storage *src, char *dst, int dstLen) {
    if (src->ss_family == AF_INET) {
        struct sockaddr_in *v4 = (struct sockaddr_in *) src;

        inet_ntop(v4->sin_family, &v4->sin_addr, dst, dstLen);
    }
    else if (src->ss_family == AF_INET6) {
        struct sockaddr_in6 *v6 = (struct sockaddr_in6 *) src;

        inet_ntop(v6->sin6_family, &v6->sin6_addr, dst, dstLen);
    }
    else {
        return(dst);
    }
    return(NULL);
}


int SLPNetAddrInfoToString(struct addrinfo *src, char *dst, int dstLen) {
    if (src->ai_family == AF_INET) {
        struct sockaddr *addr = (struct sockaddr *) src->ai_addr;
        inet_ntop(src->ai_family, &addr->sa_data[2], dst, dstLen);
    }
    else if (src->ai_family == AF_INET6) {
        struct sockaddr_in6 *addr = (struct sockaddr_in6 *) src->ai_addr;
        inet_ntop(src->ai_family, &addr->sin6_addr, dst, dstLen);
    }
    else {
        return(-1);
    }
    return(0);
}

