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
 * (common/slp_database.c) is used for the underlying storage, and indexes on
 * service type and attributes are optionally maintained as balanced binary trees.
 *
 * @file       slpd_database.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com), Richard Morrell
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 *
 * @bug Attribute request for a service type rather than a URL should return a
 *      set of attributes formed from merging the value lists of all the entries
 *      conforming to that service type.  It currently returns the attributes from
 *      a single instance of that service type.
 */

#define _GNU_SOURCE
#include <string.h>

#include "../libslpattr/libslpattr.h"
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
#include "slpd_index.h"
#include "slp_debug.h"

/* Entries used in the "handles" array in the database entry */
#define HANDLE_ATTRS            0
#define HANDLE_SRVTYPE          1

static IndexTreeNode *srvtype_index_tree = (IndexTreeNode *)0;

#ifdef ENABLE_PREDICATES
/** A structure to hold a tag and its index tree
 */
typedef struct _SLPTagIndex
{
   struct _SLPTagIndex *next;
   IndexTreeNode *root_node;
   size_t tag_len;
   char tag[1];
} SLPTagIndex;

static SLPTagIndex *tag_index_head = (SLPTagIndex *)0;

/** Create a tag index structure for the given tag, and add it to the tag index list
 *
 * @param[in] tag - Pointer to a buffer containing the tag string
 * @param[in] tag_len - Length of the tag string
 *
 * @return SLP_ERROR_OK if the allocation succeeds, or SLP_ERROR_INTERNAL_ERROR
 *         if the allocation fails
 */
int addTagIndex(size_t tag_len, const char *tag)
{
   SLPTagIndex *entry = xmalloc(sizeof (SLPTagIndex) + tag_len);

   if (entry)
   {
      entry->root_node = (IndexTreeNode *)0;
      entry->tag_len = tag_len;
      strncpy(entry->tag, tag, tag_len);
      entry->next = tag_index_head;
      tag_index_head = entry;
      return SLP_ERROR_OK;
   }
   return SLP_ERROR_INTERNAL_ERROR;
}

/** Search the tag index list for the given tag.
 *
 * @param[in] tag - Pointer to a buffer containing the tag string
 * @param[in] tag_len - Length of the tag string
 *
 * @return Pointer to the tag index entry, or NULL if not found
 */
SLPTagIndex *findTagIndex(size_t tag_len, const char *tag)
{
   SLPTagIndex *entry = tag_index_head;

   while (entry)
   {
      if (entry->tag_len == tag_len)
      {
         /* Compare the requested tag and the entry's tag.
          * Case-insensitive comparison to match the code in
          * attr_val_find_str
          */
         if (strncasecmp(entry->tag, tag, tag_len) == 0)
            break;
      }
      entry = entry->next;
   }
   return entry;
}
#endif /* ENABLE_PREDICATES */

/** A structure to hold an allocated service type and its length.
 */
typedef struct
{
   char *srvtype;
   size_t srvtypelen;
} SLPNormalisedSrvtype;

/** Takes a service type, and creates a normalised version of it.
 *
 * @param[in] srvtypelen - Length of the service type
 * @param[in] srvtype - Pointer to a buffer containing the service type
 * @param[out] normalisedSrvtypelen - Length of the normalised service type
 * @param[out] normalisedSrvtype - Buffer pointer to return the normalised service type
 *
 * @return SLP_ERROR_OK if the conversion can succeed, or SLP_ERROR_INTERNAL_ERROR
 *         if the buffer for the normalised service type cannot be allocated
 *
 * @remarks The "service:" prefix is ignored, if present
 */
static int getNormalisedSrvtype(size_t srvtypelen, const char* srvtype, size_t *normalisedSrvtypeLen, char * *normalisedSrvtype)
{
    /* Normalisation may involve collapsing escaped character sequences,
     * and should never be longer than the service type itself, but allow
     * for the addition of a trailing NUL
     */
   *normalisedSrvtype = (char *)xmalloc(srvtypelen+1);
   if (!*normalisedSrvtype)
      return SLP_ERROR_INTERNAL_ERROR;

   /* The "service:" prefix is optional, so we strip it before the character normalisation */
   if (srvtypelen >= 8 && strncmp(srvtype, "service:", 8) == 0)
   {
      srvtype += 8;
      srvtypelen -= 8;
   }
   *normalisedSrvtypeLen = SLPNormalizeString(srvtypelen, srvtype, *normalisedSrvtype, 1);
   return SLP_ERROR_OK;
}

/** Takes a service type, and outputs a structure holding the normalised service type and its length
 *
 * @param[in] srvtypelen - Length of the service type
 * @param[in] srvtype - Pointer to a buffer containing the service type
 * @param[out] ppNormalisedSrvtype - Buffer pointer to return the allocated structure
 *
 * @return SLP_ERROR_INTERNAL_ERROR if the structure cannot be allocated, SLP_ERROR_OK otherwise
 */
