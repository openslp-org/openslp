/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:		slp_dhcp.h                                                 */
/*                                                                         */
/* Abstract:    Implementation for functions that are related              */
/*              to acquiring specific dhcp parameters.                     */
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

#include "slp_dhcp.h"
#include "slp_message.h"
#include "slp_xmalloc.h"

#ifdef _WIN32
#include <winsock.h>
#include <windows.h>
#include <iphlpapi.h>
#define ETIMEDOUT	110
#define ENOTCONN	107
#else
/* non-win32 platforms close sockets with 'close' */
#define closesocket close
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

/* UDP port numbers, server and client. */
#define IPPORT_BOOTPS		67
#define IPPORT_BOOTPC		68

/* BOOTP header op codes */
#define BOOTREQUEST			1
#define BOOTREPLY				2

/* BOOTP header field value maximums */
#define MAXHTYPES				7		/* Number of htypes defined */
#define MAXHADDRLEN			6 		/* Max hw address length in bytes */
#define MAXSTRINGLEN			80		/* Max string length */

/* Some other useful constants */
#define MAX_MACADDR_SIZE	64		/* Max hardware address length */
#define MAX_DHCP_RETRIES	2		/* Max dhcp request retries */

/* timeout values */
#define USECS_PER_MSEC		1000
#define MSECS_PER_SEC		1000
#define USECS_PER_SEC		(USECS_PER_MSEC * MSECS_PER_SEC)
#define INIT_TMOUT_USECS	(250 * USECS_PER_MSEC)

/* DHCP vendor area cookie values */
#define DHCP_COOKIE1			99
#define DHCP_COOKIE2			130
#define DHCP_COOKIE3			83
#define DHCP_COOKIE4			99

/* DHCP Message Types for TAG_DHCP_MSG_TYPE */
#define DHCP_MSG_DISCOVER	1
#define DHCP_MSG_OFFER		2
#define DHCP_MSG_REQUEST	3
#define DHCP_MSG_DECLINE	4
#define DHCP_MSG_ACK			5
#define DHCP_MSG_NAK			6
#define DHCP_MSG_RELEASE	7
#define DHCP_MSG_INFORM		8

/*=========================================================================*/ 
static int dhcpCreateBCSkt(struct sockaddr_in* peeraddr) 
/*	Creates a socket and provides a broadcast addr to which DHCP requests
	should be sent. Also binds the socket to the DHCP client port.

	peeraddr         (OUT) ptr to rcv addr to which DHCP requests are sent

	Returns          Valid socket or -1 if no DA connection can be made
  =========================================================================*/
{
	int sockfd;
#ifdef _WIN32
	BOOL on = 1;
#else
	int on = 1;
#endif

	/* setup dhcp broadcast-to-server address structure */
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
	{
		struct sockaddr_in localaddr;

		localaddr.sin_family = AF_INET;
		localaddr.sin_port = htons(IPPORT_BOOTPC);
		localaddr.sin_addr.s_addr = htonl(INADDR_ANY);

		if(bind(sockfd, (struct sockaddr*)&localaddr, sizeof(localaddr))
				|| setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,
						(char*)&on, sizeof(on)))
		{
			closesocket(sockfd);
			return -1;
		}
		peeraddr->sin_family = AF_INET;
		peeraddr->sin_port = htons(IPPORT_BOOTPS);
		peeraddr->sin_addr.s_addr = htonl(INADDR_BROADCAST);
	}
	return sockfd;
}


/*=========================================================================*/ 
static int dhcpSendRequest(int sockfd, void *buf, size_t bufsz,
		struct sockaddr* peeraddr, struct timeval* timeout)
