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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h> 
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 

/* common includes */
#include <slp_compare.h>
#include <slp_buffer.h>
#include <slp_message.h>
#include <slp_logfile.h>
#include <slp_property.h>
#include <slp_linkedlist.h>


#if(!defined MAX_PATH)
#define MAX_PATH    256
#endif
#define SLPDPROCESS_RESULT_COUNT    256  
#define SLPD_MAX_SOCKETS            128  
#define SLPD_AGE_TIMEOUT            15   /* age every 15 seconds */


/*=========================================================================*/
typedef struct _SLPDCommandLine
/* Holds  values of parameters read from the command line                  */
/*=========================================================================*/
{
    char   cfgfile[MAX_PATH];
    char   regfile[MAX_PATH];
    char   logfile[MAX_PATH];
    char   pidfile[MAX_PATH];
    int    detach;
}SLPDCommandLine;


/*=========================================================================*/
extern SLPDCommandLine G_SlpdCommandLine;
/* Global variable containing command line options                         */
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
    int         useScopesLen;
    const char* useScopes; 
    int         DAAddressesLen;
    const char* DAAddresses;
    int         interfacesLen;
    const char* interfaces; 
    int         isBroadcastOnly;
    int         passiveDADetection;
    int         activeDADetection; 
    int         multicastTTL;
    int         multicastMaximumWait;
    int         unicastMaximumWait;  
    int         randomWaitBound;
    int         traceMsg;
    int         traceReg;
    int         traceDrop;
    int         traceDATraffic;
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


/*=========================================================================*/
typedef struct _SLPDDatabaseAttr
/* Structure representing the result of a database query for attributes    */
/*=========================================================================*/
{
    int   attrlen;
    char* attr;
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
    ListItem            listitem;
    pid_t               pid;        /* the pid that registered the entry */
    uid_t               uid;        /* the uid that registered the entry */
    char*               langtag;
    int                 langtaglen;
    int                 lifetime;
    int                 urllen;
    char*               url;
    int                 scopelistlen;
    char*               scopelist;
    int                 srvtypelen;
    char*               srvtype;
    int                 attrlistlen;
    char*               attrlist;
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
void SLPDDatabaseAge(int seconds);
/* Age the database entries                                                */
/*                                                                         */
/* seconds  (IN) the number of seconds to age each entry by                */
/*                                                                         */
/* Returns  - None                                                         */
/*=========================================================================*/


/*=========================================================================*/
int SLPDDatabaseReg(SLPSrvReg* srvreg,
                    int fresh,
                    pid_t pid,
                    uid_t uid);
/* Add a service registration to the database                              */
/*                                                                         */
/* srvreg   -   (IN) pointer to the SLPSrvReg to be added to the database  */
/*                                                                         */
/* fresh    -   (IN) pass in nonzero if the registration is fresh.         */
/*                                                                         */
/* pid      -   (IN) process id of the process that registered the service */
/*                                                                         */
/* uid      -   (IN) user id of the user that registered the service       */
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
                         SLPDDatabaseAttr* result,
                         int count);
/* Find attributes                                                         */
/*                                                                         */
/* srvtyperqst  (IN) the request to find.                                  */
/*                                                                         */
/* result   (OUT) pointer to a result structure that receives              */
/*                results                                                  */
/*                                                                         */
/* count    (IN)  number of elements in the result array                   */
/*                                                                         */
/* Returns  -   >0 on success. 0 if the url of the attrrqst could not be   */
/*              cound and <0 on error.                                     */
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
typedef enum _SLPDPeerType
/*=========================================================================*/
{
    SLPD_PEER_UNKNOWN   = 0,
    SLPD_PEER_LOCAL     = 1,
    SLPD_PEER_REMOTE    = 2
}SLPDPeerType;


/*=========================================================================*/
typedef struct _SLPDPeerInfo
/* Structure representing a socket                                         */
/*=========================================================================*/
{
    SLPDPeerType        peertype;
    struct sockaddr_in  peeraddr;
    socklen_t           peeraddrlen;
    pid_t               peerpid;
    uid_t               peeruid;
    gid_t               peergid;
}SLPDPeerInfo;


/*=========================================================================*/
typedef enum _SLPDSocketState
/* Value representing a type or state of a socket                          */
/*=========================================================================*/
{
    SOCKET_LISTEN       = 0,
    DATAGRAM_UNICAST    = 1,
    DATAGRAM_MULTICAST  = 2,
    STREAM_READ         = 3,
    STREAM_FIRST_READ   = 4,
    STREAM_WRITE        = 5,
    STREAM_FIRST_WRITE  = 6,
    SOCKET_CLOSE        = 7
}SLPDSocketState;


/*=========================================================================*/
typedef struct _SLPDSocket
/* Structure representing a socket                                         */
/*=========================================================================*/
{
    ListItem            listitem;    
    int                 fd;
    time_t              timestamp;      
    SLPDSocketState     state;
    SLPBuffer           recvbuf;
    SLPBuffer           sendbuf;
    SLPDPeerInfo        peerinfo;
}SLPDSocket;


/*=========================================================================*/
typedef struct _SLPDSocketList
/* Structure representing a list of SLPDSockets                            */
/*=========================================================================*/
{
    int         count;
    SLPDSocket*  head;
}SLPDSocketList;


/*=========================================================================*/
SLPDSocket* SLPDSocketListAdd(SLPDSocketList* list, SLPDSocket* addition);  
/* Adds a free()s the specified socket from the specified list             */
/*                                                                         */
/* list     - pointer to the SLPSocketList to add the socket to.           */
/*                                                                         */
/* addition - a pointer to the socket to add                               */
/*                                                                         */
/* Returns  - pointer to the added socket or NULL on error                 */
/*=========================================================================*/


/*=========================================================================*/
SLPDSocket* SLPDSocketListRemove(SLPDSocketList* list, SLPDSocket* sock);
/* Unlinks and free()s the specified socket from the specified list        */
/*                                                                         */
/* list     - pointer to the SLPSocketList to unlink the socket from.      */
/*                                                                         */
/* sock     - pointer to the SLPSocket to unlink to the list               */
/*                                                                         */
/* Returns  - pointer to the unlinked socket                               */
/*=========================================================================*/


/*=========================================================================*/
void SLPDSocketInit(SLPDSocketList* list);
/* Adds SLPSockets (UDP and TCP) for all the interfaces and the loopback   */
/*                                                                         */
/* list     - pointer to SLPSocketList to initialize                       */
/*                                                                         */
/* Returns  - zero on success, -1 on failure.                              */
/*=========================================================================*/


/*=========================================================================*/
void SLPDSocketDeInit(SLPDSocketList* list);
/* Adds SLPSockets (UDP and TCP) for all the interfaces and the loopback   */
/*                                                                         */
/* list     - pointer to SLPSocketList to deinitialize                     */
/*                                                                         */
/* Returns  - zero on success, -1 on failure.                              */
/*=========================================================================*/


/*=========================================================================*/
int SLPDProcessMessage(SLPDPeerInfo* peerinfo,
                       SLPBuffer recvbuf,
                       SLPBuffer sendbuf);
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
		     SLPDPeerInfo* peerinfo,
                     SLPBuffer buf);
/*=========================================================================*/


/*=========================================================================*/
void SLPDLogTraceReg(const char* prefix, SLPDDatabaseEntry* entry);
/*=========================================================================*/

#endif /* (!defined SLPD_H_INCLUDED) */
