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

/*!@defgroup LibSLPCode User Agent */

/*!@addtogroup LibSLPCode
 * @{
 */

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <time.h>
# include <limits.h>
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
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h> 
# include <ctype.h> 
#endif

#include "slp_buffer.h"
#include "slp_message.h"
#include "slp_property.h"
#include "slp_xid.h"
#include "slp_network.h"
#include "slp_database.h"
#include "slp_compare.h"
#include "slp_xmalloc.h"
#include "slp_parse.h"
#include "slp_iface.h"
#include "slp_xcast.h"
#include "slp_pid.h"

#ifdef ENABLE_SLPv2_SECURITY
# include "slp_auth.h"
# include "slp_spi.h"
#endif

#define MINIMUM_DISCOVERY_INTERVAL  300    /* 5 minutes */
#define MAX_RETRANSMITS             5      /* we'll only re-xmit 5 times! */
#define SLP_FUNCT_DASRVRQST         0x7f   /* fake id used internally */

#ifndef LIBSLP_CONFFILE
# ifdef _WIN32
#  define LIBSLP_CONFFILE "%WINDIR%\\slp.conf"
#  define LIBSLP_SPIFILE  "%WINDIR%\\slp.spi"
# else
#  define LIBSLP_CONFFILE "/etc/slp.conf"
#  define LIBSLP_SPIFILE "/etc/slp.spi"
# endif
#endif

#if (!defined MAX_PATH)
# define MAX_PATH 256
#endif

typedef enum _SLPCallType
{
   SLPREG = 0,
   SLPDEREG,
   SLPFINDSRVS,
   SLPFINDSRVTYPES,
   SLPFINDATTRS,
   SLPDELATTRS
} SLPCallType;

/** Used to colate Service URLS
 */
typedef struct _SLPSrvUrlColatedItem
{
   SLPListItem listitem;   
   /*!< @brief Makes this a list item.
    */
   char * srvurl;          
   /*!< @brief The item's service URL.
    */
   unsigned short lifetime;
   /*!< @brief The item's lifetime.
    */
} SLPSrvUrlColatedItem;

/** Used to pass all user parameters for "registration" requests.
 */
typedef struct _SLPRegParams
{
   int lifetime;
   /*!< The desired registration lifetime.
    */
   int fresh;
   /*!< New or renewed registration.
    */
   int urllen;
   /*!< The length of @e url in bytes.
    */
   const char * url;
   /*!< The service: URL to register.
    */
   int srvtypelen;
   /*!< The length of @e srvtype in bytes.
    */
   const char * srvtype;
   /*!< The service type to register.
    */
   int scopelistlen;
   /*!< The length of @e scopelist in bytes.
    */
   const char * scopelist;
   /*!< The scopes in which to register.
    */
   int attrlistlen;
   /*!< The length of @e attrlist in bytes.
    */
   const char * attrlist;
   /*!< The list of attributes to register.
    */
   SLPRegReport * callback;
   /*!< The users's results callback function.
    */
   void * cookie;
   /*!< The users's opaque pass-through data.
    */
} SLPRegParams, * PSLPRegParams;

/** Used to pass all user parameters for "deregistration" requests.
 */
typedef struct _SLPDeRegParams
{
   int scopelistlen;
   /*!< The length of @e scopelist in bytes.
    */
   const char * scopelist;
   /*!< The scopes to deregister from.
    */
   int urllen;
   /*!< The length of @e url in bytes.
    */
   const char * url;
   /*!< The service: URL to deregister.
    */
   SLPRegReport * callback;
   /*!< The users's results callback function.
    */
   void * cookie;
   /*!< The users's opaque pass-through data.
    */
} SLPDeRegParams, * PSLPDeRegParams;

/** Used to pass all user parameters for "find service types" requests.
 */
