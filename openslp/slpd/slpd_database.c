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

/** Database abstraction.
 *
 * Implements database abstraction. Currently a simple double linked list 
 * (common/slp_database.c) is used for the underlying storage.
 *
 * @file       slpd_database.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#include "slpd_database.h"
#include "slpd_regfile.h"
#include "slpd_property.h"
#include "slpd_log.h"
#include "slpd_knownda.h"

#ifdef ENABLE_PREDICATES
# include "slpd_predicate.h"
#endif

#include "slp_compare.h"
#include "slp_xmalloc.h"
#include "slp_pid.h"
#include "slp_net.h"
#include "slpd_incoming.h"

/** The slpd static global database object.
 */
static SLPDDatabase G_SlpdDatabase;

/** Ages the database entries and clears new and deleted entry lists
 *
 * @param[in] seconds - The number of seconds to age each entry by.
 * @param[in] ageall - Age even entries with SLP_LIFETIME_MAXIMUM.
 */
void SLPDDatabaseAge(int seconds, int ageall)
{
   SLPDatabaseHandle dh;
   SLPDatabaseEntry * entry;
   SLPSrvReg * srvreg;

   dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
   if (dh)
   {
      while (1)
      {
         entry = SLPDatabaseEnum(dh);
         if (entry == 0)
            break;

         /* srvreg is the SrvReg message from the database */
         srvreg = &entry->msg->body.srvreg;

         if (srvreg->urlentry.lifetime == SLP_LIFETIME_MAXIMUM)
         {
            if (srvreg->source == SLP_REG_SOURCE_LOCAL 
                  || srvreg->source == SLP_REG_SOURCE_STATIC)
            {
               /* entries that were made from local registrations    */
               /* and entries made from static registration file     */
               /* that have a lifetime of SLP_LIFETIME_MAXIMUM must  */
               /* NEVER be aged                                      */
               continue;
            }
            if (ageall == 0)
            {
               /* Don't age any services that have a lifetime of     */
               /* SLP_LIFETIME_MAXIMUM unless explicitly told to     */
               continue;
            }
         }

         /* If the entry is local and it's configured for pid watching and 
          * its pid is invalid, then notify DA's and ensure it times out.
          */
         if (srvreg->source == SLP_REG_SOURCE_LOCAL 
               && srvreg->pid && !SLPPidExists(srvreg->pid))
         {
            SLPDLogRegistration("PID Watcher Deregistration", entry);
            SLPDKnownDADeRegisterWithAllDas(entry->msg, entry->buf);
            SLPDatabaseRemove(dh, entry);
            continue;
         }

         /* Age entry and remove those that have timed out */
         srvreg->urlentry.lifetime -= seconds;
         if (srvreg->urlentry.lifetime <= 0)
         {
            SLPDLogRegistration("Timeout", entry);
            SLPDatabaseRemove(dh, entry);
         }
      }
      SLPDatabaseClose(dh);
   }
}

/** Add a service registration to the database.
 *
 * @param[in] msg - SLPMessage of a SrvReg message as returned by
 *    SLPMessageParse.
 *
 * @param[in] buf - Otherwise unreferenced buffer interpreted by the 
 *    msg structure.
 *
 * @return Zero on success, or a non-zero value on error.
 *
 * @remarks All registrations are treated as fresh.
 */