static int createNormalisedSrvtype(size_t srvtypelen, const char* srvtype, SLPNormalisedSrvtype **ppNormalisedSrvtype)
{
   int result;

   *ppNormalisedSrvtype = (SLPNormalisedSrvtype *)xmalloc(sizeof (SLPNormalisedSrvtype));
   if (!*ppNormalisedSrvtype)
      return SLP_ERROR_INTERNAL_ERROR;

   result = getNormalisedSrvtype(srvtypelen, srvtype, &(*ppNormalisedSrvtype)->srvtypelen, &(*ppNormalisedSrvtype)->srvtype);
   if (result != SLP_ERROR_OK)
   {
      xfree(*ppNormalisedSrvtype);
      *ppNormalisedSrvtype = (SLPNormalisedSrvtype *)0;
   }

   return result;
}

/** Takes a normalised service type structure, and frees the allocated buffer, and the structure itself
 *
 * @param[in] pNormalisedSrvtype - Pointer to the allocated structure
 */
static void freeNormalisedSrvtype(SLPNormalisedSrvtype *pNormalisedSrvtype)
{
   if (pNormalisedSrvtype)
   {
      xfree(pNormalisedSrvtype->srvtype);
      xfree(pNormalisedSrvtype);
   }
}

/** Remove an entry from the database.
 *
 * @param[in] dh - database handle
 * @param[in] entry - to be removed
 *
 * Frees up the normalised service type, and the parsed attributes, and deals with
 * cleaning up the service type and attribute indexes, as well as freeing the entry itself.
 */
void SLPDDatabaseRemove(SLPDatabaseHandle dh, SLPDatabaseEntry * entry)
{
   SLPNormalisedSrvtype *pNormalisedSrvtype = (SLPNormalisedSrvtype *)entry->handles[HANDLE_SRVTYPE];
   SLPAttributes slp_attr = (SLPAttributes)entry->handles[HANDLE_ATTRS];

   if (G_SlpdProperty.srvtypeIsIndexed)
   {
      /* Remove the index entries for this entry */
      srvtype_index_tree = index_tree_delete(srvtype_index_tree, pNormalisedSrvtype->srvtypelen, pNormalisedSrvtype->srvtype, (void *)entry);
   }

   if (pNormalisedSrvtype)
      freeNormalisedSrvtype(pNormalisedSrvtype);

   if (slp_attr)
   {
#ifdef ENABLE_PREDICATES
      /* Parsed attributes - remove them from the attribute indexes, if necessary */
      if (G_SlpdProperty.indexedAttributes)
      {
         const char * tag;
         SLPType type;
         SLPAttrIterator iter_h;
         SLPError err = SLPAttrIteratorAlloc(slp_attr, &iter_h);

         if (err == SLP_OK)
         {
            /* For each attribute name */
            while (SLPAttrIterNext(iter_h, (char const * *) &tag, &type) == SLP_TRUE)
            {
               /* Ensure it is a string value */
               if (type == SLP_STRING)
               {
                  /* Check to see if it is indexed */
                  SLPTagIndex *tag_index = findTagIndex(strlen(tag), tag);
                  if (tag_index)
                  {
                     SLPValue value;
                     /* For each value in the attribute's list */
                     while (SLPAttrIterValueNext(iter_h, &value) == SLP_TRUE)
                     {
                        /* Remove the value from the index */
                        tag_index->root_node = index_tree_delete(tag_index->root_node, value.len, value.data.va_str, (void *)entry);
                     }
                  }
               }
            }
         }
      }
#endif

      /* De-allocate the attribute structures */
      SLPAttrFree(slp_attr);
   }

   /* Now remove the entry itself */
   SLPDatabaseRemove(dh, entry);
}


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

   if ((dh = SLPDatabaseOpen(&G_SlpdDatabase.database)) != 0)
   {
      SLPDatabaseEntry * entry;

      while ((entry = SLPDatabaseEnum(dh)) != 0)
      {
         /* srvreg is the SrvReg message from the database */
         SLPSrvReg * srvreg = &entry->msg->body.srvreg;

         /* If the entry is local and it's configured for pid watching and 
          * its pid is invalid, then notify DA's and remove it.
          */
         if (srvreg->source == SLP_REG_SOURCE_LOCAL 
               && srvreg->pid != 0 && !SLPPidExists(srvreg->pid))
         {
            if (G_SlpdProperty.traceReg)
            {
               char buffer[50];
               sprintf(buffer, "PID Watcher Deregistration (pid=%d)", (int)srvreg->pid);
               SLPDLogRegistration(buffer, entry);
            }
            SLPDKnownDADeRegisterWithAllDas(entry->msg, entry->buf);
            SLPDDatabaseRemove(dh, entry);
            continue;
         }

         /* Don't age entries whose lifetime is set to SLP_LIFETIME_MAXIMUM
          * if we've been told not to age all entries, or if the entry came 
          * from the local static registration file.
          */
         if (srvreg->urlentry.lifetime == SLP_LIFETIME_MAXIMUM
               && (!ageall || srvreg->source == SLP_REG_SOURCE_STATIC || srvreg->source == SLP_REG_SOURCE_LOCAL))
            continue;

         /* Age entry and remove those that have timed out */
         srvreg->urlentry.lifetime -= seconds;
         if (srvreg->urlentry.lifetime <= 0)
         {
            SLPDLogRegistration("Timeout", entry);
            SLPDDatabaseRemove(dh, entry);
         }
      }
      SLPDatabaseClose(dh);
   }
}

