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

/** Header file that defines SLP wire protocol message structures.
 *
 * @file       slp_message.h
 * @author     John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeMessage
 */

#ifndef SLP_MESSAGE_H_INCLUDED
#define SLP_MESSAGE_H_INCLUDED

/*!@defgroup CommonCodeMessage SLP Messages
 * @ingroup CommonCode
 * @{
 */

#include <limits.h>

#include "slp_buffer.h"
#include "slp_socket.h"
#include "slp_types.h"

/** SLP Important constants */
#define SLP_RESERVED_PORT       427          /*!< The SLP port number */
#define SLP_MCAST_ADDRESS       0xeffffffd   /*!< 239.255.255.253 */
#define SLP_BCAST_ADDRESS       0xffffffff   /*!< 255.255.255.255 */
#define SLPv1_DA_MCAST_ADDRESS  0xe0000123   /*!< 224.0.1.35 */
#define LOOPBACK_ADDRESS        0x7f000001   /*!< 127.0.0.1 */

#if !defined(SLP_LIFETIME_MAXIMUM)
# define SLP_LIFETIME_MAXIMUM   0xffff
#endif

/** SLP Function ID constants */
#define SLP_FUNCT_SRVRQST        1
#define SLP_FUNCT_SRVRPLY        2
#define SLP_FUNCT_SRVREG         3
#define SLP_FUNCT_SRVDEREG       4
#define SLP_FUNCT_SRVACK         5
#define SLP_FUNCT_ATTRRQST       6
#define SLP_FUNCT_ATTRRPLY       7
#define SLP_FUNCT_DAADVERT       8
#define SLP_FUNCT_SRVTYPERQST    9
#define SLP_FUNCT_SRVTYPERPLY    10
#define SLP_FUNCT_SAADVERT       11

/** SLP Protocol Error codes */
#define SLP_ERROR_OK                      0
#define SLP_ERROR_LANGUAGE_NOT_SUPPORTED  1
#define SLP_ERROR_PARSE_ERROR             2
#define SLP_ERROR_INVALID_REGISTRATION    3
#define SLP_ERROR_SCOPE_NOT_SUPPORTED     4
#define SLP_ERROR_CHARSET_NOT_UNDERSTOOD  5 /* valid only for SLPv1 */
#define SLP_ERROR_AUTHENTICATION_UNKNOWN  5
#define SLP_ERROR_AUTHENTICATION_ABSENT   6
#define SLP_ERROR_AUTHENTICATION_FAILED   7
#define SLP_ERROR_VER_NOT_SUPPORTED       9
#define SLP_ERROR_INTERNAL_ERROR          10
#define SLP_ERROR_DA_BUSY_NOW             11
#define SLP_ERROR_OPTION_NOT_UNDERSTOOD   12
#define SLP_ERROR_INVALID_UPDATE          13
#define SLP_ERROR_MESSAGE_NOT_SUPPORTED   14
#define SLP_ERROR_REFRESH_REJECTED        15

/** Additional internal error codes */
#define SLP_ERROR_RETRY_UNICAST           100

/** SLP Flags */
#define SLP_FLAG_OVERFLOW         0x8000
#define SLP_FLAG_FRESH            0x4000
#define SLP_FLAG_MCAST            0x2000

#if !defined(UNICAST_NOT_SUPPORTED)
# define SLP_FLAG_UCAST           0x0000
#endif

/** SLP Constants */

/** Max time to wait for a complete multicast query response */
#define CONFIG_MC_MAX            15    

/** Wait interval to give up on a unicast request retransmission */
#define CONFIG_RETRY_MAX         15    

/** Default wait between retransmits */
#define CONFIG_RETRY_INTERVAL    3    

#define SLP_DA_SERVICE_TYPE      "service:directory-agent"
#define SLP_SA_SERVICE_TYPE      "service:service-agent"

/** SLP Registration Sources */
#define SLP_REG_SOURCE_UNKNOWN   0
#define SLP_REG_SOURCE_REMOTE    1  /* from a remote host    */
#define SLP_REG_SOURCE_LOCAL     2  /* from localhost or IPC */
#define SLP_REG_SOURCE_STATIC    3  /* from the slp.reg file */

/** SLP Extension IDs */

/** @todo Deprecate the use of the experimental version of the PID watcher
 * extension, which was originally implemented in OpenSLP 1.x. Currently the
 * 2.x UA code base requests this extension using the EXP version in order to
 * be compatible with 1.2.x SA's, but the 2.x SA understands both. This allows
 * some future version of the UA to send the official version and have the 
 * (then) older version of 2.x SA's understand the official version.
 */

