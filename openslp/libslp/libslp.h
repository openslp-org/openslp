/***************************************************************************/
/*									                                       */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */   
/*              Protocol                                                   */
/*                                                                         */
/* File:        libslp.h                                                   */
/*                                                                         */
/* Abstract:    Make all declarations that are used *internally by libslp  */
/*                                                                         */
/***************************************************************************/

#if(!defined LIBSLP_H_INCLUDED)
#define LIBSLP_H_INCLUDED

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


#include <slp_buffer.h>
#include <slp_message.h>
#include <slp_property.h>
#include <slp_xid.h>
#include <slp_network.h>
#include <slp_da.h>
#include <slp_compare.h>


#define MINIMUM_DISCOVERY_INTERVAL  600    /* 5 minutes */
#define MAX_RETRANSMITS             5      /* we'll only re-xmit 5 times! */
#define SLP_FUNCT_DASRVRQST         0x7f   /* fake id used internally */

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
    int             scopelistlen;
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
    unsigned long       sig;
    SLPBoolean          inUse;
    SLPBoolean          isAsync;
    int                 langtaglen;
    char*               langtag;
    SLPHandleCallParams params;
}SLPHandleInfo, *PSLPHandleInfo;


/*=========================================================================*/ 
typedef void* (*ThreadStartProc)(void *);
/*=========================================================================*/ 


/*=========================================================================*/ 
typedef SLPBoolean NetworkRqstRplyCallback(SLPError error, 
					   SLPMessage msg,
					   void* cookie);
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


/*=========================================================================*/
int NetworkConnectToMulticast(struct sockaddr_in* peeraddr);
/*=========================================================================*/


/*=========================================================================*/ 
int NetworkConnectToDA(const char* scopelist,
                       int scopelistlen,
                       struct sockaddr_in* peeradd);
/* Connects to slpd and provides a peeraddr to send to                     */
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
int NetworkConnectToSlpd(struct sockaddr_in* peeraddr);       
/* Connects to slpd and provides a peeraddr to send to                     */
/*                                                                         */
/* peeraddr         (OUT) pointer to receive the connected DA's address    */
/*                                                                         */
/* Returns          Connected socket or -1 if no DA connection can be made */
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
int KnownDADiscover(struct timeval* timeout);
/*=========================================================================*/


/*=========================================================================*/
int KnownDAConnect(const char* scopelist, 
                   int scopelistlen,
                   struct sockaddr_in* peeraddr,
                   struct timeval* timeout);
/*=========================================================================*/

#endif
