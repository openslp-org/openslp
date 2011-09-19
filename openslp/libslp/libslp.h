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

/** Declarations used internally by libslp.
 *
 * @file       libslp.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    LibSLPCode
 */

#ifndef LIBSLP_H_INCLUDED
#define LIBSLP_H_INCLUDED

/*!@defgroup LibSLPCode User Agent
 * @{
 */

#include "slp_types.h"
#include "slp_buffer.h"
#include "slp_linkedlist.h"
#include "slp_socket.h"
#include "slp_atomic.h"
#include "slp_thread.h"
#include "slp_debug.h"
#include "slp_spi.h"
#include "slp_auth.h"

#define MINIMUM_DISCOVERY_INTERVAL  300    /* 5 minutes */
#define MAX_RETRANSMITS             5      /* we'll only re-xmit 5 times! */
#define SLP_FUNCT_DASRVRQST         0x7f   /* fake id used internally */

typedef enum
{
   SLPREG = 0,
   SLPDEREG,
   SLPFINDSRVS,
   SLPFINDSRVTYPES,
   SLPFINDATTRS,
   SLPDELATTRS
} SLPCallType;

/** Used to collate Service URLS.
 */
typedef struct _SLPSrvUrlCollatedItem
{
   SLPListItem listitem;         /*!< Makes this a list item. */
   char * srvurl;                /*!< The item's service URL. */
   unsigned short lifetime;      /*!< The item's lifetime. */
} SLPSrvUrlCollatedItem;

/** Used to pass all user parameters for "registration" requests.
 */
typedef struct _SLPRegParams
{
   uint16_t lifetime;            /*!< The desired registration lifetime. */
   int fresh;                    /*!< New or renewed registration. */
   size_t urllen;                /*!< The length of @e url in bytes. */
   const char * url;             /*!< The service: URL to register. */
   size_t srvtypelen;            /*!< The length of @e srvtype in bytes. */
   const char * srvtype;         /*!< The service type to register. */
   size_t scopelistlen;          /*!< The length of @e scopelist in bytes. */
   const char * scopelist;       /*!< The scopes in which to register. */
   size_t attrlistlen;           /*!< The length of @e attrlist in bytes. */
   const char * attrlist;        /*!< The list of attributes to register. */
   SLPRegReport * callback;      /*!< The users's results callback function. */
   void * cookie;                /*!< The users's opaque pass-through data. */
} SLPRegParams, * PSLPRegParams;

/** Used to pass all user parameters for "deregistration" requests.
 */
typedef struct _SLPDeRegParams
{
   size_t scopelistlen;          /*!< The length of @e scopelist in bytes. */
   const char * scopelist;       /*!< The scopes to deregister from. */
   size_t urllen;                /*!< The length of @e url in bytes. */
   const char * url;             /*!< The service: URL to deregister. */
   SLPRegReport * callback;      /*!< The users's results callback function. */
   void * cookie;                /*!< The users's opaque pass-through data. */
} SLPDeRegParams, * PSLPDeRegParams;

/** Used to pass all user parameters for "find service types" requests.
 */
typedef struct _SLPFindSrvTypesParams
{
   size_t namingauthlen;         /*!< The length of @e namingauth in bytes. */
   const char * namingauth;      /*!< The naming authority to search type in. */
   size_t scopelistlen;          /*!< The length of @e scopelist in bytes. */
   const char * scopelist;       /*!< The scopes in which to search for types. */
   SLPSrvTypeCallback * callback;/*!< The users's results callback function. */
   void * cookie;                /*!< The users's opaque pass-through data. */
} SLPFindSrvTypesParams, * PSLPFindSrvTypesParams;

/** Used to pass all user parameters for "find services" requests.
 */
typedef struct _SLPFindSrvsParams
{
   size_t srvtypelen;            /*!< The length of @e srvtype in bytes. */
   const char * srvtype;         /*!< The service type to locate. */
   size_t scopelistlen;          /*!< The length of @e scopelist in bytes. */
   const char * scopelist;       /*!< The scopes in which to search. */
   size_t predicatelen;          /*!< The length of @e predicate in bytes. */
   const char * predicate;       /*!< The predicate associated with the find. */
   SLPSrvURLCallback * callback; /*!< The user's results callback function. */
   void * cookie;                /*!< The users's opaque pass-through data. */
} SLPFindSrvsParams, * PSLPFindSrvsParams;

/** Used to pass all user parameters for "find attributes" requests.
 */
typedef struct _SLPFindAttrsParams
{
   size_t urllen;                /*!< The length of @e url in bytes. */
   const char * url;             /*!< The URL for which to find attributes. */
   size_t scopelistlen;          /*!< The length of @e scopelist in bytes. */
   const char * scopelist;       /*!< The associated scope list. */
   size_t taglistlen;            /*!< The length of @e taglist in bytes. */
   const char * taglist;         /*!< The associated attribute tag list. */
   SLPAttrCallback * callback;   /*!< The user's results callback function. */
   void * cookie;                /*!< The users's opaque pass-through data. */
} SLPFindAttrsParams, * PSLPFindAttrsParams;

/** A union of parameter structures. 
 *
 * There is one parameter structure for each public API in the OpenSLP 
 * library. Each parameter structure is designed to hold the parameters
 * passed in that specific handle-based API.
 */
