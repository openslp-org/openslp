/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd.h                                                     */
/*                                                                         */
/* Abstract:    Makes all declarations used by slpd. Included by all slpd  */
/*              source files                                               */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/* Copyright (c) 1995, 1999  Caldera Systems, Inc.                         */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify it */
/* under the terms of the GNU Lesser General Public License as published   */
/* by the Free Software Foundation; either version 2.1 of the License, or  */
/* (at your option) any later version.                                     */
/*                                                                         */
/*     This program is distributed in the hope that it will be useful,     */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       */
/*     GNU Lesser General Public License for more details.                 */
/*                                                                         */
/*     You should have received a copy of the GNU Lesser General Public    */
/*     License along with this program; see the file COPYING.  If not,     */
/*     please obtain a copy from http://www.gnu.org/copyleft/lesser.html   */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*     Please submit patches to http://www.openslp.org                     */
/*                                                                         */
/***************************************************************************/

#if(!defined SLPD_H_INCLUDED)
#define SLPD_H_INCLUDED

/* Include platform specific headers files */
#ifdef WIN32
#include "slpd_win32.h"
#else
#include "slpd_unistd.h"
#endif

#ifdef USE_PREDICATES
#include "libslpattr.h"
#include <assert.h>
#endif


/* common includes */
#include "slp_compare.h"
#include "slp_buffer.h"
#include "slp_message.h"
#include "slp_logfile.h"
#include "slp_property.h"
#include "slp_linkedlist.h"
#include "slp_da.h"

#if(!defined MAX_PATH)
#define MAX_PATH    256
#endif

/*=========================================================================*/
/* Misc constants                                                          */
/*=========================================================================*/
#define SLPD_SMALLEST_MESSAGE       18   /* 18 bytes is smallest SLPv2 msg */
#define SLPD_MAX_SOCKETS            64   /* maximum number of sockets */
#define SLPD_COMFORT_SOCKETS        32   /* a comfortable number of sockets */
#define SLPD_MAX_SOCKET_LIFETIME    3600 /* max idle time of socket - 60 min*/
#define SLPD_AGE_INTERVAL           15   /* age every 15 seconds */
#define SLPD_ATTR_RECURSION_DEPTH   50


/*=========================================================================*/
/* Values representing a type or state of a socket                         */
/*=========================================================================*/
#define    SOCKET_PENDING_IO       100
#define    SOCKET_LISTEN           0
#define    SOCKET_CLOSE            1
#define    DATAGRAM_UNICAST        2
#define    DATAGRAM_MULTICAST      3
#define    DATAGRAM_BROADCAST      4
#define    STREAM_CONNECT_IDLE     5
#define    STREAM_CONNECT_BLOCK    6    + SOCKET_PENDING_IO
#define    STREAM_CONNECT_CLOSE    7    + SOCKET_PENDING_IO
#define    STREAM_READ             8    + SOCKET_PENDING_IO
#define    STREAM_READ_FIRST       9    + SOCKET_PENDING_IO
#define    STREAM_WRITE            10   + SOCKET_PENDING_IO
#define    STREAM_WRITE_FIRST      11   + SOCKET_PENDING_IO
#define    STREAM_WRITE_WAIT       12   + SOCKET_PENDING_IO


/* Global variables representing signals */
extern int G_SIGALRM;
extern int G_SIGTERM;
extern int G_SIGHUP;


/*=========================================================================*/
typedef struct _SLPDCommandLine
/* Holds  values of parameters read from the command line                  */
/*=========================================================================*/
{
    char   cfgfile[MAX_PATH];
    char   regfile[MAX_PATH];
    char   logfile[MAX_PATH];
    char   pidfile[MAX_PATH];
    int    action;
    int    detach;
}SLPDCommandLine;


/*=========================================================================*/
extern SLPDCommandLine G_SlpdCommandLine;
/* Global variable containing command line options                         */
/*=========================================================================*/