int SLPDDatabaseReg(SLPMessage * msg, SLPBuffer buf)
{
   SLPDatabaseHandle dh;
   SLPDatabaseEntry * entry;
   SLPSrvReg * entryreg;
   SLPSrvReg * reg;
   int result;
   SLPIfaceInfo ifaces;
   int i;

   /* reg is the SrvReg message being registered */
   reg = &msg->body.srvreg;

   /* check service-url syntax */
   if (SLPCheckServiceUrlSyntax(reg->urlentry.url, reg->urlentry.urllen))
      return SLP_ERROR_INVALID_REGISTRATION;

   /* check attr-list syntax */
   if (reg->attrlistlen 
         && SLPCheckAttributeListSyntax(reg->attrlist,reg->attrlistlen))
      return SLP_ERROR_INVALID_REGISTRATION;

   dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
   if (dh)
   {
      /* check to see if there is already an identical entry */
      while (1)
      {
         entry = SLPDatabaseEnum(dh);
         if (entry == 0) 
            break;

         /* entry reg is the SrvReg message from the database */
         entryreg = &entry->msg->body.srvreg;

         if (SLPCompareString(entryreg->urlentry.urllen, 
               entryreg->urlentry.url, reg->urlentry.urllen,
               reg->urlentry.url) == 0)
         {
            if (SLPIntersectStringList(entryreg->scopelistlen, 
                  entryreg->scopelist, reg->scopelistlen,reg->scopelist) > 0)
            {
               /* check to ensure the source addr is the same
                  as the original */
               if (G_SlpdProperty.checkSourceAddr)
               {
                  if ((entry->msg->peer.ss_family == AF_INET 
                        && msg->peer.ss_family == AF_INET 
                        && memcmp(&(((struct sockaddr_in *)
                              &(entry->msg->peer))->sin_addr),
                              &(((struct sockaddr_in *)
                                    &(msg->peer))->sin_addr),
                              sizeof(struct in_addr))) 
                        || (entry->msg->peer.ss_family == AF_INET6 
                              && msg->peer.ss_family == AF_INET6 
                              && memcmp(&(((struct sockaddr_in6 *)
                                    &(entry->msg->peer))->sin6_addr),
                                    &(((struct sockaddr_in6 *)
                                          &(msg->peer))->sin6_addr),
                                    sizeof(struct in6_addr))))
                  {
                     SLPDatabaseClose(dh);
                     return SLP_ERROR_AUTHENTICATION_FAILED;
                  }
               }

#ifdef ENABLE_SLPv2_SECURITY
               if (entryreg->urlentry.authcount 
                     && entryreg->urlentry.authcount != reg->urlentry.authcount)
               {
                  SLPDatabaseClose(dh);
                  return SLP_ERROR_AUTHENTICATION_FAILED;
               }
#endif  
               /* Remove the identical entry */
               SLPDatabaseRemove(dh, entry);
               break;
            }
         }
      }

      /* add the new srvreg to the database */
      entry = SLPDatabaseEntryCreate(msg, buf);
      if (entry)
      {
         /* set the source (allows for quicker aging ) */
         if (msg->body.srvreg.source == SLP_REG_SOURCE_UNKNOWN)
         {
            if (SLPNetIsLoopback(&(msg->peer)))
               msg->body.srvreg.source = SLP_REG_SOURCE_LOCAL; 
            else
               msg->body.srvreg.source = SLP_REG_SOURCE_REMOTE;     
         }

         /* add to database */
         SLPDatabaseAdd(dh, entry);
         SLPDLogRegistration("Registration",entry);

         /* add a socket to listen for IPv6 requests */
         if (SLPNetIsIPV6() && msg->peer.ss_family == AF_INET6) 
         {
            if (msg->body.srvreg.source == SLP_REG_SOURCE_LOCAL) 
            {
               SLPIfaceGetInfo(G_SlpdProperty.interfaces, &ifaces, AF_INET6);
               for (i = 0; i < ifaces.iface_count; i++) 
                  if (IN6_IS_ADDR_LINKLOCAL(&(((struct sockaddr_in6 *)
                        &ifaces.iface_addr[i])->sin6_addr)))
                     SLPDIncomingAddService(msg->body.srvreg.srvtype, 
                           msg->body.srvreg.srvtypelen, &ifaces.iface_addr[i]);
            }
            else 
               SLPDIncomingAddService(msg->body.srvreg.srvtype, 
                     msg->body.srvreg.srvtypelen, &msg->peer);
         }
         result = 0; /* SUCCESS! */
      }
      else
         result = SLP_ERROR_INTERNAL_ERROR;
      SLPDatabaseClose(dh);
   }
   else
      result = SLP_ERROR_INTERNAL_ERROR;

   return result;
}