/** Checks if a srvtype is already in the database
 *
 * @param [in] srvtype
 * @param [in] srvtypelen
 *
 * @return 1 if the srvtype is already in the database
 */
int SLPDDatabaseSrvtypeUsed(const char* srvtype, size_t srvtypelen)
{
   SLPDatabaseHandle dh;
   SLPDatabaseEntry * entry;
   SLPSrvReg * entryreg;
   int result = 0;

   dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
   if (dh)
   {
      while (1)
      {
         entry = SLPDatabaseEnum(dh);
         if (entry == 0) 
            break;

         /* entry reg is the SrvReg message from the database */
         entryreg = &entry->msg->body.srvreg;
         if(SLPCompareString(entryreg->srvtypelen, entryreg->srvtype, srvtypelen, srvtype) == 0)
         {
            result = 1;
            break;
         }
      }
      SLPDatabaseClose(dh);
   }

   return result;
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

   /* add a socket to listen for IPv6 requests if we haven't already (includes if this one was already registered) */
   if (SLPNetIsIPV6() && 
       !SLPDDatabaseSrvtypeUsed(msg->body.srvreg.srvtype, msg->body.srvreg.srvtypelen))
   {
      SLPIfaceGetInfo(G_SlpdProperty.interfaces, &ifaces, AF_INET6);
      for (i = 0; i < ifaces.iface_count; i++) 
         if(ifaces.iface_addr[i].ss_family == AF_INET6)
            SLPDIncomingAddService(msg->body.srvreg.srvtype, 
                  msg->body.srvreg.srvtypelen, &ifaces.iface_addr[i]);
   }

   dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
   if (dh)
   {
      SLPNormalisedSrvtype *pNormalisedSrvtype = (SLPNormalisedSrvtype *)0;
      SLPAttributes attr = (SLPAttributes)0;
#ifdef ENABLE_PREDICATES
      char attrnull;
#endif

      /* Get the normalised service type */
      result = createNormalisedSrvtype(reg->srvtypelen, reg->srvtype, &pNormalisedSrvtype);
      if (result != SLP_ERROR_OK)
      {
         SLPDatabaseClose(dh);
         return SLP_ERROR_INTERNAL_ERROR;
      }

#ifdef ENABLE_PREDICATES
      /* Get the parsed attributes */

      /* TRICKY: Temporarily NULL terminate the attribute list. We can do this
       * because there is room in the corresponding SLPv2 SRVREG SRVRQST
       * messages. Basically we are squashing the authcount and 
       * the spi string length. Don't worry, we fix things up later and it is 
       * MUCH faster than a malloc for a new buffer 1 byte longer!
       */
      attrnull = reg->attrlist[reg->attrlistlen];
      ((char *) reg->attrlist)[reg->attrlistlen] = 0;

      /* Generate an SLPAttr from the comma delimited list */
      if (SLPAttrAlloc("en", NULL, SLP_FALSE, &attr) == 0)
      {
         if (SLPAttrFreshen(attr, reg->attrlist) != 0)
         {
            /* Invalid attributes */
            SLPAttrFree(attr);
            attr = (SLPAttributes)0;
         }
      }

      /* Restore the overwritten byte */
      ((char *) reg->attrlist)[reg->attrlistlen] = attrnull;
#endif

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
                     freeNormalisedSrvtype(pNormalisedSrvtype);
                     if (attr)
                        SLPAttrFree(attr);
                     return SLP_ERROR_AUTHENTICATION_FAILED;
                  }
               }

#ifdef ENABLE_SLPv2_SECURITY
               if (entryreg->urlentry.authcount 
                     && entryreg->urlentry.authcount != reg->urlentry.authcount)
               {
                  SLPDatabaseClose(dh);
                  freeNormalisedSrvtype(pNormalisedSrvtype);
                  if (attr)
                     SLPAttrFree(attr);
                  return SLP_ERROR_AUTHENTICATION_FAILED;
               }
#endif  
               /* Remove the identical entry */
               SLPDDatabaseRemove(dh, entry);
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

         /* Update the service type index with the new entry */
         entry->handles[HANDLE_SRVTYPE] = (void *)pNormalisedSrvtype;
         if (G_SlpdProperty.srvtypeIsIndexed)
         {
            srvtype_index_tree = add_to_index(srvtype_index_tree, pNormalisedSrvtype->srvtypelen, pNormalisedSrvtype->srvtype, (void *)entry);
         }

         /* Update the attribute indexes */
         entry->handles[HANDLE_ATTRS] = (void *)attr;