/*=========================================================================*/
void SLPDPrintUsage();
/* Displays available command line options of SLPD                         */
/*=========================================================================*/


/*=========================================================================*/
int SLPDParseCommandLine(int argc,char* argv[]);
/* Must be called to initialize the command line                           */
/*                                                                         */
/* argc (IN) the argc as passed to main()                                  */
/*                                                                         */
/* argv (IN) the argv as passed to main()                                  */
/*                                                                         */
/* Returns  - zero on success.  non-zero on usage error                    */
/*=========================================================================*/


/*=========================================================================*/
typedef struct _SLPDProperty
/* structure that holds the value of all the properties slpd cares about   */
/*=========================================================================*/
{
    int             myUrlLen;
    const char*     myUrl;
    int             useScopesLen;
    const char*     useScopes; 
    int             DAAddressesLen;
    const char*     DAAddresses;
    unsigned long   DATimestamp;  /* here for convenience */
    int             interfacesLen;
    const char*     interfaces; 
    int             localeLen;
    const char*     locale;
    int             isBroadcastOnly;
    int             passiveDADetection;
    int             activeDADetection; 
    int             activeDiscoveryXmits;
    int             nextActiveDiscovery;
    int             nextPassiveDAAdvert;
    int             multicastTTL;
    int             multicastMaximumWait;
    int             unicastMaximumWait;  
    int             randomWaitBound;
    int             maxResults;
    int             traceMsg;
    int             traceReg;
    int             traceDrop;
    int             traceDATraffic;
    int             isDA;
}SLPDProperty;


/*=========================================================================*/
extern SLPDProperty G_SlpdProperty;
/* Global variable that holds all of the properties that slpd cares about  */
/*=========================================================================*/


/*=========================================================================*/
void SLPDPropertyInit(const char* conffile);
/* Called to initialize slp properties.  Reads .conf file, etc.            */
/*                                                                         */
/* conffile (IN) the path of the configuration file to use                 */ 
/*=========================================================================*/

#ifdef DEBUG
/*=========================================================================*/
void SLPDPropertyDeinit();
/*=========================================================================*/
#endif

/*=========================================================================*/
typedef struct _SLPDDatabaseAttr
/* Structure representing the result of a database query for attributes    */
/*=========================================================================*/
{
    int   attrlistlen;
    const char* attrlist;
    /* TODO: we might need some authblock storage here */
}SLPDDatabaseAttr;


/*=========================================================================*/
typedef struct _SLPDDatabaseSrvUrl
/* Structure representing the result of a database query for services      */
/*=========================================================================*/
{
    int     lifetime;
    int     urllen;
    char*   url;
    /* TODO: we might need some authblock storage here */
}SLPDDatabaseSrvUrl;


/*=========================================================================*/
typedef struct _SLPDDatabaseSrvType
/* Structure representing the result of a database query for type          */
/*=========================================================================*/
{
    int     typelen;
    char*   type;
    /* TODO: we might need some authblock storage here */
}SLPDDatabaseSrvType;


/*=========================================================================*/
typedef struct _SLPDDatabaseEntry
/* Structure representing an entry in slpd database (linked list)          */
/*=========================================================================*/
{ 
    SLPListItem         listitem;
    char*               langtag;
    int                 langtaglen;
    int                 lifetime;
    int                 islocal;
    int                 urllen;
    char*               url;
    int                 scopelistlen;
    char*               scopelist;
    int                 srvtypelen;
    char*               srvtype;
    #ifdef USE_PREDICATES
    SLPAttributes		attr;
    #endif
    int                 attrlistlen;
    char*               attrlist;
    int                 partiallistlen;
    char*               partiallist;

    /* TODO: we might need some authblock storage here */
}SLPDDatabaseEntry;


/*=========================================================================*/
int SLPDDatabaseInit(const char* regfile);
/* Initialize the database with registrations from a regfile.              */
/*                                                                         */
/* regfile  (IN)    the regfile to register.                               */
/*                                                                         */
/* Returns  - zero on success or non-zero on error.                        */
/*=========================================================================*/


