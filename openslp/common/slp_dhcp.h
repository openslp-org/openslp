/*-------------------------------------------------------------------------
 * Copyright (C) 2001 Novell, Inc.
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
 *    Neither the name of Novell nor the names of its contributors
 *    may be used to endorse or promote products derived from this 
 *    software without specific prior written permission.
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

/** Header file for common DHCP lookup functions.
 *
 * @file       slp_dhcp.h
 * @author     John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#ifndef SLP_DHCP_H_INCLUDED
#define SLP_DHCP_H_INCLUDED

/*!@defgroup CommonCodeDHCP DHCP Lookup */

/*!@addtogroup CommonCodeDHCP
 * @ingroup CommonCode
 * @{
 */

#include <stddef.h>

/* BOOTP/DHCP packet header format:
 *
 * Offs  Len   Name     Description
 * 0     1     opcode   Message opcode: 1 = BOOTREQUEST, 2 = BOOTREPLY
 * 1     1     htype    Hardware address type (eg., 1 = 10mb ethernet)
 * 2     1     hlen     Hardware address length (eg., 6 = 10mb ethernet)
 * 3     1     hops     Client sets to zero, optionally used by relay agents
 * 4     4     xid      Transaction ID, random number chosen by client
 * 8     2     secs     Client sets to seconds since start of boot process
 * 10    2     flags    Bit 0: broadcast response bit
 * 12    4     ciaddr   Client IP address - only filled if client is bound
 * 16    4     yiaddr   'your' (Client) IP address
 * 20    4     siaddr   IP address of next server to use in bootstrap
 * 24    4     giaddr   Relay agent IP address, used in booting via RA
 * 28    16    chaddr   Client hardware address
 * 44    64    sname    Optional server host name, null-terminated string
 * 108   128   file     Boot file name, null-terminated string
 * 236   var   options  Optional parameters field
 *
 * The options field has the following format:
 *
 * Offs  Len   Name     Description
 * 0     4     cookie   4-byte cookie field: 99.130.83.99 (0x63825363)
 * 
 * Followed by 1-byte option codes and 1-byte option lengths, except
 * for the two special fixed length options, pad (0) and end (255).
 * Options are defined in slp_dhcp.h as TAG_XXX values. The two we
 * really care about here are options TAG_SLP_DA and TAG_SLP_SCOPE,
 * 78 and 79, respectively. 
 * 
 * The format for TAG_SLP_DA (starting with the tag) is:
 * 
 * Offs  Len   Name     Description
 * 0     1     tag      TAG_SLP_DA - directory agent ip addresses
 * 1     1     length   length of remaining data in the option
 * 2     1     mand     flag: the use of these DA's is mandatory
 * 3     4     a(0)     4-byte ip address
 * ...
 * 3+n*4 4     a(n)     4-byte ip address
 *
 * The format for TAG_SLP_SCOPE (starting with the tag) is:
 *
 * Offs  Len   Name     Description
 * 0     1     tag      TAG_SLP_SCOPE - directory scopes to use
 * 1     1     length   length of remaining data in the option
 * 2     1     mand     flag: the use of these scopes is mandatory
 * 3     var   scopes   a null-terminated, comma-separated string of scopes
 *
 * The "DHCP Message Type" option must be included in every DHCP message.
 * All tags except for TAG_PAD(0) and TAG_END(255) begin with a tag value
 * followed by a length of remaining data value. 
 */

/* Applicable IANA BOOTP/DHCP option tag values */
#define TAG_PAD               0     /* Fixed size, 1 byte (0), no length */
#define TAG_DHCP_MSG_TYPE     53
#define TAG_DHCP_PARAM_REQ    55
#define TAG_CLIENT_IDENTIFIER 61
#define TAG_SLP_DA            78
#define TAG_SLP_SCOPE         79
#define TAG_END               255

/* The Novell (pre-rfc2610 or draft 3) format for the DHCP TAG_SLP_DA option
 * has the 'mandatory' flag containing other bits besides simply 'mandatory'.
 * These flags are important because if the DA_NAME_PRESENT flag is set, then 
 * we know we are parsing this format, otherwise it's the rfc2610 format. 
 */
#define DA_NAME_PRESENT    0x80  /* DA name present in option */
#define DA_NAME_IS_DNS     0x40  /* DA name is host name or DNS name */
#define DISABLE_DA_MCAST   0x20  /* Multicast for DA's is disabled */
#define SCOPE_PRESENT      0x10  /* Scope is present in option */

/* Character type encodings that we expect to be supported. */
#define CT_ASCII     3       /* standard 7 or 8 bit ASCII */
#define CT_UTF8      106     /* UTF-8 */
#define CT_UNICODE   1000    /* normal Unicode */

typedef int DHCPInfoCallBack(int tag, void * optdata, 
      size_t optdatasz, void * context);

int DHCPGetOptionInfo(unsigned char * dhcpOptCodes, int dhcpOptCodeCnt, 
      DHCPInfoCallBack * dhcpInfoCB, void * context);

int DHCPParseSLPTags(int tag, void * optdata, size_t optdatasz, 
      void * context);

typedef struct _DHCPContext
{
   int addrlistlen;
   int scopelistlen;
   char scopelist[256];
   unsigned char addrlist[256];
} DHCPContext;

/*! @} */

#endif   /* SLP_DHCP_H_INCLUDED */

/*=========================================================================*/