#ifdef ENABLE_PREDICATES
         if (attr && G_SlpdProperty.indexedAttributes)
         {
            const char * tag;
            SLPType type;
            SLPAttrIterator iter_h;
            SLPError err = SLPAttrIteratorAlloc(attr, &iter_h);

            if (err == SLP_OK)
            {
               /* For each attribute name */
               while (SLPAttrIterNext(iter_h, (char const * *) &tag, &type) == SLP_TRUE)
               {
                  /* Ensure it is a string value */
                  if (type == SLP_STRING)
                  {
                     /* Check to see if it is indexed */
                     SLPTagIndex *tag_index = findTagIndex(strlen(tag), tag);
                     if (tag_index)
                     {
                        SLPValue value;
                        /* For each value in the attribute's list */
                        while (SLPAttrIterValueNext(iter_h, &value) == SLP_TRUE)
                        {
                           /* Add the value to the index */
                           tag_index->root_node = add_to_index(tag_index->root_node, value.len, value.data.va_str, (void *)entry);
                        }
                     }
                  }
               }
            }
         }
#endif

         if (G_SlpdProperty.traceReg)
         {
            char buffer[40];
            sprintf(buffer, "Registration (pid=%d)", (int)reg->pid);
            SLPDLogRegistration(buffer, entry);
         }

         result = 0; /* SUCCESS! */
      }
      else
      {
         result = SLP_ERROR_INTERNAL_ERROR;
         freeNormalisedSrvtype(pNormalisedSrvtype);
      }
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
               SLPDDatabaseRemove(dh,entry);

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

/** Test an entry for whether it should be returned.
 *
 * @param[in] msg - request message.
 * @param[in] entry - database entry to be tested.
 *
 * @return Non-zero if the entry matches the request, and should be returned,
 *         zero otherwise
 */
static int SLPDDatabaseSrvRqstTestEntry(
   SLPMessage * msg, 
#ifdef ENABLE_PREDICATES
   SLPDPredicateTreeNode * predicate_parse_tree,
#endif
   SLPDatabaseEntry * entry)
{
   SLPSrvReg * entryreg;
   SLPSrvRqst * srvrqst;

#ifdef ENABLE_SLPv2_SECURITY
   int i;
#endif

   /* srvrqst is the SrvRqst being made */
   srvrqst = &(msg->body.srvrqst);

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
      if (SLPDPredicateTestTree(predicate_parse_tree, (SLPAttributes)entry->handles[HANDLE_ATTRS]))
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
               return 0;
         }
#endif
         return 1;
      }
   }
   return 0;
}

typedef struct
{
   SLPMessage *                  msg;
   SLPDDatabaseSrvRqstResult **  result;
#ifdef ENABLE_PREDICATES
   SLPDPredicateTreeNode *       predicate_parse_tree;
#endif
   int                           error_code;
} SLPDDatabaseSrvRqstStartIndexCallbackParams;

/** Filter services in the database via an index.
 *
 * @param[in] cookie - context data.
 * @param[in] p - database entry.
 *
 * @remarks @p p represents an entry with a matching srvtype
 */
static void SLPDDatabaseSrvRqstStartIndexCallback(void * cookie, void *p)
{
   SLPDDatabaseSrvRqstStartIndexCallbackParams *params;
   SLPMessage * msg;
   SLPDDatabaseSrvRqstResult ** result;
   SLPDatabaseEntry * entry;
#ifdef ENABLE_PREDICATES
   SLPDPredicateTreeNode * predicate_parse_tree;
#endif
   SLPSrvReg * entryreg;

   params = (SLPDDatabaseSrvRqstStartIndexCallbackParams *)cookie;
   if (params->error_code)
   {
      return;
   }

   msg = params->msg;
   result = params->result;
#ifdef ENABLE_PREDICATES
   predicate_parse_tree = params->predicate_parse_tree;
#endif
   entry = (SLPDatabaseEntry *)p;

   if (SLPDDatabaseSrvRqstTestEntry(msg,
#ifdef ENABLE_PREDICATES
                                    predicate_parse_tree,
#endif
                                    entry))
   {
      if ((*result)->urlcount + 1 > G_SlpdDatabase.urlcount)
      {
         /* Oops we did not allocate a big enough result */
         params->error_code = 1;
         return;
      }
      /* entry reg is the SrvReg message from the database */
      entryreg = &entry->msg->body.srvreg;

      (*result)->urlarray[(*result)->urlcount] 
            = &entryreg->urlentry;
      (*result)->urlcount ++;
   }
}