typedef struct _SLPFindSrvTypesParams
{
   int namingauthlen;
   /*!< The length of @e namingauth in bytes.
    */
   const char * namingauth;
   /*!< The naming authority to search type in.
    */
   int scopelistlen;
   /*!< The length of @e scopelist in bytes.
    */
   const char * scopelist;
   /*!< The scopes in which to search for types.
    */
   SLPSrvTypeCallback * callback;
   /*!< The users's results callback function.
    */
   void * cookie;
   /*!< The users's opaque pass-through data.
    */
} SLPFindSrvTypesParams, * PSLPFindSrvTypesParams;

/** Used to pass all user parameters for "find services" requests.
 */
typedef struct _SLPFindSrvsParams
{
   int srvtypelen;
   /*!< The length of @e url in bytes.
    */
   const char * srvtype;
   /*!< The URL for which to find attributes.
    */
   int scopelistlen;
   /*!< The length of @e scopelist in bytes.
    */
   const char * scopelist;
   /*!< The associated scope list.
    */
   int predicatelen;
   /*!< The length of @e taglist in bytes.
    */
   const char * predicate;
   /*!< The associated attribute tag list.
    */
   SLPSrvURLCallback * callback;
   /*!< The user's results callback function.
    */
   void * cookie;
   /*!< The users's opaque pass-through data.
    */
} SLPFindSrvsParams, * PSLPFindSrvsParams;

/** Used to pass all user parameters for "find attributes" requests.
 */
typedef struct _SLPFindAttrsParams
{
   int urllen;
   /*!< The length of @e url in bytes.
    */
   const char * url;
   /*!< The URL for which to find attributes.
    */
   int scopelistlen;
   /*!< The length of @e scopelist in bytes.
    */
   const char * scopelist;
   /*!< The associated scope list.
    */
   int taglistlen;
   /*!< The length of @e taglist in bytes.
    */
   const char * taglist;
   /*!< The associated attribute tag list.
    */
   SLPAttrCallback * callback;
   /*!< The user's results callback function.
    */
   void * cookie;
   /*!< The users's opaque pass-through data.
    */
} SLPFindAttrsParams, * PSLPFindAttrsParams;

/** A union of parameter structures. 
 *
 * There is one parameter structure for each public API in the OpenSLP 
 * library. Each parameter structure is designed to hold the parameters
 * passed in that specific handle-based API.
 */
typedef union _SLPHandleCallParams
{
   SLPRegParams reg;
   /*!< Registration parameters.
    */
   SLPDeRegParams dereg;
   /*!< Deregistration parameters.
    */
   SLPFindSrvTypesParams findsrvtypes;
   /*!< Find Service Type parameters.
    */
   SLPFindSrvsParams findsrvs;
   /*!< Find Service parameters.
    */
   SLPFindAttrsParams findattrs;
   /*!< Find Attribute parameters.
    */
} SLPHandleCallParams, * PSLPHandleCallParams;

/** OpenSLP handle state information.
 *
 * This structure holds internal state information relative to an open
 * OpenSLP API handle. In fact, the OpenSLP handle is really one of these
 * structures.
 */
