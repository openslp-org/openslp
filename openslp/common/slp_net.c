
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

/*=========================================================================*/
/* IPv6 SLP address constants                                              */
/*=========================================================================*/
const struct in6_addr in6addr_srvloc_node       = { 0xFF,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x01,0x16 };
const struct in6_addr in6addr_srvloc_link       = { 0xFF,0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x01,0x16 };
const struct in6_addr in6addr_srvloc_site       = { 0xFF,0x5,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x01,0x16 };
const struct in6_addr in6addr_srvlocda_node     = { 0xFF,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x01,0x23 };
const struct in6_addr in6addr_srvlocda_link     = { 0xFF,0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x01,0x23 };
const struct in6_addr in6addr_srvlocda_site     = { 0xFF,0x5,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x01,0x23 };
const struct in6_addr in6addr_service_node_mask = { 0xFF,0x1,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x10,0x00 };
const struct in6_addr in6addr_service_link_mask = { 0xFF,0x2,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x10,0x00 };
const struct in6_addr in6addr_service_site_mask = { 0xFF,0x5,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1,0x10,0x00 };

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
    int sts = 0;
    struct sockaddr_in6 *a6 = (struct sockaddr_in6 *) addr;
    struct sockaddr_in *a4 = (struct sockaddr_in *) addr;
    /* quick check for dotted quad IPv4 address */
    if(inet_pton(AF_INET, host, &a4->sin_addr) == 1) {
        addr->ss_family = AF_INET;
    }
    else {
        // try a IPv6 address
        if(inet_pton(AF_INET6, host, &a6->sin6_addr) == 1) {
            addr->ss_family = AF_INET6;
        }
        else {
            sts = -1;
        }
    }
    return(sts);
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
    int sts = -1;
    if (addr1->ss_family == addr2->ss_family) {
        if (addr1->ss_family == AF_INET) {
            struct sockaddr_in *v41 = (struct sockaddr_in *) addr1;
            struct sockaddr_in *v42 = (struct sockaddr_in *) addr2;
            if (v41->sin_family == v42->sin_family) {
                if (v41->sin_port == v42->sin_port) {
                    sts = memcmp(&v41->sin_addr, &v42->sin_addr, sizeof(v41->sin_addr));
                }
            }
        }
        else if (addr1->ss_family == AF_INET6) {
            struct sockaddr_in6 *v61 = (struct sockaddr_in6 *) addr1;
            struct sockaddr_in6 *v62 = (struct sockaddr_in6 *) addr2;
            if (v61->sin6_family == v62->sin6_family) {
                if (v61->sin6_flowinfo == v62->sin6_flowinfo) {
                    if (v61->sin6_port == v62->sin6_port) {
                        if (v61->sin6_scope_id == v62->sin6_scope_id) {
                            sts = memcmp(&v61->sin6_addr, &v62->sin6_addr, sizeof(v61->sin6_addr));
                        }
                    }
                }
            }
        }
        else {
            // don't know how to decode - use memcmp for now
            sts = memcmp(addr1, addr2, sizeof(struct sockaddr_storage));
        }
    }
    else {
        sts = -1;
    }
    return(sts);
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
        struct sockaddr_in6 *v6 = (struct sockaddr_in6 *) addr;
        IN6_IS_ADDR_MULTICAST(&v6->sin6_addr);
    }
	return(0);
}

int SLPNetIsLocal(const struct sockaddr_storage *addr) {
    DWORD sts = 0;
    if (addr->ss_family == AF_INET) {
        struct sockaddr_in *v4 = (struct sockaddr_in *) addr;
        if ((ntohl(v4->sin_addr.S_un.S_addr) & 0xff000000) == 0x7f000000) {
            sts = 1;
        }
        else {
            sts = 0;
        }
    }
    else if (addr->ss_family == AF_INET6) {
        struct sockaddr_in6 *v6 = (struct sockaddr_in6 *) addr;
        sts = IN6_IS_ADDR_LINKLOCAL(&v6->sin6_addr);
    }
	return(sts);
}

int SLPNetIsLoopback(const struct sockaddr_storage *addr) {
    DWORD sts = 0;
    if (addr->ss_family == AF_INET) {
        struct sockaddr_in *v4 = (struct sockaddr_in *) addr;
        if ((ntohl(v4->sin_addr.S_un.S_addr) == INADDR_LOOPBACK)) {
            sts = 1;
        }
        else {
            sts = 0;
        }
    }
    else if (addr->ss_family == AF_INET6) {
        struct sockaddr_in6 *v6 = (struct sockaddr_in6 *) addr;
        sts = IN6_IS_ADDR_LOOPBACK(&v6->sin6_addr);
    }
	return(sts);
}