typedef union _SLPHandleCallParams
{
   SLPRegParams reg;             /*!< Registration parameters. */
   SLPDeRegParams dereg;         /*!< Deregistration parameters. */
   SLPFindSrvTypesParams findsrvtypes; /*!< Find Service Type parameters. */
   SLPFindSrvsParams findsrvs;   /*!< Find Service parameters. */
   SLPFindAttrsParams findattrs; /*!< Find Attribute parameters. */
} SLPHandleCallParams;

/** OpenSLP handle state information.
 *
 * This structure holds internal state information relative to an open
 * OpenSLP API handle. In fact, the OpenSLP handle is really one of these
 * structures.
 */
typedef struct _SLPHandleInfo
{
#define SLP_HANDLE_SIG 0xbeeffeed
   unsigned int sig;             /*!< A handle signature value. */
   intptr_t inUse;               /*!< A lock used to control access. */

#ifdef ENABLE_ASYNC_API
   SLPBoolean isAsync;           /*!< Is operation sync or async? */
   SLPThreadHandle th;           /*!< The async operation thread handle. */
#endif

   sockfd_t dasock;              /*!< A cached DA socket. */
   struct sockaddr_storage daaddr; /*!< A cached DA address. */
   char * dascope;               /*!< A cached DA scope. */
   size_t dascopelen;            /*!< The length of @p dascope in bytes. */
   sockfd_t sasock;              /*!< A cached SA socket. */
   struct sockaddr_storage saaddr; /*!< A cached SA address. */
   char * sascope;               /*!< A cached SA scope. */
   size_t sascopelen;            /*!< The length of @p sascope in bytes. */

#ifndef MI_NOT_SUPPORTED
   const char * McastIFList;     /*!< A list of multi-cast interfaces. */
#endif

#ifndef UNICAST_NOT_SUPPORTED
   SLPBoolean dounicast;         /*!< A boolean flag - should I unicast? */
   sockfd_t unicastsock;         /*!< A cached unicast socket. */
   struct sockaddr_storage ucaddr; /*!< A cached unicast address. */
   char * unicastscope;          /*!< The unicast scope list. */
   size_t unicastscopelen;       /*!< The length in bytes of @p unicastscope. */
#endif

   size_t langtaglen;            /*!< The length in bytes of @p langtag. */
   char * langtag;               /*!< The language tag assoicated. */
   int callbackcount;            /*!< The callbacks made in this request. */
   SLPList collatedsrvurls;      /*!< The list of collated service URLs. */
   char * collatedsrvtypes;      /*!< The list of collated service types. */

#ifdef ENABLE_SLPv2_SECURITY
   SLPSpiHandle hspi;            /*!< The Security Parameter Index value. */
#endif

   SLPHandleCallParams params;   /*!< A union of parameter structures. */
} SLPHandleInfo; 

sockfd_t NetworkConnectToSlpd(void * peeraddr);
void NetworkDisconnectDA(SLPHandleInfo * handle);
void NetworkDisconnectSA(SLPHandleInfo * handle);
sockfd_t NetworkConnectToDA(SLPHandleInfo * handle, const char * scopelist,
      size_t scopelistlen, void * peeraddr);
sockfd_t NetworkConnectToSA(SLPHandleInfo * handle, const char * scopelist,
      size_t scopelistlen, void * saaddr);

typedef SLPBoolean NetworkRplyCallback(SLPError errorcode,
      void * peeraddr, SLPBuffer replybuf, void * cookie);

SLPError NetworkRqstRply(sockfd_t sock, void * peeraddr,
      const char * langtag, size_t extoffset, void * buf, char buftype, 
      size_t bufsize, NetworkRplyCallback callback, void * cookie, 
      int isV1); 

SLPError NetworkMcastRqstRply(SLPHandleInfo * handle,
      void * buf, char buftype, size_t bufsize, 
      NetworkRplyCallback callback, void * cookie, int isV1);

#ifndef UNICAST_NOT_SUPPORTED
SLPError NetworkUcastRqstRply(SLPHandleInfo * handle, void * buf, 
      char buftype, size_t bufsize, NetworkRplyCallback callback, 
      void * cookie, int isV1);
SLPError NetworkMultiUcastRqstRply(
                         struct sockaddr_in* destaddr,		// This is an array of addresses
                         const char* langtag,
                         char* buf,
                         char buftype,
                         size_t bufsize,
                         NetworkRplyCallback callback,
                         void * cookie,
                         int isV1);
#endif

sockfd_t KnownDAConnect(SLPHandleInfo * handle, size_t scopelistlen, 
      const char * scopelist, void * peeraddr);

void KnownDABadDA(void * daaddr);
int KnownDAGetScopes(size_t * scopelistlen, char ** scopelist, 
      SLPHandleInfo * handle);
void KnownDAProcessSrvRqst(SLPHandleInfo * handle);
SLPBoolean KnownDASpanningListFromCache(SLPHandleInfo * handle,
                                        int scopelistlen,
                                        const char* scopelist,
                                        struct sockaddr_in** daaddrs);

void KnownDAFreeAll(void);

void PutL16String(uint8_t ** cpp, const char * str, size_t strsz);
size_t SizeofURLEntry(size_t urllen, size_t urlauthlen);
void PutURLEntry(uint8_t ** cpp, uint16_t lifetime, const char * url,
      size_t urllen, const uint8_t * urlauth, size_t urlauthlen);

int LIBSLPPropertyInit(char const * gconffile);
void LIBSLPPropertyCleanup();

/*! @} */

#endif /* LIBSLP_H_INCLUDED */ 

/*=========================================================================*/
