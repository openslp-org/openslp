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

#ifndef SLP_NET_H_INCLUDED
#define SLP_NET_H_INCLUDED

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <io.h>
#include <errno.h>
//#if(_WIN32_WINNT >= 0x0400 && _WIN32_WINNT < 0x0500) 
#include <ws2tcpip.h>
#include <iptypes.h>
#include <iphlpapi.h>
#ifndef IPPROTO_IPV6
//#include <tpipv6.h> // For IPv6 Tech Preview.
#endif

//#endif
#include "slp_win32.h"
#define ETIMEDOUT 110
#define ENOTCONN  107
#else
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <netdb.h> 
#include <fcntl.h> 
#include <errno.h>
#endif


/* TODO find a better constant for MAX_HOSTNAME */
#define MAX_HOST_NAME 512


/*=========================================================================*/
/* IPv6 SLP address constants                                              */
/*=========================================================================*/
extern const struct in6_addr in6addr_srvloc_node;
extern const struct in6_addr in6addr_srvloc_link;
extern const struct in6_addr in6addr_srvloc_site;
extern const struct in6_addr in6addr_srvlocda_node;
extern const struct in6_addr in6addr_srvlocda_link;
extern const struct in6_addr in6addr_srvlocda_site;
extern const struct in6_addr in6addr_service_node_mask;
extern const struct in6_addr in6addr_service_link_mask;
/* extern const struct in6_addr in6addr_service_site_mask; */

/* Scope definitions */
#define SLP_SCOPE_NODE_LOCAL                0x01
#define SLP_SCOPE_LINK_LOCAL                0x02
#define SLP_SCOPE_SITE_LOCAL                0x03
#define SLP_SCOPE_ORG_LOCAL                 0x04
#define SLP_SCOPE_GLOBAL                    0x05

/*-------------------------------------------------------------------------*/
int SLPNetResolveHostToAddr(const char* host,
                            struct sockaddr_storage* addr);
/*
 * Description:
 *    Returns a string represting this host (the FDN) or null. Caller must
 *    free returned string
 *
 * Parameters:
 *    host  (IN)  pointer to hostname to resolve
 *    addr  (OUT) pointer to addr that will be filled with address
 *
 * Returns: zero on success, non-zero on error;
 *-------------------------------------------------------------------------*/


int SLPNetIsIPV6();
/*
 * Description:
 *    Used to determine if IPV6 was enabled in the configuration file
 *    
 *
 * Parameters:
 *
 * Returns: non-zero if IPV6 was configured, 0 if not configured
 *-------------------------------------------------------------------------*/


int SLPNetIsIPV4();
/*
 * Description:
 *    Used to determine if IPV4 was enabled in the configuration file
 *    
 *
 * Parameters:
 *
 * Returns: non-zero if IPV4 was configured, 0 if not configured
 *-------------------------------------------------------------------------*/

//char *SLPNetAddrToString(struct sockaddr_storage *addr); replaced with inet_pton
//int SLPNetStringToAddr(char *str, struct sockaddr_storage *addr); replaced with inet_ntop

int SLPNetCompareAddrs(const struct sockaddr_storage *addr1, const struct sockaddr_storage *addr2);
/*
 * Description:
 *    Used to determine if two sockaddr_storage structures are equal
 *    
 *
 * Parameters:
 *  (in) addr1  First address to be compared
 *  (in) addr2  Second address to be compared
 *
 * Returns: non-zero if not equal, 0 if equal
 *-------------------------------------------------------------------------*/
int SLPNetIsMCast(const struct sockaddr_storage *addr);
/*
 * Description:
 *    Used to determine if the specified sockaddr_storage is a multicast address
 *    
 *
 * Parameters:
 *  (in) addr  Address to be tested to see if multicast
 *
 * Returns: non-zero if address is a multicast address, 0 if not multicast
 *-------------------------------------------------------------------------*/

int SLPNetIsLocal(const struct sockaddr_storage *addr);
/*
 * Description:
 *    Used to determine if the specified sockaddr_storage is a local address
 *    
 *
 * Parameters:
 *  (in) addr  Address to be tested to see if local
 *
 * Returns: non-zero if address is a local address, 0 if not local
 *-------------------------------------------------------------------------*/

int SLPNetIsLoopback(const struct sockaddr_storage *addr);
/*
 * Description:
 *    Used to determine if the specified sockaddr_storage is a loopback address
 *    
 *
 * Parameters:
 *  (in) addr  Address to be tested to see if loopback
 *
 * Returns: non-zero if address is a local address, 0 if not loopback
 *-------------------------------------------------------------------------*/