/** format: Extid(2), nxtextoffs(3), pid(4) */
#define SLP_EXTENSION_ID_REG_PID       0x4001
#define SLP_EXTENSION_ID_REG_PID_EXP   0x9799   /* DEPRECATED */

/** Buffer extraction and insertion macros */
#define AS_UINT16(p) (uint16_t)              \
      (                                      \
         (((const uint8_t *)(p))[0] <<  8) | \
         (((const uint8_t *)(p))[1]      )   \
      )

#define AS_UINT24(p) (uint32_t)              \
      (                                      \
         (((const uint8_t *)(p))[0] << 16) | \
         (((const uint8_t *)(p))[1] <<  8) | \
         (((const uint8_t *)(p))[2]      )   \
      )

#define AS_UINT32(p) (uint32_t)              \
      (                                      \
         (((const uint8_t *)(p))[0] << 24) | \
         (((const uint8_t *)(p))[1] << 16) | \
         (((const uint8_t *)(p))[2] <<  8) | \
         (((const uint8_t *)(p))[3]      )   \
      )

#define TO_UINT16(p,v)                                            \
      (                                                           \
         (((uint8_t *)(p))[0] = (uint8_t)(((v) >>  8) & 0xff)),   \
         (((uint8_t *)(p))[1] = (uint8_t)(((v)      ) & 0xff))    \
      )

#define TO_UINT24(p,v)                                            \
      (                                                           \
         (((uint8_t *)(p))[0] = (uint8_t)(((v) >> 16) & 0xff)),   \
         (((uint8_t *)(p))[1] = (uint8_t)(((v) >>  8) & 0xff)),   \
         (((uint8_t *)(p))[2] = (uint8_t)(((v)      ) & 0xff))    \
      )

#define TO_UINT32(p,v)                                            \
      (                                                           \
         (((uint8_t *)(p))[0] = (uint8_t)(((v) >> 24) & 0xff)),   \
         (((uint8_t *)(p))[1] = (uint8_t)(((v) >> 16) & 0xff)),   \
         (((uint8_t *)(p))[2] = (uint8_t)(((v) >>  8) & 0xff)),   \
         (((uint8_t *)(p))[3] = (uint8_t)(((v)      ) & 0xff))    \
      )

/* Assuming the current byte is the packet version, returns the length */
#define PEEK_LENGTH(p) ((*p == 2) ? AS_UINT24(p + 2) : (*p == 1) ? AS_UINT16(p + 2) : 1)

/* buffer-based wire routines */
uint16_t GetUINT16(uint8_t ** cpp);
uint32_t GetUINT24(uint8_t ** cpp);
uint32_t GetUINT32(uint8_t ** cpp);
char * GetStrPtr(uint8_t ** cpp, size_t length);

void PutUINT16(uint8_t ** cpp, size_t val);
void PutUINT24(uint8_t ** cpp, size_t val);
void PutUINT32(uint8_t ** cpp, size_t val);

/** SLPHeader structure and associated functions */
typedef struct _SLPHeader
{
   int version;
   int functionid;
   size_t length;       
   int flags;
   int encoding;           /* language encoding, valid only for SLPv1 */
   int extoffset;    
   uint16_t xid;
   size_t langtaglen;
   const char * langtag;   /* points into the translated message */
} SLPHeader;

/** SLPAuthBlock structure and associated functions */
typedef struct _SLPAuthBlock
{
   uint16_t bsd;
   size_t length;
   uint32_t timestamp;
   size_t spistrlen;
   const char * spistr;
   const char * authstruct;
   size_t opaquelen; /* convenience */
   uint8_t * opaque; /* convenience */
} SLPAuthBlock;

/** SLPUrlEntry structure and associated functions */
typedef struct _SLPUrlEntry
{
   char reserved;       /*!< always 0 */
   int lifetime;
   size_t urllen;
   const char * url;
   int authcount;
   SLPAuthBlock * autharray; 
   size_t opaquelen;    /*!< convenience */
   uint8_t * opaque;    /*!< convenience */
} SLPUrlEntry;

/** SLPSrvRqst structure and associated functions */
typedef struct _SLPSrvRqst
{
   size_t prlistlen;
   const char * prlist;
   size_t srvtypelen;
   const char * srvtype;
   size_t scopelistlen;
   const char * scopelist;
   int predicatever;
   size_t predicatelen;
   const char * predicate;
   size_t spistrlen;
   const char * spistr;
} SLPSrvRqst;

