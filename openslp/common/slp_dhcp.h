/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_dhcp.h                                                 */
/*                                                                         */
/* Abstract:    Common for DHCP Lookup routines for OpenSLP.			   	*/
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
/***************************************************************************/

#ifndef SLP_DHCP_H_INCLUDED
#define SLP_DHCP_H_INCLUDED

#include <stddef.h>

/* Applicable IANA BOOTP/DHCP option tag values */
#define TAG_PAD     				0		/* Fixed size, 1 byte (0), no length */

#define TAG_DHCP_MSG_TYPE		53

#define TAG_DHCP_PARAM_REQ		55

#define TAG_CLIENT_IDENTIFIER	61

#define TAG_SLP_DA				78
#define TAG_SLP_SCOPE			79

#define TAG_END     				255

/* The Novell (pre-rfc2610 or draft 3) format for the DHCP TAG_SLP_DA option
	has the 'mandatory' flag containing other bits besides simply 'mandatory'.
	These flags are important because if the DA_NAME_PRESENT flag is set, then 
	we know we are parsing this format, otherwise it's the rfc2610 format. */

#define DA_NAME_PRESENT		0x80	/* DA name present in option */
#define DA_NAME_IS_DNS		0x40	/* DA name is host name or DNS name */
#define DISABLE_DA_MCAST	0x20	/* Multicast for DA's is disabled */
#define SCOPE_PRESENT		0x10	/* Scope is present in option */

/* Character type encodings that we expect to be supported. */
#define CT_ASCII     3       /* standard 7 or 8 bit ASCII */
#define CT_UTF8      106     /* UTF-8 */
#define CT_UNICODE   1000    /* normal Unicode */

/*=========================================================================*/
typedef int DHCPInfoCallBack(int tag, void *optdata, 
		size_t optdatasz, void *context);
/* Callback routine used by DHCPGetOptionInfo - called once for each			*/
/* option specified in the option array passed to that routine. If this		*/
/* routine returns a non-zero value, that value will be immediately			*/
/*	returned to the caller of DHCPGetOptionInfo.										*/
/*                                                                         */
/* Returns  -    zero on success, non-zero on 'stop processing options'.	*/
/*=========================================================================*/

/*=========================================================================*/
int DHCPGetOptionInfo(unsigned char *dhcpOptCodes, int dhcpOptCodeCnt, 
		DHCPInfoCallBack *dhcpInfoCB, void *context);
/* Calls dhcpInfoCB once for each requested option in dhcpOptCodes.			*/
/*                                                                         */
/* Returns  -    zero on success, non-zero on failure                      */
/*=========================================================================*/

/*=========================================================================*/
int DHCPParseSLPTags(int tag, void *optdata, size_t optdatasz, void *context);
/* Callback routined tests each DA discovered from DHCP and add it to the	*/
/*	DA cache.																					*/
/*                                                                         */
/* Returns: 0 on success, or nonzero to stop being called.						*/
/* This type definition is used as the context block by this callback.		*/

typedef struct _DHCPContext
{
	int addrlistlen;
	int scopelistlen;
	char scopelist[256];
	unsigned char addrlist[256];
} DHCPContext;

/*=========================================================================*/

#endif