/* returns the ipv6 scope of the address */
/* v6Addr must be pointer to a 16 byte ipv6 address in binary form */
int setScopeFromAddress(const unsigned char *v6Addr) {

    if (IN6_IS_ADDR_MULTICAST((const struct in6_addr *) v6Addr)) {
        if (IN6_IS_ADDR_MC_GLOBAL((const struct in6_addr *)v6Addr)) {
            return(SLP_SCOPE_GLOBAL);
        }
        if (IN6_IS_ADDR_MC_ORGLOCAL((const struct in6_addr *)v6Addr)) {
            return(SLP_SCOPE_ORG_LOCAL);
        }
        if (IN6_IS_ADDR_MC_SITELOCAL((const struct in6_addr *)v6Addr)) {
            return(SLP_SCOPE_SITE_LOCAL);
        }
        if (IN6_IS_ADDR_MC_NODELOCAL((const struct in6_addr *)v6Addr)) {
            return(SLP_SCOPE_NODE_LOCAL);
        }
        if (IN6_IS_ADDR_MC_LINKLOCAL((const struct in6_addr *)v6Addr)) {
            return(SLP_SCOPE_LINK_LOCAL);
        }
    }
    if (IN6_IS_ADDR_SITELOCAL((const struct in6_addr *)v6Addr)) {
        return(SLP_SCOPE_SITE_LOCAL);
    }
    if (IN6_IS_ADDR_LOOPBACK((const struct in6_addr *)v6Addr)) {
        return(SLP_SCOPE_NODE_LOCAL);
    }
    if (IN6_IS_ADDR_LINKLOCAL((const struct in6_addr *)v6Addr)) {
        return(SLP_SCOPE_LINK_LOCAL);
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
        if (address == NULL)
            v4->sin_addr.s_addr = INADDR_ANY;
        else
            v4->sin_addr.s_addr = htonl(*((int *) address));
    }
    else if (family == AF_INET6) {
        struct sockaddr_in6 *v6 = (struct sockaddr_in6 *) addr;
        v6->sin6_family = family;
        v6->sin6_flowinfo = 0;
        v6->sin6_port = htons(port);
        v6->sin6_scope_id = 0;
        if (address == NULL)
            memcpy(&v6->sin6_addr, &in6addr_any, sizeof(struct in6_addr));
        else
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
        struct sockaddr_in6 *v6 = (struct sockaddr_in6 *) dst;
        v6->sin6_family = AF_INET6;
        v6->sin6_flowinfo = 0;
        v6->sin6_port = 0;
        v6->sin6_scope_id = 0;
        memcpy(&v6->sin6_addr, &((struct sockaddr_in6 *) src->ai_addr)->sin6_addr, sizeof(struct in6_addr));
    }
    else {
        return(-1);
    }
    return(0);
}

int SLPNetSetPort(struct sockaddr_storage *addr, const short port) {
	if (addr->ss_family == AF_INET) {
		((struct sockaddr_in *)addr)->sin_port = htons(port);
    }
	else if (addr->ss_family == AF_INET6) {
		((struct sockaddr_in6 *)addr)->sin6_port = htons(port);
    }
    else {
        return -1;
    }
	return 0;
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
        return(NULL);
    }
    return(dst);
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


/*
 * Description:
 *    Returns the IPv6 multicast address for the specified Service Type.
 *    
 *
 * Parameters:
 *  (in) pSrvType   The Service Type String
 *  (in) len		Length of pSrvType
 *  (inout)addr		Sockaddr storage to return the multicast addr
 *
 * Returns: zero on success, non-zero on error;
 *-------------------------------------------------------------------------*/
unsigned long SLPNetGetSrvMcastAddr(const char *pSrvType, unsigned int len, int scope, struct sockaddr_storage *addr) {
	unsigned long group_id = 0;
	struct in6_addr *v6;

	if (addr == NULL || pSrvType == NULL)
		return -1;

	/* Run Hash to get group id */
	while (len-- != 0) {
		group_id *= 33;
		group_id += *pSrvType++;
	}
	group_id &= 0x3FF;

	v6 = &((struct sockaddr_in6 *) addr)->sin6_addr;
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
	addr->ss_family = AF_INET6;

	return 0;
}