/** Remove a service registration from the database.
 *
 * @param[in] msg - Message interpreting an SrvDereg message.
 *
 * @return Zero on success, or a non-zero value on failure.
 */
int SLPDDatabaseDeReg(SLPMessage * msg)
{
   SLPDatabaseHandle dh;
   SLPDatabaseEntry * entry = 0;
   SLPSrvReg * entryreg;
   SLPSrvDeReg * dereg;
   char srvtype[MAX_HOST_NAME];
   size_t srvtypelen = 0;

   dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
   if (dh)
   {
      /* dereg is the SrvDereg being deregistered */
      dereg = &msg->body.srvdereg;

      /* check to see if there is an identical entry */
      while (1)
      {
         entry = SLPDatabaseEnum(dh);
         if (entry == 0) 
            break;

         /* entry reg is the SrvReg message from the database */
         entryreg = &entry->msg->body.srvreg;

         if (SLPCompareString(entryreg->urlentry.urllen, 
               entryreg->urlentry.url, dereg->urlentry.urllen,
               dereg->urlentry.url) == 0)
         {
            if (SLPIntersectStringList(entryreg->scopelistlen,
                  entryreg->scopelist, dereg->scopelistlen,
                  dereg->scopelist) > 0)
            {
               /* Check to ensure the source addr is the same as */
               /* the original */
               if (G_SlpdProperty.checkSourceAddr)
               {
                  if ((entry->msg->peer.ss_family == AF_INET 
                        && msg->peer.ss_family == AF_INET 
                        && memcmp(&(((struct sockaddr_in *)
                              &(entry->msg->peer))->sin_addr),
                              &(((struct sockaddr_in *)&(msg->peer))->sin_addr),
                              sizeof(struct in_addr))) 
                        || (entry->msg->peer.ss_family == AF_INET6 
                              && msg->peer.ss_family == AF_INET6 
                              && memcmp(&(((struct sockaddr_in6 *)
                                    &(entry->msg->peer))->sin6_addr),
                                    &(((struct sockaddr_in6 *)
                                          &(msg->peer))->sin6_addr),
                                    sizeof(struct in6_addr))))
                  {
                     SLPDatabaseClose(dh);
                     return SLP_ERROR_AUTHENTICATION_FAILED;
                  }
               }

#ifdef ENABLE_SLPv2_SECURITY
               if (entryreg->urlentry.authcount 
                     && entryreg->urlentry.authcount 
                           != dereg->urlentry.authcount)
               {
                  SLPDatabaseClose(dh);
                  return SLP_ERROR_AUTHENTICATION_FAILED;
               }
#endif                    

               /* save the srvtype for later */
               strncpy(srvtype, entryreg->srvtype, entryreg->srvtypelen);
               srvtypelen = entryreg->srvtypelen;

               /* remove the registration from the database */
               SLPDLogRegistration("Deregistration",entry);
               SLPDatabaseRemove(dh,entry);

               break;
            }
         }
      }
      SLPDatabaseClose(dh);

      if (entry != 0)
      {
         /* check to see if we can stop listening for service requests for this service */
         dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
         if (dh)
         {
            while (1)
            {
               entry = SLPDatabaseEnum(dh);
               if (entry == 0) 
                  break;

               entryreg = &entry->msg->body.srvreg;
               if (SLPCompareString(entryreg->srvtypelen, entryreg->srvtype,
                     srvtypelen, srvtype) == 0)
                  break;
            }
         }

         SLPDatabaseClose(dh);

         /* okay, remove the listening sockets */
         if (entry == 0)
            SLPDIncomingRemoveService(srvtype, srvtypelen);
      }
      else
         return SLP_ERROR_INVALID_REGISTRATION;
   }
   return 0;
}

/** Find services in the database.
 *
 * @param[in] msg - The SrvRqst to find.
 *
 * @param[out] result - The address of storage for the returned 
 *    result structure
 *
 * @return Zero on success, or a non-zero value on failure.
 *
 * @remarks Caller must pass @p result (dereferenced) to 
 *    SLPDDatabaseSrvRqstEnd to free.
 */
