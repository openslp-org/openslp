/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_message.h                                              */
/*                                                                         */
/* Abstract:    Header file that defines structures and constants that are */
/*              specific to the SLP wire protocol messages.                */
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

#if(!defined SLP_MESSAGE_H_INCLUDED)
#define SLP_MESSAGE_H_INCLUDED

#include <slp_buffer.h>

#ifdef WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <netinet/in.h>			/* for htonl() routines */
#endif

typedef char            CHAR;
typedef unsigned char   UINT8;
typedef unsigned short  UINT16;

#ifndef WIN32
typedef unsigned long   UINT32;
#endif

typedef CHAR*           PCHAR;
typedef UINT8*          PUINT8;
typedef UINT16*         PUINT16;
typedef UINT32*         PUINT32;


/*=========================================================================*/
/* SLP Important constants                                                 */
/*=========================================================================*/
#define SLP_RESERVED_PORT       427
#define SLP_MCAST_ADDRESS       0xeffffffd  /* 239.255.255.253 */
#define SLP_BCAST_ADDRESS       0xffffffff  /* 255.255.255.255 */
#define SLPv1_DA_MCAST_ADDRESS  0xe0000123  /* 224.0.1.35 */
#define LOOPBACK_ADDRESS        0x7f000001  /* 127.0.0.1 */
#define SLP_MAX_DATAGRAM_SIZE   1400
#if(!defined SLP_LIFETIME_MAXIMUM) 
#define SLP_LIFETIME_MAXIMUM    0xffff
#endif

/*=========================================================================*/
/* SLP Function ID constants                                               */
/*=========================================================================*/
#define SLP_FUNCT_SRVRQST         1
#define SLP_FUNCT_SRVRPLY         2
#define SLP_FUNCT_SRVREG          3
#define SLP_FUNCT_SRVDEREG        4
#define SLP_FUNCT_SRVACK          5
#define SLP_FUNCT_ATTRRQST        6
#define SLP_FUNCT_ATTRRPLY        7
#define SLP_FUNCT_DAADVERT        8
#define SLP_FUNCT_SRVTYPERQST     9
#define SLP_FUNCT_SRVTYPERPLY     10
#define SLP_FUNCT_SAADVERT        11



/*=========================================================================*/
/* SLP Protocol Error codes                                                */
/*=========================================================================*/
#define SLP_ERROR_OK                       0
#define SLP_ERROR_LANGUAGE_NOT_SUPPORTED   1
#define SLP_ERROR_PARSE_ERROR              2
#define SLP_ERROR_INVALID_REGISTRATION     3
#define SLP_ERROR_SCOPE_NOT_SUPPORTED      4
#define SLP_ERROR_CHARSET_NOT_UNDERSTOOD   5 /* valid only for SLPv1 */
#define SLP_ERROR_AUTHENTICATION_ABSENT    6
#define SLP_ERROR_AUTHENTICATION_FAILED    7
#define SLP_ERROR_VER_NOT_SUPPORTED        9
#define SLP_ERROR_INTERNAL_ERROR           10
#define SLP_ERROR_DA_BUSY_NOW              11
#define SLP_ERROR_INVALID_UPDATE           13
#define SLP_ERROR_MESSAGE_NOT_SUPPORTED    14
#define SLP_ERROR_REFRESH_REJECTED         15


/*=========================================================================*/
/* SLP Flags                                                               */
/*=========================================================================*/
#define SLP_FLAG_OVERFLOW         0x8000
#define SLP_FLAG_FRESH            0x4000
#define SLP_FLAG_MCAST            0x2000


/*=========================================================================*/
/* SLP Constants                                                           */
/*=========================================================================*/
#define CONFIG_MC_MAX             15    /* Max time to wait for a complete */
                                        /* multicast query response        */

#define CONFIG_RETRY_MAX          15    /* Wait interval to give up on a   */
                                        /* unicast request retransmission  */

#define CONFIG_RETRY_INTERVAL      3    /* Default wait between retransmits*/

/*=========================================================================*/
/* SLPHeader structure and associated functions                            */
/*=========================================================================*/
typedef struct _SLPHeader
{
    int         version;
    int         functionid;
    int         length;       
    int         flags;
    int         encoding;	/* language encoding, valid only for SLPv1 */
    int         extoffset;    
    int         xid;
    int         langtaglen;
    const char* langtag; /* points into the translated message */
}SLPHeader;