//#define SLP_NET_TEST
#ifdef SLP_NET_TEST
int main(int argc, char* argv[]) {
    char hostfdn[1024];
    char addrString[1024];
    int sts;
    int errorCount = 0;
    struct sockaddr_storage addr;
    #ifdef _WIN32
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2,2), &wsadata);
    #endif

    sts = SLPNetGetThisHostname(hostfdn, sizeof(hostfdn), 0, AF_INET);
    if (sts != 0) {
        printf("error %d with SLPNetGetThisHostname.\r\n", sts);
        errorCount++;
    }
    else {
        printf("hostfdn = %s\r\n", hostfdn);
    }
    sts = SLPNetGetThisHostname(hostfdn, sizeof(hostfdn), 1, AF_INET);
    if (sts != 0) {
        printf("error %d with SLPNetGetThisHostname.\r\n", sts);
        errorCount++;
    }
    else {
        printf("hostfdn = %s\r\n", hostfdn);
    }
    sts = SLPNetGetThisHostname(hostfdn, sizeof(hostfdn), 0, AF_INET6);
    if (sts != 0) {
        printf("error %d with SLPNetGetThisHostname.\r\n", sts);
        errorCount++;
    }
    else {
        printf("hostfdn = %s\r\n", hostfdn);
    }
    sts = SLPNetGetThisHostname(hostfdn, sizeof(hostfdn), 1, AF_INET6);
    if (sts != 0) {
        printf("error %d with SLPNetGetThisHostname.\r\n", sts);
        errorCount++;
    }
    else {
        printf("hostfdn = %s\r\n", hostfdn);
    }

    sts = SLPNetResolveHostToAddr("localhost", &addr);
    if (sts != 0) {
        printf("error %d with SLPNetResolveHostToAddr.\r\n", sts);
        errorCount++;
    }
    else {
        printf("addr family = %d\r\n", addr.ss_family);
        SLPNetSockAddrStorageToString(&addr, addrString, sizeof(addrString));
        printf("address = %s\r\n", addrString);
    }

    sts = SLPNetResolveHostToAddr("::1", &addr);
    if (sts != 0) {
        printf("error %d with SLPNetResolveHostToAddr.\r\n", sts);
        errorCount++;
    }
    else {
        printf("addr family = %d\r\n", addr.ss_family);
        SLPNetSockAddrStorageToString(&addr, addrString, sizeof(addrString));
        printf("address = %s\r\n", addrString);
    }


    sts = SLPPropertyReadFile("e:\\source\\Hogwarts_ActiveX\\OpenSLP\\ipv6\\win32\\slpd\\slp.conf");
    if (sts == 0) {
        printf("Read config file\r\n");
    }
    else {
        printf("No config file found - using defaults.\r\n");
    }


    sts = SLPNetIsIPV6();
    if (sts == 0) {
        printf("Not using ipv6\r\n");
    }
    else {
        printf("Using ipv6\r\n");
    }
    sts = SLPNetIsIPV4();
    if (sts == 0) {
        printf("Not using ipv4\r\n");
    }
    else {
        printf("Using ipv4\r\n");
    }
    {
        struct sockaddr_storage a1;
        struct sockaddr_storage a2;
        char testaddr[] = "1:2:3:4:5::6";
        struct sockaddr_in *p41 = (struct sockaddr_in *) &a1;
        struct sockaddr_in6 *p61 = (struct sockaddr_in6 *) &a1;
        struct sockaddr_in *p42 = (struct sockaddr_in *) &a2;
        struct sockaddr_in6 *p62 = (struct sockaddr_in6 *) &a2;

        memset(&a1, 0, sizeof(a1));
        memset(&a2, 0, sizeof(a2));
        SLPNetSetAddr(&a1, AF_INET6, 2, testaddr, sizeof(testaddr));
        // first test with SLPNetCopyAddr
        SLPNetCopyAddr(&a2, &a1);
        sts = SLPNetCompareAddrs(&a1, &a2);
        if (sts != 0) {
            printf("Error, address a1 does not equal a2 - copy failed\r\n");
        }
        memset(&a2, 0, sizeof(a2));
        a2.ss_family = AF_INET6;
        memcpy(p62->sin6_addr.s6_addr, testaddr, sizeof(testaddr));
        p62->sin6_family = AF_INET6;
        p62->sin6_port = htons(2);
        sts = SLPNetCompareAddrs(&a1, &a2);
        if (sts != 0) {
            printf("Error, address a1 does not equal a2\r\n");
        }
    }

    /*
        int SLPNetIsMCast(const struct sockaddr_storage *addr);
        int SLPNetIsLocal(const struct sockaddr_storage *addr);
    */

    #ifdef _WIN32
    WSACleanup();
    #endif

}
#endif