int SLPDDatabaseSrvRqstStart(SLPMessage * msg, 
      SLPDDatabaseSrvRqstResult ** result)
{
   SLPDatabaseHandle dh;
   SLPDatabaseEntry * entry;
   SLPSrvReg * entryreg;
   SLPSrvRqst * srvrqst;

#ifdef ENABLE_SLPv2_SECURITY
   int i;
#endif

   /* start with the result set to NULL just to be safe */
   *result = 0;

   dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
   if (dh)
   {
      /* srvrqst is the SrvRqst being made */
      srvrqst = &(msg->body.srvrqst);

      while (1)
      {
         /* allocate result with generous array of url entry pointers */
         *result = (SLPDDatabaseSrvRqstResult *)xrealloc(*result, 
               sizeof(SLPDDatabaseSrvRqstResult) + (sizeof(SLPUrlEntry*) 
                     * G_SlpdDatabase.urlcount));
         if (*result == 0)
         {
            /* out of memory */
            SLPDatabaseClose(dh);
            return SLP_ERROR_INTERNAL_ERROR;
         }
         (*result)->urlarray = (SLPUrlEntry **)((*result) + 1);
         (*result)->urlcount = 0;
         (*result)->reserved = dh;

         /* rewind enumeration in case we had to reallocate */
         SLPDatabaseRewind(dh);

         /* Check to see if there is matching entry */
         while (1)
         {
            entry = SLPDatabaseEnum(dh);
            if (entry == 0)
               return 0; /* This is the only successful way out */

            /* entry reg is the SrvReg message from the database */
            entryreg = &entry->msg->body.srvreg;

            /* check the service type */
            if (SLPCompareSrvType(srvrqst->srvtypelen, srvrqst->srvtype,
                  entryreg->srvtypelen, entryreg->srvtype) == 0 
                  && SLPIntersectStringList(entryreg->scopelistlen, 
                        entryreg->scopelist, srvrqst->scopelistlen,
                        srvrqst->scopelist) > 0)
            {

#ifdef ENABLE_PREDICATES
               if (SLPDPredicateTest(msg->header.version, 
                     entryreg->attrlistlen, entryreg->attrlist, 
                     srvrqst->predicatelen, srvrqst->predicate))
#endif
               {

#ifdef ENABLE_SLPv2_SECURITY
                  if (srvrqst->spistrlen)
                  {
                     for (i = 0; i < entryreg->urlentry.authcount; i++)
                        if (SLPCompareString(srvrqst->spistrlen, 
                              srvrqst->spistr, entryreg->urlentry.autharray
                                    [i].spistrlen, entryreg->urlentry.autharray
                                    [i].spistr) == 0)
                           break;

                     if (i == entryreg->urlentry.authcount)
                        continue;
                  }
#endif
                  if ((*result)->urlcount + 1 > G_SlpdDatabase.urlcount)
                  {
                     /* Oops we did not allocate a big enough result */
                     G_SlpdDatabase.urlcount *= 2;
                     break;
                  }
                  (*result)->urlarray[(*result)->urlcount] 
                        = &entryreg->urlentry;
                  (*result)->urlcount ++;
               }
            }
         }
      }
   }
   return 0;
}

/** Find services in the database.
 *
 * @param[in] result - The result structure previously passed to
 *    SLPDDatabaseSrvRqstStart.
 */
void SLPDDatabaseSrvRqstEnd(SLPDDatabaseSrvRqstResult * result)
{
   if (result)
   {
      SLPDatabaseClose((SLPDatabaseHandle)result->reserved);
      xfree(result);
   }
}

/** Find service types in the database.
 *
 * @param[in] msg - The SrvTypRqst to find.
 * @param[out] result - The address of storage for the result structure.
 *
 * @return Zero on success, or a non-zero value on failure.
 *
 * @remarks Caller must pass @p result (dereferenced) to 
 *    SLPDDatabaseSrvtypeRqstEnd to free it.
 */