/** Find services in the database via the srvtype index.
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
static int SLPDDatabaseSrvRqstStartIndexType(SLPMessage * msg,
#ifdef ENABLE_PREDICATES
      SLPDPredicateTreeNode *parse_tree,
#endif
      SLPDDatabaseSrvRqstResult ** result)
{
   SLPSrvRqst * srvrqst;
   size_t srvtypelen;
   const char *srvtype;
   size_t normalized_srvtypelen;
   char *normalized_srvtype;
   SLPDDatabaseSrvRqstStartIndexCallbackParams params;

   /* srvrqst is the SrvRqst being made */
   srvrqst = &(msg->body.srvrqst);

   srvtypelen = srvrqst->srvtypelen;
   srvtype = srvrqst->srvtype;

   if (srvtypelen >= 8)
   {
      if (strncasecmp(srvtype, "service:", 8) == 0)
      {
         /* Skip "service:" */
         srvtypelen -= 8;
         srvtype += 8;
      }
   }
   /* the index contains normalized service type strings, so normalize the
    * string we want to search for
    */
   normalized_srvtype = (char *)xmalloc(srvtypelen + 1);
   normalized_srvtypelen = SLPNormalizeString(srvtypelen, srvtype, normalized_srvtype, 1);
   normalized_srvtype[normalized_srvtypelen] = '\0';

   /* Search the srvtype index */
   params.msg = msg;
   params.result = result;
#ifdef ENABLE_PREDICATES
   params.predicate_parse_tree = parse_tree;
#endif
   params.error_code = 0;
   find_and_call(srvtype_index_tree,
      normalized_srvtypelen,
      normalized_srvtype,
      SLPDDatabaseSrvRqstStartIndexCallback,
      (void *)&params);

   xfree(normalized_srvtype);
   return params.error_code;
}

#ifdef ENABLE_PREDICATES
/** Find services in the database via an attribute index.
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
static int SLPDDatabaseSrvRqstStartIndexAttribute(
      size_t search_str_len,
      const char *search_str,
      IndexTreeNode * attribute_index,
      SLPMessage * msg,
      SLPDPredicateTreeNode *parse_tree,
      SLPDDatabaseSrvRqstResult ** result)
{
   size_t processed_searchstr_len;
   char *processed_searchstr;
   SLPDDatabaseSrvRqstStartIndexCallbackParams params;
   char * wildcard;
   SLPError err;

   /* We can use the index in two ways
    * If the string we are searching for is not wildcarded, then we
    * can simply use the index to filter all entries which match the
    * full string.
    * If the string we are searching for is wildcarded, then we can
    * use the index to filter all entries which start with the part of
    * the string preceding the first wildcard character.
    */
   wildcard = memchr(search_str, '*', search_str_len);
   if (wildcard)
   {
      /** found wildcard character - this can't be the first character
       *  as we check for that before calling this function
       */
      SLP_ASSERT(wildcard != search_str);
      search_str_len = wildcard - search_str;
   }

   /* the index contains processed attribute strings, so process the
    * string we're searching for in the same way
    */
   if ((err = SLPAttributeSearchString(search_str_len, search_str, &processed_searchstr_len, &processed_searchstr)) != SLP_OK)
      return (int)err;

   /* Search the index */
   params.msg = msg;
   params.result = result;
   params.predicate_parse_tree = parse_tree;
   params.error_code = 0;
   if (wildcard)
      find_leading_and_call(attribute_index,
         processed_searchstr_len,
         processed_searchstr,
         SLPDDatabaseSrvRqstStartIndexCallback,
         (void *)&params);
   else
      find_and_call(attribute_index,
         processed_searchstr_len,
         processed_searchstr,
         SLPDDatabaseSrvRqstStartIndexCallback,
         (void *)&params);

   xfree(processed_searchstr);
   return params.error_code;
}
#endif /* ENABLE_PREDICATES */

/** Find services in the database via linear scan.
 *
 * @param[in] msg - The SrvRqst to find.
 *
 * @param[out] result - The address of storage for the returned 
 *    result structure
 *
 * @return Zero on success, or a non-zero value on failure (not enough result entries).
 *
 * @remarks Caller must pass @p result (dereferenced) to 
 *    SLPDDatabaseSrvRqstEnd to free.
 */
