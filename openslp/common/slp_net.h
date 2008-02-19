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
 * @ingroup    CommonCodeNetUtil
 */

#ifndef SLP_NET_H_INCLUDED
#define SLP_NET_H_INCLUDED

/*!@defgroup CommonCodeNetwork Network 
 * @ingroup CommonCode
 */

/*!@defgroup CommonCodeNetUtil Utility
 * @ingroup CommonCodeNetwork
 * @{
 */

#include "slp_types.h"
#include "slp_socket.h"

/** @todo Find a better constant for MAX_HOSTNAME. */

/** The maximum length of a host name in OpenSLP. */
#define MAX_HOST_NAME 512

/** IPv6 SLP address constants */
extern const struct in6_addr in6addr_srvloc_node;
extern const struct in6_addr in6addr_srvloc_link;
extern const struct in6_addr in6addr_srvloc_site;
extern const struct in6_addr in6addr_srvlocda_node;
extern const struct in6_addr in6addr_srvlocda_link;
extern const struct in6_addr in6addr_srvlocda_site;
extern const struct in6_addr in6addr_service_node_mask;
extern const struct in6_addr in6addr_service_link_mask;
/* extern const struct in6_addr in6addr_service_site_mask; */

/** IN6 "Any" and "Loopback" address initializer macros */
#ifdef _AIX
# define SLP_IN6ADDR_ANY_INIT        {{{0,0,0,0}}}
# define SLP_IN6ADDR_LOOPBACK_INIT   {{{0,0,0,1}}}
# define ss_family                   __ss_family
#else
# define SLP_IN6ADDR_ANY_INIT        {{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}}
# define SLP_IN6ADDR_LOOPBACK_INIT   {{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}}}
#endif

extern const struct in6_addr slp_in6addr_any;
extern const struct in6_addr slp_in6addr_loopback;

/** Scope definitions */
#define SLP_SCOPE_NODE_LOCAL  0x01
#define SLP_SCOPE_LINK_LOCAL  0x02
#define SLP_SCOPE_SITE_LOCAL  0x05
#define SLP_SCOPE_ORG_LOCAL   0x08
#define SLP_SCOPE_GLOBAL      0x0e

int SLPNetResolveHostToAddr(const char * host, 
      struct sockaddr_storage * addr);
int SLPNetIsIPV6(void);
int SLPNetIsIPV4(void);
int SLPNetCompareAddrs(const void * addr1, const void * addr2);
int SLPNetIsMCast(const void * addr);
int SLPNetIsLocal(const void * addr);
int SLPNetIsLoopback(const void * addr);
int SLPNetSetAddr(void * addr, int family, uint16_t port, 
      const void * address);
int SLPNetSetParams(void * addr, int family, uint16_t port);
int SLPNetSetPort(void * addr, uint16_t port);
char * SLPNetSockAddrStorageToString(struct sockaddr_storage const * src, 
      char * dst, size_t dstLen);
int SLPNetGetSrvMcastAddr(const char * pSrvType, size_t len, 
      int scope, void * addr);
int SLPNetExpandIpv6Addr(const char * ipv6Addr, char * result, 
      size_t resultSize);
unsigned int SLPNetGetMCastScope(struct sockaddr_storage* addr);
int SLPNetIsMCastSrvloc(struct sockaddr_storage* addr);
int SLPNetIsMCastSrvlocDA(struct sockaddr_storage* addr);
int SLPNetAddrLen(void * addr);

/*! @} */

#endif   /* SLP_NET_H_INCLUDED */

/*=========================================================================*/