int SLPDDatabaseSrvTypeRqstStart(SLPMessage * msg, 
      SLPDDatabaseSrvTypeRqstResult ** result)
{
   SLPDatabaseHandle dh;
   SLPDatabaseEntry * entry;
   SLPSrvReg * entryreg;
   SLPSrvTypeRqst * srvtyperqst;

   dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
   if (dh)
   {
      /* srvtyperqst is the SrvTypeRqst being made */
      srvtyperqst = &(msg->body.srvtyperqst);

      while (1)
      {
         /* allocate result with generous srvtypelist of url entry pointers */
         *result = (SLPDDatabaseSrvTypeRqstResult *)xrealloc(*result, 
               sizeof(SLPDDatabaseSrvTypeRqstResult) 
                     + G_SlpdDatabase.srvtypelistlen);
         if (*result == 0)
         {
            /* out of memory */
            SLPDatabaseClose(dh);
            return SLP_ERROR_INTERNAL_ERROR;
         }
         (*result)->srvtypelist = (char*)((*result) + 1);
         (*result)->srvtypelistlen = 0;
         (*result)->reserved = dh;

         /* rewind enumeration in case we had to reallocate */
         SLPDatabaseRewind(dh);

         while (1)
         {
            entry = SLPDatabaseEnum(dh);
            if (entry == 0)
               return 0; /* This is the only successful way out */

            /* entry reg is the SrvReg message from the database */
            entryreg = &entry->msg->body.srvreg;

            if (SLPCompareNamingAuth(entryreg->srvtypelen, entryreg->srvtype,
                     srvtyperqst->namingauthlen, srvtyperqst->namingauth) == 0 
                  && SLPIntersectStringList(srvtyperqst->scopelistlen,
                        srvtyperqst->scopelist, entryreg->scopelistlen,
                        entryreg->scopelist) 
                  && SLPContainsStringList((*result)->srvtypelistlen, 
                        (*result)->srvtypelist, entryreg->srvtypelen,
                        entryreg->srvtype) == 0)
            {
               /* Check to see if we allocated a big enough srvtypelist */
               if ((*result)->srvtypelistlen + entryreg->srvtypelen 
                     > G_SlpdDatabase.srvtypelistlen)
               {
                  /* Oops we did not allocate a big enough result */
                  G_SlpdDatabase.srvtypelistlen *= 2;
                  break;
               }

               /* Append a comma if needed */
               if ((*result)->srvtypelistlen)
               {
                  (*result)->srvtypelist[(*result)->srvtypelistlen] = ',';
                  (*result)->srvtypelistlen += 1;
               }

               /* Append the service type */
               memcpy(((*result)->srvtypelist) + (*result)->srvtypelistlen,
                     entryreg->srvtype, entryreg->srvtypelen);
               (*result)->srvtypelistlen += entryreg->srvtypelen;
            }
         }
      }
      SLPDatabaseClose(dh);
   }
   return 0;
}

/** Release resources used to find service types in the database.
 *
 * @param[in] result - A pointer to the result structure previously 
 *    passed to SLPDDatabaseSrvTypeRqstStart.
 */
void SLPDDatabaseSrvTypeRqstEnd(SLPDDatabaseSrvTypeRqstResult * result)
{
   if (result)
   {
      SLPDatabaseClose((SLPDatabaseHandle)result->reserved);
      xfree(result);
   }
}

/** Find attributes in the database.
 *
 * @param[in] msg - The AttrRqst to find.
 *
 * @param[out] result - The address of storage for the returned 
 *    result structure.
 *
 * @return Zero on success, or a non-zero value on failure.
 *
 * @remarks Caller must pass @p result (dereferenced) to 
 *    SLPDDatabaseAttrRqstEnd to free it.
 */