/*=========================================================================*/
int SLPDKnownDADeinit();
/* Deinitializes the KnownDA list.  Removes all entries and deregisters    */
/* all services.                                                           */
/*                                                                         */
/* returns  zero on success, Non-zero on failure                           */
/*=========================================================================*/


#ifdef DEBUG
/*=========================================================================*/
void SLPDDatabaseDeinit();
/* De-initialize the database.  Free all resources taken by registrations  */
/*=========================================================================*/
#endif

/*=========================================================================*/
SLPDDatabaseEntry *SLPDDatabaseEntryAlloc();
/* Allocates and initializes a database entry.                             */
/*                                                                         */
/* Returns  - New database entry. If there is not enough memory, null (0)  */
/*            is returned.                                                 */
/*=========================================================================*/


/*=========================================================================*/
void SLPDDatabaseEntryFree(SLPDDatabaseEntry* entry);
/* Free all resource related to the specified entry                        */
/*=========================================================================*/


/*=========================================================================*/
void SLPDDatabaseAge(int seconds);
/* Agea the database entries and clears new and deleted entry lists        */
/*                                                                         */
/* seconds  (IN) the number of seconds to age each entry by                */
/*                                                                         */
/* Returns  - None                                                         */
/*=========================================================================*/


/*=========================================================================*/
int SLPDDatabaseEnum(void** handle,
                     SLPDDatabaseEntry** entry);
/* Enumerate through all entries of the database                           */
/*                                                                         */
/* handle (IN/OUT) pointer to opaque data that is used to maintain         */
/*                 enumerate entries.  Pass in a pointer to NULL to start  */
/*                 enumeration.                                            */
/*                                                                         */
/* entry (OUT) pointer to an entry structure pointer that will point to    */
/*             the next entry on valid return                              */
/*                                                                         */
/* returns: >0 if end of enumeration, 0 on success, <0 on error            */
/*=========================================================================*/


/*=========================================================================*/
int SLPDDatabaseReg(SLPSrvReg* srvreg,
                    int fresh,
                    int islocal);
/* Add a service registration to the database                              */
/*                                                                         */
/* srvreg   -   (IN) pointer to the SLPSrvReg to be added to the database  */
/*                                                                         */
/* fresh    -   (IN) pass in nonzero if the registration is fresh.         */
/*                                                                         */
/* islocal -    (IN) pass in nonzero if the registration is local to this  */
/*              machine                                                    */
/*                                                                         */
/* Returns  -   Zero on success.  non-zero on error                        */
/*                                                                         */
/* NOTE:        All registrations are treated as fresh regardless of the   */
/*              setting of the fresh parameter                             */
/*=========================================================================*/


/*=========================================================================*/
int SLPDDatabaseDeReg(SLPSrvDeReg* srvdereg);
/* Remove a service registration from the database                         */
/*                                                                         */
/* regfile  -   (IN) filename of the registration file to read into the    */
/*              database. Pass in NULL for no file.                        */
/*                                                                         */
/* Returns  -   Zero on success.  Non-zero if syntax error in registration */
/*              file.                                                      */
/*=========================================================================*/


/*=========================================================================*/
int SLPDDatabaseFindSrv(SLPSrvRqst* srvrqst, 
                        SLPDDatabaseSrvUrl* result,
                        int count);
/* Find services in the database                                           */
/*                                                                         */
/* srvrqst  (IN) the request to find.                                      */
/*                                                                         */
/* result   (OUT) pointer to an array of result structures that receives   */
/*                results                                                  */
/*                                                                         */
/* count    (IN)  number of elements in the result array                   */
/*                                                                         */
/* Returns  - The number of services found or <0 on error.  If the number  */
/*            of services found is exactly equal to the number of elements */
/*            in the array, the call may be repeated with a larger array.  */
/*=========================================================================*/