/*=========================================================================*/
/* SLPAuthBlock structure and associated functions                         */
/*=========================================================================*/
typedef struct _SLPAuthBlock
{
    struct _SLPAuthBlock*   next;  /* The next authblock in a list or NULL */
    int                     bsd;
    int                     length;
    unsigned long           timestamp;
    int                     spistrlen;
    const char*             spistr;
    const char*             authstruct;
}SLPAuthBlock;


/*=========================================================================*/
/* SLPUrlEntry structure and associated functions                          */
/*=========================================================================*/
typedef struct _SLPUrlEntry
{
    char                    reserved;       /* This will always be 0 */
    int                     lifetime;
    int                     urllen;
    const char*             url;
    char                    authcount;
    SLPAuthBlock*           autharray;
}SLPUrlEntry;


/*=========================================================================*/
/* SLPSrvRqst structure and associated functions                           */
/*=========================================================================*/
typedef struct _SLPSrvRqst
{
    int         prlistlen;
    const char* prlist;
    int         srvtypelen;
    const char* srvtype;
    int         scopelistlen;
    const char* scopelist;
    int         predicatelen;
    const char* predicate;
    int         spistrlen;
    const char* spistr;
}SLPSrvRqst;


/*=========================================================================*/
typedef struct _SLPSrvRply                                                 
/*=========================================================================*/
{
    int             errorcode;
    int             urlcount;
    SLPUrlEntry*    urlarray;
}SLPSrvRply;


/*=========================================================================*/
typedef struct _SLPSrvReg
/*=========================================================================*/
{
    SLPUrlEntry         urlentry;
    int                 srvtypelen;
    const char*         srvtype;
    int                 scopelistlen;
    const char*         scopelist;
    int                 attrlistlen;
    const char*         attrlist;
    char                authcount;
    SLPAuthBlock*       autharray;
}SLPSrvReg;
                                                                             

/*=========================================================================*/
typedef struct _SLPSrvDeReg
/*=========================================================================*/
{
    int                 scopelistlen;
    const char*         scopelist;
    SLPUrlEntry         urlentry;
    int                 taglistlen;
    const char*         taglist;
}SLPSrvDeReg;


/*=========================================================================*/
typedef struct _SLPSrvAck
/*=========================================================================*/
{
    int errorcode;
}SLPSrvAck;



/*=========================================================================*/
typedef struct _SLPDAAdvert
/*=========================================================================*/
{
    int                 errorcode;
    unsigned int        bootstamp;
    int                 urllen;
    const char*         url;
    int                 scopelistlen;
    const char*         scopelist;
    int                 attrlistlen;
    const char*         attrlist;
    int                 spilistlen;
    const char*         spilist;
    char                authcount;
    SLPAuthBlock*       autharray;
}SLPDAAdvert;


/*=========================================================================*/
typedef struct _SLPAttrRqst
/*=========================================================================*/
{
    int         prlistlen;
    const char* prlist;
    int         urllen;
    const char* url;
    int         scopelistlen;
    const char* scopelist;
    int         taglistlen;
    const char* taglist;
    int         spistrlen;
    const char* spistr;
}SLPAttrRqst;


/*=========================================================================*/
typedef struct _SLPAttrRply
/*=========================================================================*/
{
    int             errorcode;
    int             attrlistlen;
    const char*     attrlist;
    char            authcount;
    SLPAuthBlock*   autharray;
}SLPAttrRply;


/*=========================================================================*/
typedef struct _SLPSrvTypeRqst
/*=========================================================================*/
{
    int         prlistlen;
    const char* prlist;
    int         namingauthlen;
    const char* namingauth;
    int         scopelistlen;
    const char* scopelist;
}SLPSrvTypeRqst;



/*=========================================================================*/
typedef struct _SLPSrvTypeRply
/*=========================================================================*/
{
    int         errorcode;
    int         srvtypelistlen;
    const char* srvtypelist;
}SLPSrvTypeRply;