static int SLPDDatabaseSrvRqstStartScan(SLPMessage * msg,
#ifdef ENABLE_PREDICATES
      SLPDPredicateTreeNode *parse_tree,
#endif
      SLPDDatabaseSrvRqstResult ** result)
{
   SLPDatabaseHandle dh;
   SLPDatabaseEntry * entry;
   SLPSrvReg * entryreg;
   SLPSrvRqst * srvrqst;

#ifdef ENABLE_SLPv2_SECURITY
   int i;
#endif

   dh = (*result)->reserved;
   if (dh)
   {
      /* srvrqst is the SrvRqst being made */
      srvrqst = &(msg->body.srvrqst);

      /* Check to see if there is matching entry */
      while (1)
      {
         entry = SLPDatabaseEnum(dh);
         if (entry == 0)
            return 0; /* This is the only successful way out */

         if (SLPDDatabaseSrvRqstTestEntry(msg,
#ifdef ENABLE_PREDICATES
                                          parse_tree,
#endif
                                          entry))
         {
            if ((*result)->urlcount + 1 > G_SlpdDatabase.urlcount)
            {
               /* Oops we did not allocate a big enough result */
               return 1;
            }
            /* entry reg is the SrvReg message from the database */
            entryreg = &entry->msg->body.srvreg;

            (*result)->urlarray[(*result)->urlcount] 
                  = &entryreg->urlentry;
            (*result)->urlcount ++;
         }
      }
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
   SLPSrvRqst * srvrqst;

   int start_result;
   int use_index = 0;
   
#ifdef ENABLE_SLPv2_SECURITY
   int i;
#endif

#ifdef ENABLE_PREDICATES
   SLPDPredicateTreeNode * predicate_parse_tree = (SLPDPredicateTreeNode *)0;
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

         /* Check if we can use the srvtype index */
         if (G_SlpdProperty.srvtypeIsIndexed)
         {
            size_t srvtypelen = srvrqst->srvtypelen;
            const char *srvtype = srvrqst->srvtype;
            if (srvtypelen >= 8)
            {
               if (strncasecmp(srvtype, "service:", 8) == 0)
               {
                  /* Skip "service:" */
                  srvtypelen -= 8;
                  srvtype += 8;
               }
            }
            if (memchr(srvtype, ':', srvtypelen))
            {
               /* Searching for a concrete type - can use the index */
               use_index = 1;
            }
         }

#ifdef ENABLE_PREDICATES
         /* Generate the predicate parse tree */
         if (srvrqst->predicatelen > 0)
         {
            /* TRICKY: Temporarily NULL-terminate the predicate string.  We can do
             * this because there is room in the corresponding SLPv2 SRVREG message.
             * Don't worry, we fix things up later and it is MUCH faster than a
             * malloc for a new buffer 1 byte longer!
             */
            SLPDPredicateParseResult err;
            const char *end;
            char *prednullptr = (char *)&srvrqst->predicate[srvrqst->predicatelen];
            char prednull;

            prednull = *prednullptr;
            *prednullptr = '\0';

#if defined(ENABLE_SLPv1)
            if (msg->header.version == 1)
               err = createPredicateParseTreev1(srvrqst->predicate, &end, &predicate_parse_tree, SLPD_PREDICATE_RECURSION_DEPTH);
            else
#endif
               err = createPredicateParseTree(srvrqst->predicate, &end, &predicate_parse_tree, SLPD_PREDICATE_RECURSION_DEPTH);

            /* Restore the squashed byte */
            *prednullptr = prednull;

            if (err == PREDICATE_PARSE_OK && end != prednullptr)
            {
               /* Found trash characters after the predicate - discard the parse tree before aborting */
               SLPDLog("Trash after predicate\n");
               freePredicateParseTree(predicate_parse_tree);
               return 0;
            }
            else if (err != PREDICATE_PARSE_OK)
            {
               SLPDLog("Invalid predicate\n");
               /* Nothing matches an invalid predicate */
               return 0;
            }
         }
#endif /* ENABLE_PREDICATES */

         if (use_index)
            start_result = SLPDDatabaseSrvRqstStartIndexType(msg,
#ifdef ENABLE_PREDICATES
                                                             predicate_parse_tree,
#endif
                                                             result);
         else
         {
#ifdef ENABLE_PREDICATES
            /* Analyse the parse tree to see if we can use the attribute indexes.
             * If the top node is a leaf node whose attribute name is indexed, or
             * if the top node is an AND node and one of its sub-nodes is a leaf
             * node whose attribute name is indexed, and the leaf node's type is
             * EQUALS, then we can use the index for that attribute name, unless
             * the search value starts with a wildcard character.
             */
            SLPTagIndex * tag_index = (SLPTagIndex *)0;
            SLPDPredicateTreeNode * tag_node = (SLPDPredicateTreeNode *)0;

            if (predicate_parse_tree)
            {
               if (predicate_parse_tree->nodeType == EQUAL && predicate_parse_tree->nodeBody.comparison.value_str[0] != WILDCARD)
               {
                  tag_index = findTagIndex(predicate_parse_tree->nodeBody.comparison.tag_len, predicate_parse_tree->nodeBody.comparison.tag_str);
                  if (tag_index)
                     tag_node = predicate_parse_tree;
               }
               else if (predicate_parse_tree->nodeType == NODE_AND)
               {
                  SLPDPredicateTreeNode * sub_node = predicate_parse_tree->nodeBody.logical.first;
                  while (sub_node)
                  {
                     if (sub_node->nodeType == EQUAL && sub_node->nodeBody.comparison.value_str[0] != WILDCARD)
                     {
                        tag_index = findTagIndex(sub_node->nodeBody.comparison.tag_len, sub_node->nodeBody.comparison.tag_str);
                        if (tag_index)
                        {
                           tag_node = sub_node;
                           break;
                        }
                     }
                     sub_node = sub_node->next;
                  }
               }
            }
            if (tag_index)
            {
               start_result = SLPDDatabaseSrvRqstStartIndexAttribute(
                  tag_node->nodeBody.comparison.value_len,
                  tag_node->nodeBody.comparison.value_str,
                  tag_index->root_node,
                  msg,
                  predicate_parse_tree,
                  result);
            }
            else

#endif /* ENABLE_PREDICATES */

               start_result = SLPDDatabaseSrvRqstStartScan(msg,
#ifdef ENABLE_PREDICATES
                                                           predicate_parse_tree,
#endif
                                                           result);
         }
#ifdef ENABLE_PREDICATES
         if (predicate_parse_tree)
            freePredicateParseTree(predicate_parse_tree);
#endif
         if (start_result == 0)
            return 0;

         /* We didn't allocate enough URL entries - loop round after updating the number to allocate */
         G_SlpdDatabase.urlcount *= 2;
      }
   }
   return 0;
}