/*=========================================================================*/
int SLPDDatabaseFindType(SLPSrvTypeRqst* srvtyperqst, 
                         SLPDDatabaseSrvType* result,
                         int count);
/* Find service types                                                      */
/*                                                                         */
/* srvtyperqst  (IN) the request to find.                                  */
/*                                                                         */
/* result   (OUT) pointer to an array of result structures that receives   */
/*                results                                                  */
/*                                                                         */
/* count    (IN)  number of elements in the result array                   */
/*                                                                         */
/* Returns  - The number of services found or <0 on error.  If the number  */
/*            of services found is exactly equal to the number of elements */
/*            in the array, the call may be repeated with a larger array.  */
/*=========================================================================*/


/*=========================================================================*/
int SLPDDatabaseFindAttr(SLPAttrRqst* attrrqst, 
                         SLPDDatabaseAttr* result);
/* Find attributes                                                         */
/*                                                                         */
/* srvtyperqst  (IN) the request to find.                                  */
/*                                                                         */
/* result   (OUT) pointer to a result structure that receives              */
/*                results                                                  */
/*                                                                         */
/* Returns  -   1 on success, zero if not found, negative on error         */
/*=========================================================================*/


/*=========================================================================*/
SLPDDatabaseEntry* SLPDRegFileReadEntry(FILE* fd, SLPDDatabaseEntry** entry);
/* Reads an entry SLPDDatabase entry from a file                           */
/*                                                                         */
/* fd       (IN) file to read from                                         */
/*                                                                         */
/* entry    (OUT) Address of a pointer that will be set to the location of */
/*                a dynamically allocated SLPDDatabase entry.  The entry   */
/*                must be freed                                            */
/*                                                                         */
/* Returns  *entry or null on error.                                       */
/*=========================================================================*/


/*=========================================================================*/
typedef struct _SLPDSocket
/* Structure representing a socket                                         */
/*=========================================================================*/
{
    SLPListItem         listitem;    
    int                 fd;
    time_t              age;  /* in seconds */    
    int                 state;
    struct sockaddr_in  peeraddr;
    
    /* Incoming socket stuff */
    SLPBuffer           recvbuf;
    SLPBuffer           sendbuf;

    /* Outgoing socket stuff */
    SLPDAEntry*         daentry;
    SLPList             sendlist;
}SLPDSocket;


/*==========================================================================*/
SLPDSocket* SLPDSocketCreateConnected(struct in_addr* addr);
/*                                                                          */
/* addr - (IN) the address of the peer to connect to                        */
/*                                                                          */
/* Returns: A connected socket or a socket in the process of being connected*/
/*          if the socket was connected the SLPDSocket->state will be set   */
/*          to writable.  If the connect would block, SLPDSocket->state will*/
/*          be set to connect.  Return NULL on error                        */
/*==========================================================================*/


/*==========================================================================*/
SLPDSocket* SLPDSocketCreateListen(struct in_addr* peeraddr);
/*                                                                          */
/* peeraddr - (IN) the address of the peer to connect to                    */
/*                                                                          */
/* type (IN) DATAGRAM_UNICAST, DATAGRAM_MULTICAST, DATAGRAM_BROADCAST       */
/*                                                                          */
/* Returns: A listening socket. SLPDSocket->state will be set to            */
/*          SOCKET_LISTEN.   Returns NULL on error                          */
/*==========================================================================*/


/*==========================================================================*/
SLPDSocket* SLPDSocketCreateDatagram(struct in_addr* peeraddr, int type);
/* peeraddr - (IN) the address of the peer to connect to                    */
/*                                                                          */
/* type - (IN) the type of socket to create DATAGRAM_UNICAST,               */
/*             DATAGRAM_MULTICAST, or DATAGRAM_BROADCAST                    */ 
/* Returns: A datagram socket SLPDSocket->state will be set to              */
/*          DATAGRAM_UNICAST, DATAGRAM_MULTICAST, or DATAGRAM_BROADCAST     */
/*==========================================================================*/


