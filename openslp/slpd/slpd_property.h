
/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_property.h                                            */
/*                                                                         */
/* Abstract:    Defines the data structures for global SLP properties      */
/*                                                                         */
/* WARNING:     NOT thread safe!                                           */
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

#ifndef SLPD_PROPERTY_H_INCLUDED
#define SLPD_PROPERTY_H_INCLUDED

#include "slpd.h"

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
    int             DAActiveDiscoveryInterval;
    int             activeDiscoveryXmits;
    int             nextActiveDiscovery;
    int             nextPassiveDAAdvert;
    const char*     multicastIF;
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
    int             securityEnabled;
    int             checkSourceAddr;
}SLPDProperty;


/*=========================================================================*/
extern SLPDProperty G_SlpdProperty;
/* Global variable that holds all of the properties that slpd cares about  */
/*=========================================================================*/


/*=========================================================================*/
int SLPDPropertyInit(const char* conffile); 
/* Called to initialize slp properties.  Reads .conf file, etc.            */
/*                                                                         */
/* conffile (IN) the path of the configuration file to use                 */
/*=========================================================================*/


#ifdef DEBUG
/*=========================================================================*/
void SLPDPropertyDeinit();
/* Release all resources used by the properties                            */
/*=========================================================================*/
#endif


#endif