/*	Sends a buffer to PEERADDR, times out after period specified in TIMEOUT

	Returns  -		zero on success non-zero on failure

	errno				EPIPE error during write
						ETIMEDOUT read timed out
  =========================================================================*/ 
{
	fd_set writefds;
	int xferbytes;
	int flags = 0;

#if defined(MSG_NOSIGNAL)
	flags = MSG_NOSIGNAL;
#endif

	FD_ZERO(&writefds);
	FD_SET(sockfd, &writefds);

	if((xferbytes = select(sockfd + 1, 0, &writefds, 0, timeout)) > 0)
	{
		if((xferbytes = sendto(sockfd, (char*)buf, (int)bufsz, flags, 
				peeraddr, sizeof(struct sockaddr_in))) <= 0)
		{
			errno = EPIPE;
			return -1;
		}
	}
	else if(xferbytes == 0)
	{
		errno = ETIMEDOUT;
		return -1;
	}
	else
	{
		errno = EPIPE;
		return -1;
	}
	return 0;
}


/*=========================================================================*/ 
static int dhcpRecvResponse(int sockfd, void *buf, size_t bufsz,
		struct timeval* timeout)
/*	Receives a DHCP response from a DHCP server. Since DHCP responses are
	broadcasts, we compare XID with received response to ensure we are
	returning the correct response from a prior request.

	Returns  -    	zero on success, non-zero on failure

	errno         	ENOTCONN error during read
						ETIMEDOUT read timed out
  =========================================================================*/ 
{
	int xferbytes;
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);

	if((xferbytes = select(sockfd + 1, &readfds, 0 , 0, timeout)) > 0)
	{
		if((xferbytes = recvfrom(sockfd, (char*)buf, (int)bufsz, 0, 0, 0)) <= 0)
		{
			errno = ENOTCONN;
			return -1;
		}
		return xferbytes;
	}
	else if(xferbytes == 0)
	{
		errno = ETIMEDOUT;
		return -1;
	}
	errno = ENOTCONN;
	return -1;
}

/*=========================================================================*/ 
static int dhcpProcessOptions(unsigned char *data, size_t datasz,
		DHCPInfoCallBack *dhcpInfoCB, void *context)
/*	Calls dhcpInfoCB once for each option returned by the dhcp server.

	Returns	-		zero on success, non-zero on failure

	errno				ENOTCONN error during read
						ETIME read timed out
						ENOMEM out of memory
						EINVAL parse error
  =========================================================================*/ 
{
	int err, taglen;
	unsigned char tag;

	/* validate vendor data header */
	if(datasz < 4 
			|| *data++ != DHCP_COOKIE1 || *data++ != DHCP_COOKIE2 
			|| *data++ != DHCP_COOKIE3 || *data++ != DHCP_COOKIE4) 
		return -1;			/* invalid dhcp response */

	datasz -= 4;			/* account for DHCP cookie values */

	/* validate and process each tag in the vendor data */
	while(datasz-- > 0 && (tag = *data++) != TAG_END) 
	{
		if(tag != TAG_PAD) 
		{
			if(datasz-- > 0 && (taglen = *data++) > (int)datasz) 
				return -1;	/* tag length greater than total data length */

			if(err = dhcpInfoCB(tag, data, taglen, context))
				return err;

			datasz -= taglen;
			data += taglen;
      }
	}
	return 0;	
}

/*=========================================================================*/ 
static int dhcpGetAddressInfo(unsigned char *ipaddr, unsigned char *chaddr, 
		unsigned char *hlen, unsigned char *htype)
