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

/** Header file for common network utility functions.
 *
 * @file       slp_net.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#ifndef SLP_NET_H_INCLUDED
#define SLP_NET_H_INCLUDED

/*!@defgroup CommonCodeNetwork Network 
 * @ingroup CommonCode
 */

/*!@defgroup CommonCodeNetUtil Utility */

/*!@addtogroup CommonCodeNetUtil
 * @ingroup CommonCodeNetwork
 * @{
 */

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <winsock2.h>
# include <windows.h>
# include <io.h>
# include <errno.h>
# include <ws2tcpip.h>
# include "slp_win32.h"
# define ETIMEDOUT 110
# define ENOTCONN  107
#else
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <sys/socket.h>
# include <sys/time.h>
# include <netinet/in.h>
# include <arpa/inet.h> 
# include <netdb.h> 
# include <fcntl.h> 
# include <errno.h>
#endif

/** @todo Find a better constant for MAX_HOSTNAME. */
#define MAX_HOST_NAME 512

/* IPv6 SLP address constants */
extern const struct in6_addr in6addr_srvloc_node;
extern const struct in6_addr in6addr_srvloc_link;
extern const struct in6_addr in6addr_srvloc_site;
extern const struct in6_addr in6addr_srvlocda_node;
extern const struct in6_addr in6addr_srvlocda_link;
extern const struct in6_addr in6addr_srvlocda_site;
extern const struct in6_addr in6addr_service_node_mask;
extern const struct in6_addr in6addr_service_link_mask;
/* extern const struct in6_addr in6addr_service_site_mask; */

#define SLP_IN6ADDR_ANY_INIT        {{{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }}}
#define SLP_IN6ADDR_LOOPBACK_INIT   {{{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }}}

extern const struct in6_addr slp_in6addr_any;
extern const struct in6_addr slp_in6addr_loopback;

/* Scope definitions */
#define SLP_SCOPE_NODE_LOCAL  0x01
#define SLP_SCOPE_LINK_LOCAL  0x02
#define SLP_SCOPE_SITE_LOCAL  0x03
#define SLP_SCOPE_ORG_LOCAL   0x04
#define SLP_SCOPE_GLOBAL      0x05

int SLPNetGetThisHostname(char* hostfdn, unsigned int hostfdnLen, int numeric_only, int family);

int SLPNetResolveHostToAddr(const char* host,
                            struct sockaddr_storage* addr);

int SLPNetIsIPV6();

int SLPNetIsIPV4();

//char *SLPNetAddrToString(struct sockaddr_storage *addr); replaced with inet_pton
//int SLPNetStringToAddr(char *str, struct sockaddr_storage *addr); replaced with inet_ntop

int SLPNetCompareAddrs(const struct sockaddr_storage *addr1, const struct sockaddr_storage *addr2);

int SLPNetIsMCast(const struct sockaddr_storage *addr);

int SLPNetIsLocal(const struct sockaddr_storage *addr);

int SLPNetIsLoopback(const struct sockaddr_storage *addr);

int SLPNetSetAddr(struct sockaddr_storage *addr, const int family, const short port, const unsigned char *address, const int addrLen);

int SLPNetSetParams(struct sockaddr_storage *addr, const int family, const short port);

int SLPNetCopyAddr(struct sockaddr_storage *dst, const struct sockaddr_storage *src);

int SLPNetSetSockAddrStorageFromAddrInfo(struct sockaddr_storage *dst, struct addrinfo *src);

int SLPNetSetPort(struct sockaddr_storage *addr, const short port);

char * SLPNetSockAddrStorageToString(struct sockaddr_storage *src, char *dst, int dstLen);

int SLPNetAddrInfoToString(struct addrinfo *src, char *dst, int dstLen);

unsigned long SLPNetGetSrvMcastAddr(const char *pSrvType, unsigned int len, int scope, struct sockaddr_storage *addr);

int SLPNetExpandIpv6Addr(char *ipv6Addr, char *result, int resultSize);

/*! @} */

#endif   /* SLP_NET_H_INCLUDED */

/*=========================================================================*/
