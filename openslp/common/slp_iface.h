/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_iface.h                                                */
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

#ifndef SLP_IFACE_H_INCLUDED
#define SLP_IFACE_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock.h>
#endif

#define SLP_MAX_IFACES 10

/*=========================================================================*/
typedef struct _SLPInterfaceInfo
/*=========================================================================*/
{
    int iface_count;
    struct sockaddr_in iface_addr[SLP_MAX_IFACES];
    struct sockaddr_in bcast_addr[SLP_MAX_IFACES];
}SLPIfaceInfo;


/*=========================================================================*/
int SLPIfaceGetInfo(const char* useifaces,
                    SLPIfaceInfo* ifaces);
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
 *
 * Returns:
 *     zero on success, non-zero (with errno set) on error.
 *=========================================================================*/


/*=========================================================================*/
int SLPIfaceSockaddrsToString(const struct sockaddr_in* addrs,
                              int addrcount,
                              char** addrstr);
/* Description:
 *    Get the comma delimited string of addresses from an array of sockaddrs
 *
 * Parameters:
 *     addrs (IN) Pointer to array of sockaddrs to convert
 *     addrcount (IN) Number of sockaddrs in addrs.
 *     addrstr (OUT) pointer to receive xmalloc() allocated address string.
 *                   Caller must xfree() addrstr when no longer needed.
 *
 * Returns:
 *     zero on success, non-zero (with errno set) on error.
 *=========================================================================*/



/*=========================================================================*/
int SLPIfaceStringToSockaddrs(const char* addrstr,
                              struct sockaddr_in* addrs,
                              int* addrcount);
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

#endif
