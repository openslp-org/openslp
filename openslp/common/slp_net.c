
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

#include "slp_net.h"
#include "slp_xmalloc.h"

#ifdef _WIN32
/* definition for inet_aton() since Microsoft does not have this yet */
#define inet_aton(opt,bind) ((bind)->s_addr = inet_addr(opt))
#endif

/*-------------------------------------------------------------------------*/
int SLPNetGetThisHostname(char** hostfdn, int numeric_only)
/* 
 * Description:
 *    Returns a string represting this host (the FDN) or null. Caller must    
 *    free returned string                                                    
 *
 * Parameters:
 *    hostfdn   (OUT) pointer to char pointer that is set to buffer 
 *                    contining this machine's FDN.  Caller must free
 *                    returned string with call to xfree()
 *    numeric_only (IN) force return of numeric address.  
 *-------------------------------------------------------------------------*/
{
    /* TODO find a better constant for MAX_HOSTNAME */
    #define MAX_HOST_NAME 256

    char            host[MAX_HOST_NAME];
    struct hostent* he;
    struct in_addr  ifaddr;

    *hostfdn = 0;

    if(gethostname(host, MAX_HOST_NAME) == 0)
    {
        he = gethostbyname(host);
        if(he)
        {
            /* if the hostname has a '.' then it is probably a qualified 
             * domain name.  If it is not then we better use the IP address
             */
            if(!numeric_only && strchr(he->h_name,'.'))
            {
                *hostfdn = xstrdup(he->h_name);
            }
            else
            {
                ifaddr.s_addr = *((unsigned long*)he->h_addr);
                *hostfdn = xstrdup(inet_ntoa(ifaddr));
            }
            
        }
    }

    return 0;
}

/*-------------------------------------------------------------------------*/
int SLPNetResolveHostToAddr(const char* host,
                            struct in_addr* addr)
/* 
 * Description:
 *    Returns a string represting this host (the FDN) or null. Caller must    
 *    free returned string                                                    
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
    if(inet_aton(host,addr))
    {
        return 0;
    }

    /* Use resolver */
    he = gethostbyname(host);
    if(he != 0)
    {
        if(he->h_addrtype == AF_INET)
        {
            memcpy(addr,he->h_addr_list[0],sizeof(struct in_addr));
            return 0;
        }
        else
        {
            /* TODO: support other address types */
        }
    }

    return -1;
}

int SLPNetIsIPV6() {
	return(0);
}

int SLPNetIsIPV4() {
    return(0);
}

int SLPNetCompareAddrs(const struct sockaddr_storage *addr1, const struct sockaddr_storage *addr2) {
	return(0);
}


int SLPNetIsMCast(const struct sockaddr_storage *addr) {
	return(0);
}

int SLPNetIsLocal(const struct sockaddr_storage *addr) {
	return(0);
}

int SLPNetSetAddr(struct sockaddr_storage *addr, const int family, const short port, const unsigned char *address, const int addrLen) {
	return(0);
}

int SLPNetCopyAddr(struct sockaddr_storage *dst, const struct sockaddr_storage *src) {
	return(0);
}


