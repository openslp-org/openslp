/***************************************************************************/
/*									                                       */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */   
/*              Protocol                                                   */
/*                                                                         */
/* File:        libslp.h                                                   */
/*                                                                         */
/* Abstract:    Make all declarations that are used *internally by libslp  */
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


#if(!defined LIBSLP_H_INCLUDED)
#define LIBSLP_H_INCLUDED

#ifdef WIN32
#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#endif

#include "../common/slp_buffer.h"
#include "../common/slp_message.h"
#include "../common/slp_property.h"
#include "../common/slp_xid.h"
#include "../common/slp_network.h"
#include "../common/slp_da.h"
#include "../common/slp_compare.h"
#ifdef ENABLE_AUTHENTICATION
#include "../common/slp_auth.h"
#include "../common/slp_spi.h"
#endif

#define MINIMUM_DISCOVERY_INTERVAL  300    /* 5 minutes */
#define MAX_RETRANSMITS             5      /* we'll only re-xmit 5 times! */
#define SLP_FUNCT_DASRVRQST         0x7f   /* fake id used internally */

#if(!defined LIBSLP_CONFFILE)
#ifdef WIN32
#define LIBSLP_CONFFILE "%WINDIR%\\slp.conf"
#define LIBSLP_SPIFILE  "%WINDIR%\\slp.spi"
#else
#define LIBSLP_CONFFILE "/etc/slp.conf"
#define LIBSLP_SPIFILE "/etc/spi.conf"
#endif
#endif

#if (!defined MAX_PATH)
#define MAX_PATH 256
#endif


/*=========================================================================*/
typedef enum _SLPCallType
/*=========================================================================*/
{
    SLPREG = 0,
    SLPDEREG,
    SLPFINDSRVS,
    SLPFINDSRVTYPES,
    SLPFINDATTRS,
    SLPDELATTRS
}SLPCallType;


/*=========================================================================*/
typedef struct _SLPRegParams
/* Used to pass parameters to functions that deals with handle based SLP   */
/* API calls                                                               */
/*=========================================================================*/
{
    int             lifetime;
    int             fresh;
    int             urllen;
    const char*     url;
    int             srvtypelen;
    const char*     srvtype;
    int             scopelistlen;
    const char*     scopelist;
    int             attrlistlen;
    const char*     attrlist;
    SLPRegReport*   callback;
    void*           cookie;
}SLPRegParams,*PSLPRegParams;


/*=========================================================================*/
typedef struct _SLPDeRegParams
/* Used to pass parameters to functions that deals with handle based SLP   */
/* API calls                                                               */
/*=========================================================================*/
{
    int            scopelistlen;
    const char*     scopelist;
    int             urllen;
    const char*     url;
    SLPRegReport*   callback;
    void*           cookie;
}SLPDeRegParams,*PSLPDeRegParams;


/*=========================================================================*/
typedef struct _SLPFindSrvTypesParams
/* Used to pass parameters to functions that deals with handle based SLP   */
/* API calls                                                               */
/*=========================================================================*/
{
    int                 namingauthlen;
    const char*         namingauth;
    int                 scopelistlen;
    const char*         scopelist;
    SLPSrvTypeCallback* callback;
    void*               cookie;
}SLPFindSrvTypesParams,*PSLPFindSrvTypesParams;

/*=========================================================================*/
typedef struct _SLPFindSrvsParams
/* Used to pass parameters to functions that deals with handle based SLP   */
/* API calls                                                               */
/*=========================================================================*/
{
    int                 srvtypelen;
    const char*         srvtype;
    int                 scopelistlen;
    const char*         scopelist;
    int                 predicatelen;
    const char*         predicate;
    SLPSrvURLCallback*  callback;
    void*               cookie;
}SLPFindSrvsParams,*PSLPFindSrvsParams;


/*=========================================================================*/
typedef struct _SLPFindAttrsParams
/* Used to pass parameters to functions that deals with handle based SLP   */
/* API calls                                                               */
/*=========================================================================*/
{
    int                 urllen;
    const char*         url;
    int                 scopelistlen;
    const char*         scopelist;
    int                 taglistlen;
    const char*         taglist;
    SLPAttrCallback*    callback;
    void*               cookie;
}SLPFindAttrsParams,*PSLPFindAttrsParams;

/*=========================================================================*/
typedef union _SLPHandleCallParams
/* Used to pass parameters to functions that deals with handle based SLP   */
/* API calls                                                               */
/*=========================================================================*/
{
    SLPRegParams          reg;
    SLPDeRegParams        dereg;
    SLPFindSrvTypesParams findsrvtypes;
    SLPFindSrvsParams     findsrvs;
    SLPFindAttrsParams    findattrs;
}SLPHandleCallParams, *PSLPHandleCallParams;

    #define SLP_HANDLE_SIG 0xbeeffeed



/*=========================================================================*/
typedef struct _SLPHandleInfo
/* The SLPHandle that is used internally in slplib is actually a pointer to*/
/* a struct _SLPHandleInfo                                                 */
/*=========================================================================*/
{
    unsigned int        sig;
    SLPBoolean          inUse;
    SLPBoolean          isAsync;
    int                 dasock;
    struct sockaddr_in  daaddr;
    char*               dascope;
    int                 dascopelen;
    int                 sasock;
    struct sockaddr_in  saaddr;
    char*               sascope;
    int                 sascopelen;
    int                 langtaglen;
    char*               langtag;
#ifdef ENABLE_AUTHENTICATION
    SLPSpiHandle        hspi;
#endif
    SLPHandleCallParams params;
}SLPHandleInfo, *PSLPHandleInfo; 