/*==========================================================================*/
SLPDSocket* SLPDSocketCreateBoundDatagram(struct in_addr* myaddr,
                                          struct in_addr* peeraddr,
                                          int type);
/* myaddr - (IN) the address of the interface to join mcast on              */                                                                          
/*                                                                          */
/* peeraddr - (IN) the address of the peer to connect to                    */
/*                                                                          */
/* type (IN) DATAGRAM_UNICAST, DATAGRAM_MULTICAST, DATAGRAM_BROADCAST       */
/*                                                                          */
/* Returns: A datagram socket SLPDSocket->state will be set to              */
/*          DATAGRAM_UNICAST, DATAGRAM_MULTICAST, or DATAGRAM_BROADCAST     */
/*==========================================================================*/


/*=========================================================================*/
SLPDSocket* SLPDSocketAlloc();
/* Allocate memory for a new SLPDSocket.                                   */
/*                                                                         */
/* Returns: pointer to a newly allocated SLPDSocket, or NULL if out of     */
/*          memory.                                                        */
/*=========================================================================*/


/*=========================================================================*/
void SLPDSocketFree(SLPDSocket* sock);
/* Frees memory associated with the specified SLPDSocket                   */
/*                                                                         */
/* sock (IN) pointer to the socket to free                                 */
/*=========================================================================*/


/*=========================================================================*/
void SLPDIncomingAge(time_t seconds);
/* Age the sockets in the incoming list by the specified number of seconds.*/
/*                                                                         */
/* seconds (IN) seconds to age each entry of the list                      */
/*=========================================================================*/


/*=========================================================================*/
void SLPDIncomingHandler(int* fdcount,
                         fd_set* readfds,
                         fd_set* writefds);
/* Handles all outgoing requests that are pending on the specified file    */
/* discriptors                                                             */
/*                                                                         */
/* fdcount  (IN/OUT) number of file descriptors marked in fd_sets          */
/*                                                                         */
/* readfds  (IN) file descriptors with pending read IO                     */
/*                                                                         */
/* writefds  (IN) file descriptors with pending read IO                    */
/*=========================================================================*/


/*=========================================================================*/
int SLPDIncomingInit();
/* Initialize incoming socket list to have appropriate sockets for all     */
/* network interfaces                                                      */
/*                                                                         */
/* Returns  Zero on success non-zero on error                              */
/*=========================================================================*/


/*=========================================================================*/
int SLPDIncomingDeinit();
/* Deinitialize incoming socket list to have appropriate sockets for all   */
/* network interfaces                                                      */
/*                                                                         */
/* Returns  Zero on success non-zero on error                              */
/*=========================================================================*/


/*=========================================================================*/
extern SLPList G_IncomingSocketList;
/*=========================================================================*/


/*=========================================================================*/
void SLPDOutgoingAge(time_t seconds);
/* Age the sockets in the outgoing list by the specified number of seconds.*/
/*                                                                         */
/* seconds (IN) seconds to age each entry of the list                      */
/*=========================================================================*/


/*=========================================================================*/
void SLPDOutgoingHandler(int* fdcount,
                         fd_set* readfds,
                         fd_set* writefds);

/* Handles all incoming requests that are pending on the specified file    */
/* discriptors                                                             */
/*                                                                         */
/* fdcount  (IN/OUT) number of file descriptors marked in fd_sets          */
/*                                                                         */
/* readfds  (IN) file descriptors with pending read IO                     */
/*                                                                         */
/* writefds  (IN) file descriptors with pending read IO                    */
/*=========================================================================*/


/*=========================================================================*/
void SLPDOutgoingDatagramWrite(SLPDSocket* sock);
/* Add a ready to write outgoing datagram socket to the outgoing list.     */
/* The datagram will be written then sit in the list until it ages out     */
/* (after  net.slp.unicastMaximumWait)                                     */
/*                                                                         */
/* sock (IN) the socket that will belong on the outgoing list              */
/*=========================================================================*/


