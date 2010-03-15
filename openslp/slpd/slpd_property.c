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

/** Global SLP property management.
 *
 * @file       slpd_property.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#include "slpd_property.h"
#include "slp_message.h"
#include "slp_property.h"  
#include "slp_iface.h"
#include "slp_net.h"
#include "slp_xmalloc.h"
#include "slpd_log.h"

/** The global daemon attribute structure
 */
SLPDProperty G_SlpdProperty;

/** Reinitialize the slpd property management subsystem.
 *
 * Clears and rereads configuration parameters from files into the system.
 * Resets slpd-specific overrides.
 */
void SLPDPropertyReinit(void)
{
   int sts;
   char * myinterfaces = 0;
   int family = AF_UNSPEC;

   /* free previous settings (if any) */
   xfree(G_SlpdProperty.useScopes);
   xfree(G_SlpdProperty.DAAddresses);
   xfree(G_SlpdProperty.interfaces);
   xfree(G_SlpdProperty.locale);

   /* reinitialize property sub-system */
   (void)SLPPropertyReinit();

   /* set the properties without hard defaults */
   G_SlpdProperty.isDA = SLPPropertyAsBoolean("net.slp.isDA");
   G_SlpdProperty.activeDADetection = SLPPropertyAsBoolean("net.slp.activeDADetection");

   if (G_SlpdProperty.activeDADetection)
   {
      G_SlpdProperty.DAActiveDiscoveryInterval = SLPPropertyAsInteger("net.slp.DAActiveDiscoveryInterval");
      if (G_SlpdProperty.DAActiveDiscoveryInterval > 1
            && G_SlpdProperty.DAActiveDiscoveryInterval < SLPD_CONFIG_DA_FIND)
         G_SlpdProperty.DAActiveDiscoveryInterval = SLPD_CONFIG_DA_FIND;
   }
   else
      G_SlpdProperty.DAActiveDiscoveryInterval = 0;

   G_SlpdProperty.passiveDADetection = SLPPropertyAsBoolean("net.slp.passiveDADetection");
   G_SlpdProperty.staleDACheckPeriod = SLPPropertyAsInteger("net.slp.staleDACheckPeriod");
   if (G_SlpdProperty.staleDACheckPeriod > 0)
   {
      if (G_SlpdProperty.staleDACheckPeriod < (SLPD_HEARTBEATS_PER_CHECK_PERIOD + 1) * SLPD_AGE_INTERVAL)
          G_SlpdProperty.staleDACheckPeriod = (SLPD_HEARTBEATS_PER_CHECK_PERIOD + 1) * SLPD_AGE_INTERVAL;
      SLPDLog("staleDACheckPeriod = %ds\n", G_SlpdProperty.staleDACheckPeriod);
   }

   G_SlpdProperty.isBroadcastOnly = SLPPropertyAsBoolean("net.slp.isBroadcastOnly");
   G_SlpdProperty.multicastTTL = SLPPropertyAsInteger("net.slp.multicastTTL");
   G_SlpdProperty.multicastMaximumWait = SLPPropertyAsInteger("net.slp.multicastMaximumWait");
   G_SlpdProperty.unicastMaximumWait = SLPPropertyAsInteger("net.slp.unicastMaximumWait");
   SLPPropertyAsIntegerVector("net.slp.unicastTimeouts", G_SlpdProperty.unicastTimeouts, MAX_RETRANSMITS);
   G_SlpdProperty.randomWaitBound = SLPPropertyAsInteger("net.slp.randomWaitBound");
   G_SlpdProperty.maxResults = SLPPropertyAsInteger("net.slp.maxResults");
   G_SlpdProperty.traceMsg = SLPPropertyAsBoolean("net.slp.traceMsg");
   G_SlpdProperty.traceReg = SLPPropertyAsBoolean("net.slp.traceReg");
   G_SlpdProperty.traceDrop = SLPPropertyAsBoolean("net.slp.traceDrop");
   G_SlpdProperty.traceDATraffic = SLPPropertyAsBoolean("net.slp.traceDATraffic");
   G_SlpdProperty.appendLog = SLPPropertyAsBoolean("net.slp.appendLog");

   if ((G_SlpdProperty.DAAddresses = SLPPropertyXDup("net.slp.DAAddresses")) != 0)
      G_SlpdProperty.DAAddressesLen = strlen(G_SlpdProperty.DAAddresses);

   /** @todo Make sure that we are using scopes correctly. What about DHCP, etc? */

   if ((G_SlpdProperty.useScopes = SLPPropertyXDup("net.slp.useScopes")) != 0)
      G_SlpdProperty.useScopesLen = strlen(G_SlpdProperty.useScopes);

   if ((G_SlpdProperty.locale = SLPPropertyXDup("net.slp.locale")) != 0)
      G_SlpdProperty.localeLen = strlen(G_SlpdProperty.locale);

   G_SlpdProperty.securityEnabled = SLPPropertyAsBoolean("net.slp.securityEnabled");
   G_SlpdProperty.checkSourceAddr = SLPPropertyAsBoolean("net.slp.checkSourceAddr");
   G_SlpdProperty.DAHeartBeat = SLPPropertyAsInteger("net.slp.DAHeartBeat");
   if (G_SlpdProperty.staleDACheckPeriod > 0)
   {
      /* Adjust the heartbeat interval if we need to send it faster for
       * stale DA detection
       */
      int maxHeartbeat = G_SlpdProperty.staleDACheckPeriod / SLPD_HEARTBEATS_PER_CHECK_PERIOD;
      if ((G_SlpdProperty.DAHeartBeat == 0) ||
          (G_SlpdProperty.DAHeartBeat > maxHeartbeat))
      {
         SLPDLog("Overriding heartbeat to %ds for stale DA check\n", maxHeartbeat);
         G_SlpdProperty.DAHeartBeat = maxHeartbeat;
      }
   }
   /* Can't send it out more frequently than every interval */
   if (G_SlpdProperty.DAHeartBeat < SLPD_AGE_INTERVAL)
      G_SlpdProperty.DAHeartBeat = SLPD_AGE_INTERVAL;

   G_SlpdProperty.port = (uint16_t)SLPPropertyAsInteger("net.slp.port");

   /* set the net.slp.interfaces property */
   if (SLPNetIsIPV4() && SLPNetIsIPV6())
      family = AF_UNSPEC;
   else if (SLPNetIsIPV4())
      family = AF_INET;
   else if (SLPNetIsIPV6())
      family = AF_INET6;

   myinterfaces = SLPPropertyXDup("net.slp.interfaces");
   sts = SLPIfaceGetInfo(myinterfaces, &G_SlpdProperty.ifaceInfo, family);
   xfree(myinterfaces);

   if (!G_SlpdProperty.indexingPropertiesSet)
   {
#ifdef ENABLE_PREDICATES
      G_SlpdProperty.indexedAttributesLen = 0;
      if ((G_SlpdProperty.indexedAttributes = SLPPropertyXDup("net.slp.indexedAttributes")) != 0)
         G_SlpdProperty.indexedAttributesLen = strlen(G_SlpdProperty.indexedAttributes);
#endif

      G_SlpdProperty.srvtypeIsIndexed = SLPPropertyAsBoolean("net.slp.indexSrvtype");
      G_SlpdProperty.indexingPropertiesSet = 1;
   }
   else
   {
#ifdef ENABLE_PREDICATES
      int attrchg = 0;
      char *indexedAttributes = SLPPropertyXDup("net.slp.indexedAttributes");
      if (!indexedAttributes && G_SlpdProperty.indexedAttributes)
         attrchg = 1;
      else if (indexedAttributes && !G_SlpdProperty.indexedAttributes)
         attrchg = 1;
      else if (indexedAttributes)
      {
         if (strcmp(indexedAttributes, G_SlpdProperty.indexedAttributes) != 0)
            attrchg = 1;
      }
      if (attrchg)
         SLPDLog("Cannot change value of net.slp.indexedAttributes without restarting the daemon\n");
      xfree(indexedAttributes);
#endif
      if (G_SlpdProperty.srvtypeIsIndexed != SLPPropertyAsBoolean("net.slp.indexSrvtype"))
         SLPDLog("Cannot change value of net.slp.indexSrvtype without restarting the daemon\n");
   }

   if (sts == 0)
   {
      myinterfaces = 0;
      if (SLPIfaceSockaddrsToString(G_SlpdProperty.ifaceInfo.iface_addr,
            G_SlpdProperty.ifaceInfo.iface_count, &myinterfaces) == 0)
      {
         SLPPropertySet("net.slp.interfaces", myinterfaces, SLP_PA_USERSET);
         G_SlpdProperty.interfaces = myinterfaces;
         G_SlpdProperty.interfacesLen = strlen(G_SlpdProperty.interfaces);
      }
   }

   /* set the value used internally as the url for this agent */
   strcpy(G_SlpdProperty.urlPrefix, G_SlpdProperty.isDA? SLP_DA_SERVICE_TYPE: SLP_SA_SERVICE_TYPE);
   strcat(G_SlpdProperty.urlPrefix, "://");
   G_SlpdProperty.urlPrefixLen = strlen(G_SlpdProperty.urlPrefix);

   /* set other values used internally */
   G_SlpdProperty.DATimestamp = (uint32_t)time(0);     /* DATimestamp must be the boot time of the process */
   G_SlpdProperty.activeDiscoveryXmits = 3;  /* ensures xmit on first 3 calls to SLPDKnownDAActiveDiscovery() */
   G_SlpdProperty.nextActiveDiscovery = 0;   /* ensures xmit on first call to SLPDKnownDAActiveDiscovery() */
   G_SlpdProperty.nextPassiveDAAdvert = 0;   /* ensures xmit on first call to SLPDKnownDAPassiveDiscovery()*/

}

/** Initialize the slpd property management subsystem.
 *
 * Reads configuration parameters from a file into the system.
 *
 * @param[in] conffile - The name of the configuration file
 *
 * @return Zero on success, or a non-zero value on failure.
 */
int SLPDPropertyInit(const char * conffile)
{
   /* initialize the slp property subsystem */
   if (SLPPropertyInit(conffile) != 0)
      return -1;

   memset(&G_SlpdProperty, 0, sizeof(G_SlpdProperty));

   SLPDPropertyReinit();

   return 0;
}

/** Release resources associated with configuration data.
 * 
 * Free all memory stored in the global slpd properties structure, and
 * exit the properties sub-system.
 */
void SLPDPropertyDeinit(void)
{
   xfree(G_SlpdProperty.useScopes);
   xfree(G_SlpdProperty.DAAddresses);
   xfree(G_SlpdProperty.interfaces);
#ifdef ENABLE_PREDICATES
   xfree(G_SlpdProperty.indexedAttributes);
#endif
   xfree(G_SlpdProperty.locale);

   SLPPropertyExit();
}

/*=========================================================================*/
