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


#if(!defined MAX_PATH)
#define MAX_PATH    256
#endif
#define SLPDPROCESS_RESULT_COUNT    256  
#define SLPD_MAX_SOCKETS            128  
#define SLPD_AGE_TIMEOUT            30   /* age every 30 seconds */


/*=========================================================================*/
typedef struct _SLPDCommandLine
/* Holds  values of parameters read from the command line                  */
/*=========================================================================*/
{
    char   cfgfile[MAX_PATH];
    char   regfile[MAX_PATH];
    char   logfile[MAX_PATH];
    char   pidfile[MAX_PATH];
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
    struct _SLPDDatabaseEntry*   previous;
    struct _SLPDDatabaseEntry*   next;
    
    int                 pid;        /* the pid that registered the entry */
    int                 uid;        /* the uid that registered the entry */
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
                    int pid,
                    int uid);
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
typedef enum _SLPDSocketType
/* Value representing a type or state of a socket                          */
/*=========================================================================*/
{
    TCP_LISTEN              = 0,
    UDP                     = 1,
    UDP_MCAST               = 2,
    TCP_READ                = 3,
    TCP_FIRST_READ          = 4,
    TCP_WRITE               = 5,
    TCP_FIRST_WRITE         = 6,
    SOCKET_CLOSE            = 7
}SLPSocketType;

/*=========================================================================*/
typedef struct _SLPDSocket
/* Structure representing a socket                                         */
/*=========================================================================*/
{
    struct _SLPDSocket*  previous;
    struct _SLPDSocket*  next;
    int                 fd;
    time_t              timestamp;      
    int                 peeraddrlen;
    struct sockaddr_in  peeraddr;
    SLPSocketType       type;
    SLPBuffer           recvbuf;
    SLPBuffer           sendbuf;
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
SLPDSocket* SLPDSocketListLink(SLPDSocketList* list, SLPDSocket* sock);
/* Links the specified socket to the specified list                        */
/*                                                                         */
/* list     - pointer to the SLPSocketList to link the socket to.          */
/*                                                                         */
/* sock     - pointer to the SLPSocket to link to the list                 */
/*                                                                         */
/* Returns  - pointer to the linked socket                                 */
/*=========================================================================*/


/*=========================================================================*/
SLPDSocket* SLPDSocketListUnlink(SLPDSocketList* list, SLPDSocket* sock);
/* Unlinks the specified socket from the specified list                    */
/*                                                                         */
/* list     - pointer to the SLPSocketList to unlink the socket from.      */
/*                                                                         */
/* sock     - pointer to the SLPSocket to unlink to the list               */
/*                                                                         */
/* Returns  - pointer to the unlinked socket                               */
/*=========================================================================*/


/*=========================================================================*/
SLPDSocket* SLPDSocketListDestroy(SLPDSocketList* list, SLPDSocket* sock);
/* Unlinks and free()s the specified socket from the specified list        */
/*                                                                         */
/* list     - pointer to the SLPSocketList to unlink the socket from.      */
/*                                                                         */
/* sock     - pointer to the SLPSocket to unlink to the list               */
/*                                                                         */
/* Returns  - pointer to the unlinked socket                               */
/*=========================================================================*/


/*=========================================================================*/
void SLPDSocketListDestroyAll(SLPDSocketList* list);
/* Destroys all of the sockets from the specified list                     */
/*                                                                         */
/* list     - pointer to the SLPSocketList to destroy                      */
/*                                                                         */
/* Returns  - none                                                         */
/*=========================================================================*/


/*=========================================================================*/
void SLPDSocketInit(SLPDSocketList* list);
/* Adds SLPSockets (UDP and TCP) for all the interfaces and the loopback   */
/*                                                                         */
/* list     - pointer to SLPSocketList to which the SLPSockets will be     */
/*            added                                                        */
/*                                                                         */
/* Returns  - zero on success, -1 on failure.                              */
/*=========================================================================*/


/*=========================================================================*/
int SLPDProcessMessage(SLPBuffer recvbuf,
                       SLPBuffer sendbuf);
/* Processes the recvbuf and places the results in sendbuf                 */
/*                                                                         */
/* recvbuf  - message to process                                           */
/*                                                                         */
/* sendbuf  - results of the processed message                             */
/*                                                                         */
/* Returns  - zero on success errno on error.                              */
/*=========================================================================*/

#endif /* (!defined SLPD_H_INCLUDED) */