/*=========================================================================*/
SLPDSocket* SLPDOutgoingConnect(struct in_addr* addr);
/* Get a pointer to a connected socket that is associated with the         */
/* outgoing socket list.  If a connected socket already exists on the      */
/* outgoing list, a pointer to it is returned, otherwise a new connection  */
/* is made and added to the outgoing list                                  */
/*                                                                         */
/* addr (IN) the address of the peer a connection is desired for           */
/*                                                                         */
/* returns: pointer to socket or null on error                             */
/*=========================================================================*/


/*=========================================================================*/
int SLPDOutgoingInit();
/* Initialize outgoing socket list to have appropriate sockets for all     */
/* network interfaces                                                      */
/*                                                                         */
/* Returns  Zero on success non-zero on error                              */
/*=========================================================================*/


/*=========================================================================*/
int SLPDOutgoingDeinit(int graceful);
/* Deinitialize incoming socket list to have appropriate sockets for all   */
/* network interfaces                                                      */
/*                                                                         */
/* graceful (IN) Do not close sockets with pending writes                  */
/*                                                                         */
/* Returns  Zero on success non-zero when pending writes remain            */
/*=========================================================================*/


/*=========================================================================*/
extern SLPList G_OutgoingSocketList;
/*=========================================================================*/


/*=========================================================================*/
int SLPDProcessMessage(struct sockaddr_in* peeraddr,
                       SLPBuffer recvbuf,
                       SLPBuffer* sendbuf);
/* Processes the recvbuf and places the results in sendbuf                 */
/*                                                                         */
/* recvfd   - the socket the message was received on                       */
/*                                                                         */
/* recvbuf  - message to process                                           */
/*                                                                         */
/* sendbuf  - results of the processed message                             */
/*                                                                         */
/* Returns  - zero on success SLP_ERROR_PARSE_ERROR or                     */
/*            SLP_ERROR_INTERNAL_ERROR on ENOMEM.                          */
/*=========================================================================*/


/*=========================================================================*/
void SLPDLogTraceMsg(const char* prefix,
                     struct sockaddr_in* peerinfo,
                     SLPBuffer buf);
/*=========================================================================*/


/*=========================================================================*/
void SLPDLogTraceReg(const char* prefix, SLPDDatabaseEntry* entry);
/*=========================================================================*/


/*=========================================================================*/
void SLPDLogDATrafficMsg(const char* prefix,
                         struct sockaddr_in* peerinfo,
                         SLPMessage daadvert);
/*=========================================================================*/


/*=========================================================================*/
void SLPDLogKnownDA(const char* prefix,
                    SLPDAEntry* daentry);
/*=========================================================================*/


/*=========================================================================*/
int SLPDKnownDAInit();
/* Initializes the KnownDA list.  Removes all entries and adds entries     */
/* that are statically configured.                                         */
/*                                                                         */
/* returns  zero on success, Non-zero on failure                           */
/*=========================================================================*/


/*=========================================================================*/
SLPDAEntry* SLPDKnownDAAdd(struct in_addr* addr,
                           const SLPDAEntry* daentry);
/* Adds a DA to the known DA list if it is new, removes it if DA is going  */
/* down or adjusts entry if DA changed.                                    */
/*                                                                         */
/* addr     (IN) pointer to in_addr of the DA to add                       */
/*                                                                         */
/* pointer (IN) pointer to a daentry to add                                */
/*                                                                         */
/* returns  Pointer to the added or updated entry                          */
/*=========================================================================*/


/*=========================================================================*/
int SLPDKnownDAEntryToDAAdvert(int errorcode,
                               unsigned int xid,
                               const SLPDAEntry* daentry,
                               SLPBuffer* sendbuf);