int SLPNetSetAddr(struct sockaddr_storage *addr, const int family, const short port, const unsigned char *address, const int addrLen);
/*
 * Description:
 *    Used to set up the relevant fields of a sockaddr_storage structure
 *    
 *
 * Parameters:
 *  (in/out) addr   Address of sockaddr_storage struct to be filled out
 *  (in) family     Protocol family (AF_INET for IPV4, AF_INET6 for IPV6)
 *  (in) port       Port for this address.  Note that appropriate host to network translations
 *                  will occur as part of this call.
 *  (in) address    The IP address for this sockaddr_storage struct.  If NULL this call will 
 *                  set the address to IN_ADDR_ANY.
 *  (in) addrLen    The length of the adddress to be set.
 *
 * Returns: 0 if address set correctly, non-zero there were errors setting fields of addr
 *-------------------------------------------------------------------------*/

int SLPNetSetParams(struct sockaddr_storage *addr, const int family, const short port);
/*
 * Description:
 *    Used to set the port and family of a sockaddr_storage
 *    
 *
 * Parameters:
 *  (in/out) addr   Address of sockaddr_storage struct to be filled out
 *  (in) family     Protocol family (AF_INET for IPV4, AF_INET6 for IPV6)
 *  (in) port       Port for this address.  Note that appropriate host to network translations
 *                  will occur as part of this call.
 *
 * Returns: 0 if address set correctly, non-zero there were errors setting fields of addr
 *-------------------------------------------------------------------------*/

int SLPNetCopyAddr(struct sockaddr_storage *dst, const struct sockaddr_storage *src);
/*
 * Description:
 *    Used to copy one sockaddr_storage struct to another
 *    
 *
 * Parameters:
 *  (in/out) dst    Destination address to be filled in
 *  (in) src        Source address to be copied from
 *
 * Returns: 0 if address set correctly, non-zero there were errors setting fields of dst
 *-------------------------------------------------------------------------*/


int SLPNetSetSockAddrStorageFromAddrInfo(struct sockaddr_storage *dst, struct addrinfo *src);
/*
 * Description:
 *    Used to copy an addrinfo struct to a sockaddr_storage struct
 *    
 *
 * Parameters:
 *  (in/out) dst    Destination address to be filled in
 *  (in) src        Source address to be copied from
 *
 * Returns: 0 if address set correctly, non-zero there were errors setting fields of dst
 *-------------------------------------------------------------------------*/

int SLPNetSetPort(struct sockaddr_storage *addr, const short port);
/*
 * Description:
 *    Used to copy an addrinfo struct to a sockaddr_storage struct
 *    
 *
 * Parameters:
 *  (in/out) addr   Destination address whose port is being set
 *  (in) port        Port to be set
 *
 * Returns: 0 if port was set correctly, non-zero there were errors setting fields of dst
 *-------------------------------------------------------------------------*/

char * SLPNetSockAddrStorageToString(struct sockaddr_storage *src, char *dst, int dstLen);
/*
 * Description:
 *    Used to obtain a string representation of the network portion of a sockaddr_storage struct
 *    
 *
 * Parameters:
 *  (in) src        Source address to grab the network address from - family must be set
 *  (in/out) dst    Destination address to be filled in with the address ("x.x.x.x" for ipv4,
 *                   ("x:x:..:x" for IPV6)
 *  (out) dstLen    The number of bytes that may be copied into dst
 *
 * Returns: dst if address set correctly, NULL if there were errors setting dst
 *-------------------------------------------------------------------------*/

int SLPNetAddrInfoToString(struct addrinfo *src, char *dst, int dstLen);
/*
 * Description:
 *    Used to obtain a string representation of the network portion of a addrinfo struct
 *    
 *
 * Parameters:
 *  (in) src        Source address to grab the network address from - family must be set
 *  (in/out) dst    Destination address to be filled in with the address ("x.x.x.x" for ipv4,
 *                   ("x:x:..:x" for IPV6)
 *  (out) dstLen    The number of bytes that may be copied into dst
 *
 * Returns: 0 if address set correctly, non-zero there were errors setting dst
 *-------------------------------------------------------------------------*/

unsigned long SLPNetGetSrvMcastAddr(const char *pSrvType, unsigned int len, int scope, struct sockaddr_storage *addr);
/*
 * Description:
 *    Returns the IPv6 multicast address for the specified Service Type.
 *    
 *
 * Parameters:
 *  (in) pSrvType   The Service Type String
 *  (in) len		Length of pSrvType
 *  (in) scope		Scope of the multicast address
 *  (inout)addr		Sockaddr storage to return the multicast addr
 *
 * Returns: zero on success, non-zero on error;
 *-------------------------------------------------------------------------*/

int SLPNetExpandIpv6Addr(char *ipv6Addr, char *result, int resultSize);
/* 
*   Description
*    fully expands a ipv6 address given an ipv6 address in the shorthand notation  
*
*   Parameters
*    [in] ipv6Addr - the shorthand address to expand
*    [out] result - buffer to store the expanded address in
*    [in] resultSize - size of the result buffer, must be atleast 40 bytes
*
*   Returns
*   zero on success, non-zero if errors were detected
*-------------------------------------------------------------------------*/

#endif
