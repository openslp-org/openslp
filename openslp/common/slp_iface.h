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

/*=========================================================================*/
int SLPInterfaceGetAddresses(struct sockaddr* addr, int* addrcount);
/* Description:
 *    Get the network interface addresses for this host.  Exclude the
 *    loopback interface
 *
 * Parameters:
 *     addr (OUT) Pointer to array of sockaddr structures to be filled*
 *     addrcount (INOUT) The number of sockaddr stuctures in the addr array
 *                       on successful return will contain the number of 
 *                       sockaddrs that were filled in the addr array
 *
 * Returns:
 *     zero on success, non-zero (with errno set) on error.
 *=========================================================================*/
                             


/*=========================================================================*/
int SLPInterfaceGetBroadcastAddress(const struct sockaddr* ifaddr,
                                    struct sockaddr bcastaddr);
/* Description:
 *    Get the broadcast address for the specified interface 
 *
 * Parameters:
 *     ifaddr (IN) Pointer to interface address to get broadcast address for
 *     addrcount (OUT) Pointer to sockaddr to receive the broadcast address
 *
 * Returns:
 *     zero on success, non-zero (with errno set) on error.
 *=========================================================================*/



/*=========================================================================*/
int SLPInterfaceSockaddrsToString(const struct sockaddr* addrs, 
                                  int addrcount,
                                  char** addrstr);
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



/*=========================================================================*/
int SLPInterfaceStringToSockaddrs(const char* addrstr,
                                  struct sockaddr* addrs,
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