/* Pack a buffer with a DAAdvert using information from a SLPDAentry       */
/*                                                                         */
/* errorcode (IN) the errorcode for the DAAdvert                           */
/*                                                                         */
/* xid (IN) the xid to for the DAAdvert                                    */
/*                                                                         */
/* daentry (IN) pointer to the daentry that contains the rest of the info  */
/*              to make the DAAdvert                                       */
/*                                                                         */
/* sendbuf (OUT) pointer to the SLPBuffer that will be packed with a       */
/*               DAAdvert                                                  */
/*                                                                         */
/* returns: zero on success, non-zero on error                             */
/*=========================================================================*/


/*=========================================================================*/
int SLPDKnownDAEnum(void** handle,
                    SLPDAEntry** entry);
/* Enumerate through all entries of the database                           */
/*                                                                         */
/* handle (IN/OUT) pointer to opaque data that is used to maintain         */
/*                 enumerate entries.  Pass in a pointer to NULL to start  */
/*                 enumeration.                                            */
/*                                                                         */
/* entry (OUT) pointer to an entry structure pointer that will point to    */
/*             the next entry on valid return                              */
/*                                                                         */
/* returns: >0 if end of enumeration, 0 on success, <0 on error            */
/*=========================================================================*/


/*=========================================================================*/
void SLPDKnownDARemove(SLPDAEntry* daentry);
/* Remove the specified entry from the list of KnownDAs                    */
/*                                                                         */
/* daentry (IN) the entry to remove.                                       */
/*                                                                         */
/* Warning! memory pointed to by daentry will be freed                     */
/*=========================================================================*/


/*=========================================================================*/
void SLPDKnownDAEcho(struct sockaddr_in* peerinfo,
                     SLPMessage msg,
                     SLPBuffer buf);
/* Echo a srvreg message to a known DA                                     */
/*									                                       */
/* peerinfo (IN) the peer that the registration came from                  */    
/*                                                                         */ 
/* msg (IN) the translated message to echo                                 */
/*                                                                         */
/* buf (IN) the message buffer to echo                                     */
/*                                                                         */
/* Returns:  Zero on success, non-zero on error                            */
/*=========================================================================*/


/*=========================================================================*/
void SLPDKnownDAActiveDiscovery(int seconds);
/* Set outgoing socket list to send an active DA discovery SrvRqst         */
/*									                                       */
/* seconds (IN) number seconds that elapsed since the last call to this    */
/*              function                                                   */
/*									                                       */
/* Returns:  none                                                          */
/*=========================================================================*/


/*=========================================================================*/
void SLPDKnownDAPassiveDAAdvert(int seconds, int dadead);
/* Send passive daadvert messages if properly configured and running as    */
/* a DA                                                                    */
/*	                                                                       */
/* seconds (IN) number seconds that elapsed since the last call to this    */
/*              function                                                   */
/*                                                                         */
/* dadead  (IN) nonzero if the DA is dead and a bootstamp of 0 should be   */
/*              sent                                                       */
/*                                                                         */
/* Returns:  none                                                          */
/*=========================================================================*/


/*=========================================================================*/
extern SLPList G_KnownDAList;                                         
/* The list of DAs known to slpd.                                          */
/*=========================================================================*/  


#if(defined USE_PREDICATES)

/*=========================================================================*/
int SLPDPredicateTest(const char* predicate, SLPAttributes attr);
/* Determine whether the specified attribute list satisfies                */
/* the specified predicate                                                 */
/*                                                                         */
/* predicatelen (IN) the length of the predicate string                    */
/*                                                                         */
/* predicate    (IN) the predicate string                                  */
/*                                                                         */
/* attr         (IN) attribute list to test                                */
/*                                                                         */
/* Returns: Zero if there is a match, a positive value if there was not a  */
/*          match, and a negative value if there was a parse error in the  */
/*          predicate string.                                              */
/*=========================================================================*/

#endif /* (defined USE_PREDICATES) */ 


#endif /*(!defined SLPD_H_INCLUDED) */