#ifdef ENABLE_ASYNC_API
/*=========================================================================*/
typedef void* (*ThreadStartProc)(void *);  
/*=========================================================================*/

/*=========================================================================*/
SLPError ThreadCreate(ThreadStartProc startproc, void *arg);
/* Creates a thread                                                        */
/*                                                                         */
/* startproc    (IN) Address of the thread start procedure.                */
/*                                                                         */
/* arg          (IN) An argument for the thread start procedure.           */
/*                                                                         */
/* Returns      SLPError code                                              */
/*=========================================================================*/
#endif


/*=========================================================================*/
typedef SLPBoolean NetworkRqstRplyCallback(SLPError error, 
                                           SLPMessage msg,
                                           void* cookie);  
/*=========================================================================*/


/*=========================================================================*/
int NetworkConnectToMulticast(struct sockaddr_in* peeraddr);
/*=========================================================================*/


/*=========================================================================*/
int NetworkConnectToSlpd(struct sockaddr_in* peeraddr); 
/* Connects to slpd and provides a peeraddr to send to                     */
/*                                                                         */
/* peeraddr         (OUT) pointer to receive the connected DA's address    */
/*                                                                         */
/* Returns          Connected socket or -1 if no DA connection can be made */
/*=========================================================================*/


/*=========================================================================*/
void NetworkDisconnectDA(PSLPHandleInfo handle);  
/* Called after DA fails to respond                                        */
/*                                                                         */
/* handle   (IN) a handle previously passed to NetworkConnectToDA()        */
/*=========================================================================*/


/*=========================================================================*/
void NetworkDisconnectSA(PSLPHandleInfo handle);  
/* Called after SA fails to respond                                        */
/*                                                                         */
/* handle   (IN) a handle previously passed to NetworkConnectToSA()        */
/*=========================================================================*/


/*=========================================================================*/
int NetworkConnectToDA(PSLPHandleInfo handle,
                       const char* scopelist,
                       int scopelistlen,
                       struct sockaddr_in* peeraddr); 
/* Connects to slpd and provides a peeraddr to send to                     */
/*                                                                         */
/* handle           (IN) SLPHandle info (caches connection reuse info)     */
/*                                                                         */
/* scopelist        (IN) Scope that must be supported by DA. Pass in NULL  */
/*                       for any scope                                     */
/*                                                                         */
/* scopelistlen     (IN) Length of the scope list in bytes.  Ignored if    */
/*                       scopelist is NULL                                 */
/*                                                                         */
/* peeraddr         (OUT) pointer to receive the connected DA's address    */
/*                                                                         */
/* Returns          Connected socket or -1 if no DA connection can be made */
/*=========================================================================*/


/*=========================================================================*/
int NetworkConnectToSA(PSLPHandleInfo handle,
                       const char* scopelist,
                       int scopelistlen,
                       struct sockaddr_in* peeraddr); 
/* Connects to slpd and provides a peeraddr to send to                     */
/*                                                                         */
/* handle           (IN) SLPHandle info  (caches connection info)          */
/*                                                                         */
/* scopelist        (IN) Scope that must be supported by SA. Pass in NULL  */
/*                       for any scope                                     */
/*                                                                         */
/* scopelistlen     (IN) Length of the scope list in bytes.  Ignored if    */
/*                       scopelist is NULL                                 */
/*                                                                         */
/* peeraddr         (OUT) pointer to receive the connected SA's address    */
/*                                                                         */
/* Returns          Connected socket or -1 if no SA connection can be made */
/*=========================================================================*/


/*=========================================================================*/
SLPError NetworkRqstRply(int sock,
                         struct sockaddr_in* peeraddr,
                         const char* langtag,
                         char* buf,
                         char buftype,
                         int bufsize,
                         NetworkRqstRplyCallback callback,
                         void * cookie); 
/* Transmits and receives SLP messages via multicast convergence algorithm */
/*                                                                         */
/* Returns  -    SLP_OK on success                                         */
/*=========================================================================*/


/*=========================================================================*/
int KnownDAConnect(int scopelistlen,
                   const char* scopelist,
                   struct sockaddr_in* peeraddr);
/* Get a connected socket to a DA that supports the specified scope        */
/*                                                                         */
/* scopelistlen (IN) stringlen of the scopelist                            */
/*                                                                         */
/* scopelist (IN) DA must support this scope                               */
/*                                                                         */
/* peeraddr (OUT) the peer that was connected to                           */
/*                                                                         */
/*                                                                         */
/* returns: valid socket file descriptor or -1 if no DA is found           */
/*=========================================================================*/


/*=========================================================================*/
int KnownDABadDA(struct sockaddr_in* peeraddr);
/* Mark a KnownDA as a Bad DA.                                             */
/*                                                                         */
/* peeraddr (IN) address of the bad DA                                     */
/*                                                                         */
/* Returns: zero on success.                                               */
/*=========================================================================*/


/*=========================================================================*/
int KnownDAGetScopes(int* scopelistlen,
                     char** scopelist);
/* Gets a list of scopes from the known DA list                            */
/*                                                                         */
/* scopelistlen (OUT) stringlen of the scopelist                           */
/*                                                                         */
/* scopelist (OUT) NULL terminated list of scopes                          */
/*                                                                         */
/* returns: zero on success, non-zero on failure                           */
/*=========================================================================*/


#endif