/*=========================================================================*/
typedef struct _SLPSAAdvert
/*=========================================================================*/
{
    int             urllen;
    const char*     url;
    int             scopelistlen;
    const char*     scopelist;
    int             attrlistlen;
    const char*     attrlist;
    char            authcount;
    SLPAuthBlock*   authblock;
}SLPSAAdvert;


/*=========================================================================*/
typedef struct _SLPMessage
/*=========================================================================*/
{
    SLPHeader             header;
    
    union _body
    {
        SLPSrvRqst        srvrqst;
        SLPSrvRply        srvrply;
        SLPSrvReg         srvreg;
        SLPSrvDeReg       srvdereg;
        SLPSrvAck         srvack;
        SLPDAAdvert       daadvert;
        SLPAttrRqst       attrrqst;
        SLPAttrRply       attrrply;
        SLPSrvTypeRqst    srvtyperqst;
        SLPSrvTypeRply    srvtyperply;
        SLPSAAdvert       saadvert;
    }body; 

}*SLPMessage;


/*=========================================================================*/
SLPMessage SLPMessageAlloc();
/* Allocates memory for a SLP message descriptor                           */
/*                                                                         */
/* Returns   - A newly allocated SLPMessage pointer of NULL on ENOMEM      */
/*=========================================================================*/


/*=========================================================================*/
SLPMessage SLPMessageRealloc(SLPMessage msg);
/* Reallocates memory for a SLP message descriptor                         */
/*                                                                         */
/* Returns   - A newly allocated SLPMessage pointer of NULL on ENOMEM      */
/*=========================================================================*/


/*=========================================================================*/
void SLPMessageFree(SLPMessage message);
/* Frees memory that might have been allocated by the SLPMessage for       */
/* UrlEntryLists or AuthBlockLists.                                        */
/*                                                                         */
/* message  - (IN) the SLPMessage to free                                  */
/*=========================================================================*/


/*=========================================================================*/
int SLPMessageParseBuffer(SLPBuffer buffer, SLPMessage message);
/* Initializes a message descriptor by parsing the specified buffer.       */
/*                                                                         */
/* buffer   - (IN) pointer the SLPBuffer to parse                          */
/*                                                                         */
/* message  - (OUT) set to describe the message from the buffer            */
/*                                                                         */
/* Returns  - Zero on success, SLP_ERROR_PARSE_ERROR, or                   */
/*            SLP_ERROR_INTERNAL_ERROR if out of memory.  SLPMessage is    */
/*            invalid return is not successful.                            */
/*                                                                         */
/* WARNING  - If successful, pointers in the SLPMessage reference memory in*/ 
/*            the parsed SLPBuffer.  If SLPBufferFree() is called then the */
/*            pointers in SLPMessage will be invalidated.                  */
/*=========================================================================*/

#ifdef i386
/*=========================================================================*/
#define AsUINT16(charptr)   ( ntohs(*((PUINT16)(charptr))) )
#define AsUINT24(charptr)   ( ntohl(*((PUINT32)(charptr)))>>8 )
#define AsUINT32(charptr)   ( ntohl(*((PUINT32)(charptr))) )
/* Macros used to parse buffers                                            */
/*=========================================================================*/

/*=========================================================================*/
#define ToUINT16(charptr,val)   ( *((PUINT16)(charptr)) =  htons((val)) )
#define ToUINT24(charptr,val)   ( *((PUINT32)(charptr)) =  htonl((val)<<8) )
#define ToUINT32(charptr,val)   ( *((PUINT32)(charptr)) =  htonl((val)) )
/* Macros used to set buffers                                              */
/*=========================================================================*/
#else
/*=========================================================================*/
unsigned short AsUINT16(const char *charptr);
unsigned int AsUINT24(const char *charptr);
unsigned int AsUINT32(const char *charptr);
/* Functions used to parse buffers                                         */
/*=========================================================================*/

/*=========================================================================*/
void ToUINT16(char *charptr, unsigned int val);
void ToUINT24(char *charptr, unsigned int val);
void ToUINT32(char *charptr, unsigned int val);
/* Functions used to set buffers                                           */
/*=========================================================================*/
#endif

#if defined(ENABLE_SLPv1)
#include <slp_v1message.h>
#endif
#endif