int SLPDDatabaseAttrRqstStart(SLPMessage * msg,
      SLPDDatabaseAttrRqstResult ** result)
{
   SLPDatabaseHandle dh;
   SLPDatabaseEntry * entry;
   SLPSrvReg * entryreg;
   SLPAttrRqst * attrrqst;

#ifdef ENABLE_SLPv2_SECURITY
   int i;
#endif

   *result = xmalloc(sizeof(SLPDDatabaseAttrRqstResult));
   if (*result == 0)
      return SLP_ERROR_INTERNAL_ERROR;

   memset(*result, 0, sizeof(SLPDDatabaseAttrRqstResult));
   dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
   if (dh)
   {
      (*result)->reserved = dh;

      /* attrrqst is the AttrRqst being made */
      attrrqst = &msg->body.attrrqst;

      while (1)
      {
         entry = SLPDatabaseEnum(dh);
         if (entry == 0)
            return 0;

         /* entry reg is the SrvReg message from the database */
         entryreg = &entry->msg->body.srvreg;

         if (SLPCompareString(attrrqst->urllen, attrrqst->url, 
                  entryreg->urlentry.urllen, entryreg->urlentry.url) == 0 
               || SLPCompareSrvType(attrrqst->urllen, attrrqst->url, 
                     entryreg->srvtypelen, entryreg->srvtype) == 0)
         {
            if (SLPIntersectStringList(attrrqst->scopelistlen, 
                  attrrqst->scopelist, entryreg->scopelistlen, 
                  entryreg->scopelist))
            {
               if (attrrqst->taglistlen == 0)
               {
#ifdef ENABLE_SLPv2_SECURITY
                  if (attrrqst->spistrlen)
                  {
                     for (i = 0; i < entryreg->authcount; i++)
                        if (SLPCompareString(attrrqst->spistrlen, 
                              attrrqst->spistr, 
                              entryreg->autharray[i].spistrlen,
                              entryreg->autharray[i].spistr) == 0)
                           break;

                     if (i == entryreg->authcount)
                        continue;
                  }
#endif
                  /* Send back what was registered */
                  (*result)->attrlistlen = entryreg->attrlistlen;
                  (*result)->attrlist = (char*)entryreg->attrlist;
                  (*result)->authcount = entryreg->authcount;
                  (*result)->autharray = entryreg->autharray;                        
               }
#ifdef ENABLE_PREDICATES
               else
               {
                  /* Send back a partial list as specified by taglist */
                  if (SLPDFilterAttributes(entryreg->attrlistlen, 
                        entryreg->attrlist, attrrqst->taglistlen,
                        attrrqst->taglist, &(*result)->attrlistlen,
                        &(*result)->attrlist) == 0)
                  {
                     (*result)->ispartial = 1;
                     break;
                  }
               }
#endif
            }
         }
      }
   }
   return 0;
}

/** Release resources used to find attributes in the database.
 *
 * @param[in] result - A pointer to the result structure previously 
 *    passed to SLPDDatabaseSrvTypeRqstStart.
 */
void SLPDDatabaseAttrRqstEnd(SLPDDatabaseAttrRqstResult * result)
{
   if (result)
   {
      SLPDatabaseClose((SLPDatabaseHandle)result->reserved);
      if (result->ispartial && result->attrlist) 
         xfree(result->attrlist);
      xfree(result);
   }
}

/** Start an enumeration of the entire database.
 *
 * @return An enumeration handle that is passed to subsequent calls to
 *    SLPDDatabaseEnum. Returns 0 on failure. Returned enumeration handle 
 *    (if not 0) must be passed to SLPDDatabaseEnumEnd when you are done 
 *    with it.
 */
void * SLPDDatabaseEnumStart(void)
{
   return SLPDatabaseOpen(&G_SlpdDatabase.database);   
}

/** Enumerate through all entries of the database.
 *
 * @param[in] eh - A pointer to opaque data that is used to maintain
 *    enumerate entries. Pass in a pointer to NULL to start the process.
 *
 * @param[out] msg - The address of storage for a SrvReg message object.
 * @param[out] buf - The address of storage for a SrvReg buffer object.
 *
 * @return A pointer to the enumerated entry or 0 if end of enumeration.
 */
