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

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>   

#include <slp_buffer.h>
#include <slp_message.h>
#include <slp_property.h>

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
    SLPRegParams        reg;
    SLPDeRegParams      dereg;
    SLPFindSrvsParams   findsrvs;
    SLPFindAttrsParams  findattrs;
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
    int                 slpdsock;
    struct sockaddr     slpdaddr;
    SLPHandleCallParams params;
}SLPHandleInfo, *PSLPHandleInfo;


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


/*=========================================================================*/ 
int NetworkGetSlpdSocket(struct sockaddr* peeraddr, int peeraddrlen);
/* Connects to slpd.  Will not block.                                      */
/*                                                                         */
/* peeraddr     (OUT) receives the address of the socket                   */
/*                                                                         */
/* peeraddrlen  (OUT) receives the length of the socket address            */
/*                                                                         */
/* Returns  -   -1 if connection can not be made                           */
/*=========================================================================*/ 


/*=========================================================================*/ 
void NetworkCloseSlpdSocket();
/*=========================================================================*/ 


/*=========================================================================*/ 
SLPError NetworkSendMessage(int sockfd,
                            SLPBuffer buf,
                            struct timeval* timeout,
                            struct sockaddr* peeraddr,
                            int peeraddrlen);
/* Sends a message                                                         */
/*                                                                         */
/* Returns  -    SLP_OK, SLP_NETWORK_TIMEOUT, SLP_NETWORK_ERROR, or        */
/*               SLP_PARSE_ERROR.                                          */
/*=========================================================================*/ 



/*=========================================================================*/ 
SLPError NetworkRecvMessage(int sockfd,
                            SLPBuffer buf,
                            struct timeval* timeout,
                            struct sockaddr* peeraddr,
                            int* peeraddrlen);
/* Receives a message                                                      */
/*                                                                         */
/* Returns  -    SLP_OK, SLP_NETWORK_TIMEOUT, SLP_NETWORK_ERROR, or        */
/*               SLP_PARSE_ERROR.                                          */
/*=========================================================================*/ 



/*=========================================================================*/ 
int NetworkConnectToDA(struct sockaddr_in* peeraddr);
/*=========================================================================*/ 


/*=========================================================================*/ 
int NetworkConnectToSlpMulticast(struct sockaddr_in* peeraddr);
/* Creates a socket and connects it to the SLP multicast address           */
/*                                                                         */
/* Returns  - Valid file descriptor on success, -1 on failure w/ errno set.*/
/*=========================================================================*/ 


/*=========================================================================*/ 
int NetworkConnectToSlpBroadcast(struct sockaddr_in* peeraddr);
/* Creates a socket and connects it to the SLP multicast address           */
/*                                                                         */
/* Returns  - Valid file descriptor on success, -1 on failure w/ errno set.*/
/*=========================================================================*/ 


/*=========================================================================*/
void XidSeed();
/* Seeds the XID generator.  Should only be called 1 time per process!     */
/* currently called when the first handle is opened.                       */
/*=========================================================================*/


/*=========================================================================*/
unsigned short XidGenerate();
/*                                                                         */
/* Returns: A hopefully unique 16-bit value                                */
/*=========================================================================*/

#endif