/*	return hardware MAC address for specified ip address.

	Returns	-		zero on success, non-zero on failure
  =========================================================================*/ 
{
#ifdef _WIN32

	HMODULE hmod;	
	DWORD (WINAPI *pGetAdaptersInfo)(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen);
	char ipastr[16];

	*hlen = 0;

	sprintf(ipastr, "%d.%d.%d.%d", ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
	
	if((hmod = LoadLibrary("iphlpapi.dll")) != 0)
	{
		if((pGetAdaptersInfo = (DWORD(WINAPI *)(PIP_ADAPTER_INFO,PULONG))
				GetProcAddress(hmod, "GetAdaptersInfo")) != 0)
		{
			DWORD dwerr;
			ULONG bufsz = 0;
			IP_ADAPTER_INFO *aip = 0;
			if((dwerr = (*pGetAdaptersInfo)(aip, &bufsz)) == ERROR_BUFFER_OVERFLOW
					&& (aip = (IP_ADAPTER_INFO *)xmalloc(bufsz)) != 0
					&& (dwerr = (*pGetAdaptersInfo)(aip, &bufsz)) == ERROR_SUCCESS)
			{
				IP_ADAPTER_INFO *pcur;
				for(pcur = aip; pcur && !*hlen; pcur = pcur->Next)
				{
					IP_ADDR_STRING *caddrp;
					for(caddrp = &pcur->IpAddressList; caddrp && !*hlen; caddrp = caddrp->Next)
					{
						if(strcmp(ipastr, caddrp->IpAddress.String) == 0)
						{
							*hlen = pcur->AddressLength;
							*htype = pcur->Type;
							memcpy(chaddr, pcur->Address, pcur->AddressLength);
							break;
						}
					}
				}
				if(!*hlen)	/* couldn't find the one we wanted, just use the first */
				{
					*hlen = aip->AddressLength;
					*htype = aip->Type;
					memcpy(chaddr, aip->Address, aip->AddressLength);
				}
			}
			xfree(aip);
		}
		FreeLibrary(hmod);
	}

#else

	/* how do you get a mac address in unix? */

	*hlen = 0;

	(void)ipaddr;
	(void)chaddr;
	(void)hlen;
	(void)htype;

#endif

	return *hlen? 0: -1;
}


/*=========================================================================*/ 
int DHCPGetOptionInfo(unsigned char *dhcpOptCodes, int dhcpOptCodeCnt, 
		DHCPInfoCallBack *dhcpInfoCB, void *context)
/* Calls dhcpInfoCB once for each requested option in dhcpOptCodes.

	Returns  -    	zero on success, non-zero on failure

	errno         	ENOTCONN error during read
               	ETIME read timed out
               	ENOMEM out of memory
               	EINVAL parse error

	BOOTP/DHCP packet header format:

	Offs	Len	Name		Description
	0		1		opcode	Message opcode: 1 = BOOTREQUEST, 2 = BOOTREPLY
	1		1		htype		Hardware address type (eg., 1 = 10mb ethernet)
	2		1		hlen		Hardware address length (eg., 6 = 10mb ethernet)
	3		1		hops		Client sets to zero, optionally used by relay agents
	4		4		xid		Transaction ID, random number chosen by client
	8		2		secs		Client sets to seconds since start of boot process
	10		2		flags		Bit 0: broadcast response bit
	12		4		ciaddr	Client IP address - only filled if client is bound
	16		4		yiaddr	'your' (Client) IP address
	20		4		siaddr	IP address of next server to use in bootstrap
	24		4		giaddr	Relay agent IP address, used in booting via RA
	28		16		chaddr	Client hardware address
	44		64		sname		Optional server host name, null-terminated string
	108	128	file		Boot file name, null-terminated string
	236	var	options	Optional parameters field

	The options field has the following format:

	Offs	Len	Name		Description
	0		4		cookie	4-byte cookie field: 99.130.83.99 (0x63825363)
	
	Followed by 1-byte option codes and 1-byte option lengths, except
	for the two special fixed length options, pad (0) and end (255).
	Options are defined in slp_dhcp.h as TAG_XXX values. The two we
	really care about here are options TAG_SLP_DA and TAG_SLP_SCOPE,
	78 and 79, respectively. 
	
	The format for TAG_SLP_DA (starting with the tag) is:
	
	Offs	Len	Name		Description
	0		1		tag		TAG_SLP_DA - directory agent ip addresses
	1		1		length	length of remaining data in the option
	2		1		mand		flag: the use of these DA's is mandatory
	3		4		a(0)		4-byte ip address
	...
	3+n*4	4		a(n)		4-byte ip address

	The format for TAG_SLP_SCOPE (starting with the tag) is:

	Offs	Len	Name		Description
	0		1		tag		TAG_SLP_SCOPE - directory scopes to use
	1		1		length	length of remaining data in the option
	2		1		mand		flag: the use of these scopes is mandatory
	3		var	scopes	a null-terminated, comma-separated string of scopes

	The "DHCP Message Type" option must be included in every DHCP message.
	All tags except for TAG_PAD(0) and TAG_END(255) begin with a tag value
	followed by a length of remaining data value. 
  =========================================================================*/ 
{
	UINT32 xid;
	time_t timer;
	struct timeval tv;
	int sockfd, retries;
	struct sockaddr_in sendaddr;
	unsigned char chaddr[MAX_MACADDR_SIZE];
	unsigned char hlen, htype;
	unsigned char sndbuf[512];
	unsigned char rcvbuf[512];
	struct hostent *hep;
	unsigned char *p;
	size_t rcvbufsz;
	char host[256];

	/* Get our IP and MAC addresses */
	if(gethostname(host, (int)sizeof(host))
			|| !(hep = gethostbyname(host))
			|| dhcpGetAddressInfo((unsigned char *)hep->h_addr, 
					chaddr, &hlen, &htype))
		return -1;

	/* get a reasonably random transaction id value */
	xid = (UINT32)time(&timer);

	/* BOOTP request header */
	memset(sndbuf, 0, 236);		/* clear bootp header */
	p = sndbuf;
	*p++ = BOOTREQUEST;			/* opcode */
	*p++ = htype;
	*p++ = hlen;
	*p++;								/* hops */
	ToUINT32(p, xid);
	p += 2 * sizeof(UINT32);	/* xid, secs, flags */
	memcpy(p, hep->h_addr, 4);
	p += 4 * sizeof(UINT32);	/* ciaddr, yiaddr, siaddr, giaddr */
	memcpy(p, chaddr, hlen);
	p += 16 + 64 + 128;			/* chaddr, sname and file */
	*p++ = DHCP_COOKIE1;			/* options, cookies 1-4 */
	*p++ = DHCP_COOKIE2;
	*p++ = DHCP_COOKIE3;
	*p++ = DHCP_COOKIE4;

	/* DHCP Message Type option */
	*p++ = TAG_DHCP_MSG_TYPE;
	*p++ = 1;						/* option length */
	*p++ = DHCP_MSG_INFORM;		/* message type is DHCPINFORM */

	/* DHCP Parameter Request option */
	*p++ = TAG_DHCP_PARAM_REQ;	/* request for DHCP parms */
	*p++ = (unsigned char)dhcpOptCodeCnt;
	memcpy(p, dhcpOptCodes, dhcpOptCodeCnt);
	p += dhcpOptCodeCnt;

	/* DHCP Client Identifier option */
	*p++ = TAG_CLIENT_IDENTIFIER;
	*p++ = hlen + 1;				/* option length */
	*p++ = htype;					/* client id is htype/haddr */
	memcpy(p, chaddr, hlen);
	p += hlen;

	/* End option */
	*p++ = TAG_END;

	/* get a broadcast send/recv socket and address */
	if((sockfd = dhcpCreateBCSkt(&sendaddr)) < 0)
		return -1;

	/* setup select timeout */
	tv.tv_sec = 0;
	tv.tv_usec = INIT_TMOUT_USECS;

	retries = 0;
	srand((unsigned)time(&timer));
	while (retries++ < MAX_DHCP_RETRIES)
	{
		if(dhcpSendRequest(sockfd, sndbuf, p - sndbuf, 
				(struct sockaddr *)&sendaddr, &tv) < 0)
		{
			if (errno != ETIMEDOUT)
			{
				closesocket(sockfd);
				return -1;
			}
		}
		else if((rcvbufsz = dhcpRecvResponse(sockfd, rcvbuf, 
				sizeof(rcvbuf), &tv)) < 0)
		{
			if (errno != ETIMEDOUT)
			{
				closesocket(sockfd);
				return -1;
			}
		}
		else if(rcvbufsz >= 236 && AsUINT32(&rcvbuf[4]) == xid)
			break;

		/* exponential backoff randomized by a 
			uniform number between -1 and 1 */
		tv.tv_usec = tv.tv_usec * 2 + (rand() % 3) - 1;
		tv.tv_sec = tv.tv_usec / USECS_PER_SEC;
		tv.tv_usec %= USECS_PER_SEC;
	}
	closesocket(sockfd);
	return dhcpProcessOptions(rcvbuf + 236, rcvbufsz - 236, 
			dhcpInfoCB, context);
}