typedef struct _SLPHandleInfo
{
#  define SLP_HANDLE_SIG 0xbeeffeed
   unsigned int sig;
   /*!< A handle signature value.
    */
   SLPBoolean inUse;
   /*!< A lock used to control access.
    */
   SLPBoolean isAsync;
   /*!< Is operation sync or async?
    */
   int dasock;
   /*!< A cached DA socket.
    */
   struct sockaddr_storage daaddr;
   /*!< A cached DA address.
    */
   char * dascope;
   /*!< A cached DA scope.
    */
   int dascopelen;
   /*!< The length of @p dascope in bytes.
    */
   int sasock;
   /*!< A cached SA socket.
    */
   struct sockaddr_storage saaddr;
   /*!< A cached SA address.
    */
   char * sascope;
   /*!< A cached SA scope.
    */
   int sascopelen;
   /*!< The length of @p sascope in bytes.
    */

#ifndef MI_NOT_SUPPORTED
   const char * McastIFList;
   /*!< A list of multi-cast interfaces.
    */
#endif /* MI_NOT_SUPPORTED */

#ifndef UNICAST_NOT_SUPPORTED
   int dounicast;
   /*!< A boolean flag - should I unicast?
    */
   int unicastsock;
   /*!< A cached unicast socket.
    */
   struct sockaddr_storage unicastaddr;
   /*!< A cached unicast address.
    */
   char * unicastscope;
   /*!< The unicast scope list.
    */
   int unicastscopelen;
   /*!< The length in bytes of @p unicastscope.
    */
#endif

   int langtaglen;
   /*!< The length in bytes of @p langtag.
    */
   char * langtag;
   /*!< The language tag assoicated.
    */
   int callbackcount;
   /*!< The callbacks made in this request.
    */
   SLPList collatedsrvurls;
   /*!< The list of collated service URLs.
    */
   char * collatedsrvtypes;
   /*!< The list of collated service types.
    */

#ifdef ENABLE_SLPv2_SECURITY
   SLPSpiHandle hspi;
   /*!< The Security Parameter Index value.
    */
#endif

   SLPHandleCallParams params;
   /*!< A union of parameter structures.
    */
} SLPHandleInfo, * PSLPHandleInfo; 

#ifdef ENABLE_ASYNC_API
typedef void* (*ThreadStartProc)(void *);  
SLPError ThreadCreate(ThreadStartProc startproc, void *arg);
#endif

int NetworkConnectToMulticast(struct sockaddr_storage* peeraddr);

int NetworkConnectToSlpd(struct sockaddr_storage* peeraddr); 

void NetworkDisconnectDA(PSLPHandleInfo handle);  

void NetworkDisconnectSA(PSLPHandleInfo handle);  

int NetworkConnectToDA(PSLPHandleInfo handle,
                       const char* scopelist,
                       int scopelistlen,
                       struct sockaddr_storage* peeraddr); 

int NetworkConnectToSA(PSLPHandleInfo handle,
                       const char* scopelist,
                       int scopelistlen,
                       struct sockaddr_storage* peeraddr); 

typedef SLPBoolean NetworkRplyCallback(SLPError errorcode,
                                       struct sockaddr_storage* peerinfo,
                                       SLPBuffer replybuf,
                                       void* cookie);  

SLPError NetworkRqstRply(int sock,
                         struct sockaddr_storage* peeraddr,
                         const char* langtag,
                         int extoffset,
                         char* buf,
                         char buftype,
                         int bufsize,
                         NetworkRplyCallback callback,
                         void * cookie); 

#ifndef MI_NOT_SUPPORTED
SLPError NetworkMcastRqstRply(PSLPHandleInfo handle,
#else
SLPError NetworkMcastRqstRply(const char* langtag,
#endif /* MI_NOT_SUPPORTED */
                              char* buf,
                              char buftype,
                              int bufsize,
                              NetworkRplyCallback callback,
                              void * cookie);

#ifndef UNICAST_NOT_SUPPORTED
SLPError NetworkUcastRqstRply(PSLPHandleInfo handle,
                              char* buf,
                              char buftype,
                              int bufsize,
                              NetworkRplyCallback callback,
                              void * cookie);
#endif
			      
int KnownDAConnect(PSLPHandleInfo handle,
                   int scopelistlen,
                   const char* scopelist,
                   struct sockaddr_storage* peeraddr);

void KnownDABadDA(struct sockaddr_storage* daaddr);

int KnownDAGetScopes(int* scopelistlen,
#ifndef MI_NOT_SUPPORTED
                     char** scopelist,
                     PSLPHandleInfo handle);
#else
                     char** scopelist);
#endif /* MI_NOT_SUPPORTED */

void KnownDAProcessSrvRqst(PSLPHandleInfo handle);

#ifdef DEBUG
void KnownDAFreeAll(void);
#endif

/*! @} */

#endif /* LIBSLP_H_INCLUDED */ 

/*=========================================================================*/