SLPMessage * SLPDDatabaseEnum(void * eh, SLPMessage ** msg, SLPBuffer * buf)
{
   SLPDatabaseEntry * entry;
   entry = SLPDatabaseEnum((SLPDatabaseHandle)eh);
   if (entry)
   {
      *msg = entry->msg;
      *buf = entry->buf;
   }
   else
   {
      *msg = 0;
      *buf = 0;
   }
   return *msg;
}

/** End an enumeration started by SLPDDatabaseEnumStart.
 *
 * @param[in] eh - The enumeration handle returned by SLPDDatabaseEnumStart.
 */
void SLPDDatabaseEnumEnd(void * eh)
{
   if (eh)
      SLPDatabaseClose((SLPDatabaseHandle)eh);
}

/** Indicates whether or not the database is empty.
 *
 * @return A boolean value; True if the database is empty, False if not.
 */
int SLPDDatabaseIsEmpty(void)
{
   int result = 1;

   SLPDatabaseHandle dh;
   dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
   {
      result = SLPDatabaseCount(dh) == 0;
      SLPDatabaseClose(dh);
   }
   return result;
}

/** Initialize the database with registrations from a regfile.
 *
 * @param[in] regfile - The registration file to register. 
 *    Pass in 0 for no registration file.
 *
 * @return Zero on success, or a non-zero value on error.
 */
int SLPDDatabaseInit(const char * regfile)
{
   /* Set initial values */
   memset(&G_SlpdDatabase,0,sizeof(G_SlpdDatabase));
   G_SlpdDatabase.urlcount = SLPDDATABASE_INITIAL_URLCOUNT;
   G_SlpdDatabase.srvtypelistlen = SLPDDATABASE_INITIAL_SRVTYPELISTLEN;
   SLPDatabaseInit(&G_SlpdDatabase.database);

   /* Call the reinit function */
   return SLPDDatabaseReInit(regfile);
}

/** Re-initialize the database with changed registrations from a regfile.
 *
 * @param[in] regfile - The registration file to register.
 *
 * @return Zzero on success, or a non-zero value on error.
 */
int SLPDDatabaseReInit(const char * regfile)
{
   SLPDatabaseHandle dh;
   SLPDatabaseEntry * entry;
   SLPMessage * msg;
   SLPBuffer buf;
   FILE * fd;

   /* open the database handle and remove all the static registrations
      (the registrations from the /etc/slp.reg) file. */
   dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
   if (dh)
   {
      while (1)
      {
         entry = SLPDatabaseEnum(dh);
         if (entry == 0)
            break;

         if (entry->msg->body.srvreg.source == SLP_REG_SOURCE_STATIC)
            SLPDatabaseRemove(dh, entry);
      }
      SLPDatabaseClose(dh);
   }

   /* read static registration file if any */
   if (regfile)
   {
      fd = fopen(regfile, "rb");
      if (fd)
      {
         while (SLPDRegFileReadSrvReg(fd, &msg, &buf) == 0)
            SLPDDatabaseReg(msg, buf);

         fclose(fd);
      }
   }
   return 0;
}

#ifdef DEBUG
/** Cleans up all resources used by the database.
 */
void SLPDDatabaseDeinit(void)
{
   SLPDatabaseDeinit(&G_SlpdDatabase.database);
}

/** Dumps currently valid service registrations present with slpd.
 */
void SLPDDatabaseDump(void)
{
   SLPMessage * msg;
   SLPBuffer buf;
   void * eh;

   eh = SLPDDatabaseEnumStart();
   if (eh)
   {
      SLPDLog("\n========================================================================\n");
      SLPDLog("Dumping Registrations\n");
      SLPDLog("========================================================================\n");
      while (SLPDDatabaseEnum(eh, &msg, &buf))
      {
         SLPDLogMessageInternals(msg);
         SLPDLog("\n");
      }
      SLPDDatabaseEnumEnd(eh);
   }
}
#endif

/*=========================================================================*/