/** Clean up at the end of a search.
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
               /* Check to see if we allocated a big enough srvtypelist, not forgetting to allow for a comma separator */
               if ((*result)->srvtypelistlen + entryreg->srvtypelen + 1
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

/** Process an entry and incorporate its attributes into the result.
 *
 * @param[in] msg - request message.
 * @param[in] entry - database entry to be tested.
 *
 * @return Non-zero if the entry matches the request, and should be returned,
 *         zero otherwise
 */
static int SLPDDatabaseAttrRqstProcessEntry(
   SLPAttrRqst * attrrqst,
   SLPSrvReg * entryreg,
   SLPDDatabaseAttrRqstResult ** result)
{
#ifdef ENABLE_SLPv2_SECURITY
   int i;
#endif

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
                  return 0;
            }
#endif
            /* Send back what was registered */
            (*result)->attrlistlen = entryreg->attrlistlen;
            (*result)->attrlist = (char*)entryreg->attrlist;
            (*result)->authcount = entryreg->authcount;
            (*result)->autharray = entryreg->autharray;                        
            return 1;
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
               return 1;
            }
         }
#endif
      }
   }
   return 0;
}

typedef struct
{
   SLPMessage *                   msg;
   SLPDDatabaseAttrRqstResult **  result;
   int                            error_code;
} SLPDDatabaseAttrRqstStartIndexCallbackParams;

/** Handle a database entry located in the database via an index.
 *
 * @param[in] cookie - context data.
 * @param[in] p - database entry.
 *
 * @remarks @p p represents an entry with a matching srvtype
 */
static void SLPDDatabaseAttrRqstStartIndexCallback(void * cookie, void *p)
{
   SLPDDatabaseAttrRqstStartIndexCallbackParams * params = (SLPDDatabaseAttrRqstStartIndexCallbackParams *)cookie;
   SLPDDatabaseAttrRqstResult ** result = params->result;
   SLPMessage * msg;
   SLPDatabaseEntry * entry;

   entry = (SLPDatabaseEntry *)p;
   msg = params->msg;

   (void)SLPDDatabaseAttrRqstProcessEntry(&msg->body.attrrqst, &entry->msg->body.srvreg, result);
}

/** Find attributes in the database via srvtype index
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
int SLPDDatabaseAttrRqstStartIndexType(SLPMessage * msg,
      SLPDDatabaseAttrRqstResult ** result)
{
   SLPAttrRqst * attrrqst;
   size_t srvtypelen;
   const char *srvtype;
   const char *p;
   SLPNormalisedSrvtype * pNormalisedSrvtype;
   SLPDDatabaseAttrRqstStartIndexCallbackParams params;
   char urlnull;
   char *urlnullptr;

#ifdef ENABLE_SLPv2_SECURITY
   int i;
#endif

   /* attrrqst is the AttrRqst being made */
   attrrqst = &msg->body.attrrqst;

   srvtypelen = attrrqst->urllen;
   srvtype = attrrqst->url;

   /* Temporarily NUL-terminate the URL to use strstr */
   urlnullptr = (char *)&srvtype[srvtypelen];
   urlnull = *urlnullptr;

   /* See if we have a full url, or just a srvtype */
   p = strstr(srvtype, "://");

   /* Restore the squashed byte */
   *urlnullptr = urlnull;

   if (p)
   {
      /* full URL - srvtype is the first part */
      srvtypelen = p - srvtype;
   }

   /* index contains normalised service types, so normalise this one before searching for it */
   if (createNormalisedSrvtype(srvtypelen, srvtype, &pNormalisedSrvtype) == SLP_ERROR_OK)
   {
      /* Search the srvtype index */
      params.msg = msg;
      params.result = result;
      params.error_code = 0;
      find_and_call(
         srvtype_index_tree,
         pNormalisedSrvtype->srvtypelen,
         pNormalisedSrvtype->srvtype,
         SLPDDatabaseAttrRqstStartIndexCallback,
         (void *)&params);

      freeNormalisedSrvtype(pNormalisedSrvtype);
   }

   return 0;
}