/** SLPSrvRply structure and associated functions */
typedef struct _SLPSrvRply                                                 
{
   int errorcode;
   int urlcount;
   SLPUrlEntry * urlarray;
} SLPSrvRply;

/** SLPSrvReg structure and associated functions */
typedef struct _SLPSrvReg
{
   SLPUrlEntry urlentry;
   size_t srvtypelen;
   const char * srvtype;
   size_t scopelistlen;
   const char * scopelist;
   size_t attrlistlen;
   const char * attrlist;
   int authcount;
   SLPAuthBlock * autharray;
   /* The following are used for OpenSLP specific extensions */
   uint32_t pid;
   int source;    /*!< convenience */
} SLPSrvReg;

/** SLPSrvDeReg structure and associated functions */
typedef struct _SLPSrvDeReg
{
   size_t scopelistlen;
   const char * scopelist;
   SLPUrlEntry urlentry;
   size_t taglistlen;
   const char * taglist;
} SLPSrvDeReg;

/** SLPSrvAck structure and associated functions */
typedef struct _SLPSrvAck
{
   int errorcode;
} SLPSrvAck;

/** SLPDAAdvert structure and associated functions */
typedef struct _SLPDAAdvert
{
   int errorcode;
   uint32_t bootstamp;
   size_t urllen;
   const char * url;
   size_t scopelistlen;
   const char * scopelist;
   size_t attrlistlen;
   const char * attrlist;
   size_t spilistlen;
   const char * spilist;
   int authcount;
   SLPAuthBlock * autharray;
} SLPDAAdvert;

/** SLPAttrRqst structure and associated functions */
typedef struct _SLPAttrRqst
{
   size_t prlistlen;
   const char * prlist;
   size_t urllen;
   const char * url;
   size_t scopelistlen;
   const char * scopelist;
   size_t taglistlen;
   const char * taglist;
   size_t spistrlen;
   const char * spistr;
} SLPAttrRqst;

/** SLPAttrRply structure and associated functions */
typedef struct _SLPAttrRply
{
   int errorcode;
   size_t attrlistlen;
   const char * attrlist;
   int authcount;
   SLPAuthBlock * autharray;
} SLPAttrRply;

/** SLPSrvTypeRqst structure and associated functions */
typedef struct _SLPSrvTypeRqst
{
   size_t prlistlen;
   const char * prlist;
   size_t namingauthlen;
   const char * namingauth;
   size_t scopelistlen;
   const char * scopelist;
} SLPSrvTypeRqst;

/** SLPSrvTypeRply structure and associated functions */
typedef struct _SLPSrvTypeRply
{
   int errorcode;
   size_t srvtypelistlen;
   const char * srvtypelist;
} SLPSrvTypeRply;

/** SLPSAAdvert structure and associated functions */
typedef struct _SLPSAAdvert
{
   size_t urllen;
   const char * url;
   size_t scopelistlen;
   const char * scopelist;
   size_t attrlistlen;
   const char * attrlist;
   int authcount;
   SLPAuthBlock * autharray;
} SLPSAAdvert;

/** SLP wire protocol message management structures and prototypes */
typedef struct _SLPMessage
{
   struct sockaddr_storage peer;
   struct sockaddr_storage localaddr;
   SLPHeader header;
   union _body
   {
      SLPSrvRqst     srvrqst;
      SLPSrvRply     srvrply;
      SLPSrvReg      srvreg;
      SLPSrvDeReg    srvdereg;
      SLPSrvAck      srvack;
      SLPDAAdvert    daadvert;
      SLPAttrRqst    attrrqst;
      SLPAttrRply    attrrply;
      SLPSrvTypeRqst srvtyperqst;
      SLPSrvTypeRply srvtyperply;
      SLPSAAdvert    saadvert;
   } body; 
} SLPMessage;

void SLPMessageFreeInternals(SLPMessage * mp);
SLPMessage * SLPMessageAlloc(void);
SLPMessage * SLPMessageRealloc(SLPMessage * mp);
void SLPMessageFree(SLPMessage * mp);

/* Message parsing routines */
int SLPMessageParseHeader(SLPBuffer buffer, SLPHeader * header);
int SLPMessageParseBuffer(void * peeraddr, const void * localaddr, 
      SLPBuffer buffer, SLPMessage * mp);

/*! @} */

#endif   /* SLP_MESSAGE_H_INCLUDED */

/*=========================================================================*/
