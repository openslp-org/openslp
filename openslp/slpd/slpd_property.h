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

/** Global SLP property routines.
 *
 * @file       slpd_property.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#ifndef SLPD_PROPERTY_H_INCLUDED
#define SLPD_PROPERTY_H_INCLUDED

/*!@defgroup SlpdCodeProperty Properties */

/*!@addtogroup SlpdCodeProperty
 * @ingroup SlpdCode
 * @{
 */

#include "slp_types.h"
#include "slp_iface.h"
#include "slp_message.h"
#include "slpd.h"

#define MAX_URLPREFIX_SZ   (sizeof(SLP_DA_SERVICE_TYPE) + sizeof("://"))
#define MAX_RETRANSMITS             5      /* we'll only re-xmit 5 times! */

/** A structure that holds the value of all the properties slpd cares about.
 */
typedef struct _SLPDProperty
{
   size_t urlPrefixLen;
   char urlPrefix[MAX_URLPREFIX_SZ];
   size_t useScopesLen;
   char * useScopes;
   size_t DAAddressesLen;
   char * DAAddresses;
   uint32_t DATimestamp;
   SLPIfaceInfo ifaceInfo;
   size_t interfacesLen;
   char * interfaces;
   int port;
   size_t localeLen;
   char * locale;
   int isBroadcastOnly;
   int passiveDADetection;
   int activeDADetection; 
   int DAActiveDiscoveryInterval;
   int activeDiscoveryXmits;
   int nextActiveDiscovery;
   int nextPassiveDAAdvert;
   int multicastTTL;
   int multicastMaximumWait;
   int unicastMaximumWait;  
   int unicastTimeouts[MAX_RETRANSMITS];
   int randomWaitBound;
   int maxResults;
   int traceMsg;
   int traceReg;
   int traceDrop;
   int traceDATraffic;
   int isDA;
   int securityEnabled;
   int checkSourceAddr;
   int DAHeartBeat;
} SLPDProperty;

extern SLPDProperty G_SlpdProperty;

void SLPDPropertyReinit(void);
int SLPDPropertyInit(const char * conffile); 
void SLPDPropertyDeinit(void);

/*! @} */

#endif   /* SLPD_PROPERTY_H_INCLUDED */

/*=========================================================================*/