/** Find attributes in the database via linear scan
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
int SLPDDatabaseAttrRqstStartScan(SLPMessage * msg,
      SLPDDatabaseAttrRqstResult ** result)
{
   SLPDatabaseHandle dh;
   SLPDatabaseEntry * entry;
   SLPSrvReg * entryreg;
   SLPAttrRqst * attrrqst;

#ifdef ENABLE_SLPv2_SECURITY
   int i;
#endif

   dh = (*result)->reserved;
   if (dh)
   {
      /* attrrqst is the AttrRqst being made */
      attrrqst = &msg->body.attrrqst;

      /* Check to see if there is matching entry */
      while (1)
      {
         entry = SLPDatabaseEnum(dh);
         if (entry == 0)
            return 0;

         /* entry reg is the SrvReg message from the database */
         entryreg = &entry->msg->body.srvreg;

         if (SLPDDatabaseAttrRqstProcessEntry(attrrqst, entryreg, result) != 0)
            return 0;
      }
   }
   return 0;
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
   int start_result = 1;

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

      /* Check if we can use the srvtype index */
      if (G_SlpdProperty.srvtypeIsIndexed)
      {
         start_result = SLPDDatabaseAttrRqstStartIndexType(msg, result);
      }
      else
      {
         start_result = SLPDDatabaseAttrRqstStartScan(msg, result);
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
#ifdef ENABLE_PREDICATES
   char *taglist;
#endif

   /* Set initial values */
   memset(&G_SlpdDatabase,0,sizeof(G_SlpdDatabase));
   G_SlpdDatabase.urlcount = SLPDDATABASE_INITIAL_URLCOUNT;
   G_SlpdDatabase.srvtypelistlen = SLPDDATABASE_INITIAL_SRVTYPELISTLEN;
   SLPDatabaseInit(&G_SlpdDatabase.database);

#ifdef ENABLE_PREDICATES
   /* Initialise the tag indexes */
   /* Assumption: indexedAttributes cannot change after startup */
   taglist = G_SlpdProperty.indexedAttributes;
   if (taglist)
   {
      /* A tag list is like a set of attributes containing only keywords */
      int result = SLP_ERROR_OK;
      const char * tag;
      SLPType type;
      SLPAttrIterator iter_h;
      SLPAttributes attr;

      if (SLPAttrAlloc("en", NULL, SLP_FALSE, &attr) == 0)
      {
         if (SLPAttrFreshen(attr, taglist) != 0)
         {
            SLPDLog("List of indexed attributes is invalid\n");
            result = SLP_ERROR_INTERNAL_ERROR;
         }
         else
         {
            /* Now process the list, checking all the "attributes" are keywords */
            SLPError err = SLPAttrIteratorAlloc(attr, &iter_h);

            if (err == SLP_OK)
            {
               /* For each attribute name */
               while (SLPAttrIterNext(iter_h, (char const * *) &tag, &type) == SLP_TRUE)
               {
                  /* Ensure it is a keyword value */
                  if (type == SLP_KEYWORD)
                  {
                     /* Set up the index for this tag */
                     if ((err = addTagIndex(strlen(tag), tag)) != SLP_ERROR_OK)
                     {
                        SLPDLog("Error creating index for indexed attributes\n");
                        result = SLP_ERROR_INTERNAL_ERROR;
                        break;
                     }
                  }
                  else
                  {
                     /* Non-keyword value */
                     SLPDLog("Error processing list of indexed attributes\n");
                     result = SLP_ERROR_INTERNAL_ERROR;
                     break;
                  }
               }
            }
            else
            {
               SLPDLog("Memory allocation fail processing list of indexed attributes\n");
               result = SLP_ERROR_INTERNAL_ERROR;
            }
         }
      }
      else
      {
         SLPDLog("Memory allocation fail processing list of indexed attributes\n");
         result = SLP_ERROR_INTERNAL_ERROR;
      }
      SLPAttrFree(attr);
      if (result != SLP_ERROR_OK)
         return result;
   }
#endif /* ENABLE_PREDICATES */

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
            SLPDDatabaseRemove(dh, entry);
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

#ifdef DEBUG
void SLPDDatabaseUsr1(void)
{
   extern void print_tree(IndexTreeNode *root_node, unsigned depth);
   SLPTagIndex *tag_index;
   if (!srvtype_index_tree)
      SLPDLog("Service type index is empty\n");
   else
   {
      SLPDLog("Service type index tree:\n");
      print_tree(srvtype_index_tree, 1);
   }
   if (!tag_index_head)
      SLPDLog("Tag index list is empty\n");
   else
   {
      for (tag_index = tag_index_head; tag_index; tag_index = tag_index->next)
      {
         char buffer[100];
         strncpy(buffer, tag_index->tag, tag_index->tag_len);
         buffer[tag_index->tag_len] = '\0';
         SLPDLog("Tag Index Tree for %s\n", buffer);
         print_tree(tag_index->root_node, 1);
      }
   }
}
#endif
/*=========================================================================*/
