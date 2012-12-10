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

/** Processes incoming SLP messages.
 *
 * @file       slpd_process.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#include "slpd_process.h"
#include "slpd_outgoing.h"
#include "slpd_property.h"
#include "slpd_database.h"
#include "slpd_knownda.h"
#include "slpd_log.h"

#ifdef ENABLE_SLPv2_SECURITY
# include "slpd_spi.h"
#endif

#include "slp_xmalloc.h"
#include "slp_message.h"
#include "slp_compare.h"
#include "slp_net.h"

#ifdef ENABLE_SLPv2_SECURITY
# include "slp_auth.h"
#endif

/** Process an SA SrvRequest message.
 *
 * @param[in] message - The message to process.
 * @param[out] sendbuf - The response buffer to fill.
 * @param[in] errorcode - The error code from the client request.
 *
 * @return Zero on success, or a non-zero SLP error on failure.
 *
 * @internal
 */
static int ProcessSASrvRqst(SLPMessage * message, SLPBuffer * sendbuf,
      int errorcode)
{
   char localaddr_str[INET6_ADDRSTRLEN + 2];
   size_t size = 0;
   SLPBuffer result = *sendbuf;

   if (message->body.srvrqst.scopelistlen == 0
         || SLPIntersectStringList(message->body.srvrqst.scopelistlen,
               message->body.srvrqst.scopelist, G_SlpdProperty.useScopesLen,
               G_SlpdProperty.useScopes) != 0)
   {
      /* send back a SAAdvert */

      /* ensure the buffer is big enough to handle the whole SAAdvert */
      size = message->header.langtaglen + 21;/* 14 bytes for header     */
                                             /*  2 bytes for url count  */
                                             /*  2 bytes for scope list len */
                                             /*  2 bytes for attr list len */
                                             /*  1 byte for authblock count */
      size += G_SlpdProperty.urlPrefixLen;
      localaddr_str[0] = 0;
      if (message->localaddr.ss_family == AF_INET)
         inet_ntop(message->localaddr.ss_family, &((struct sockaddr_in *)
               &message->localaddr)->sin_addr, localaddr_str,
               sizeof(localaddr_str));
      else if (message->localaddr.ss_family == AF_INET6)
      {
         strcpy(localaddr_str, "[");
         inet_ntop(message->localaddr.ss_family, &((struct sockaddr_in6 *)
               &message->localaddr)->sin6_addr, &localaddr_str[1],
               sizeof(localaddr_str) - 1);
         strcat(localaddr_str, "]");
      }
      size += strlen(localaddr_str);
      size += G_SlpdProperty.useScopesLen;

      /* TODO: size += G_SlpdProperty.SAAttributes */

      result = SLPBufferRealloc(result,size);
      if (result == 0)
      {
         /* TODO: out of memory, what should we do here! */
         errorcode = SLP_ERROR_INTERNAL_ERROR;
         goto FINISHED;
      }

      /* Add the header */

      /* version */
      *result->curpos++ = 2;

      /* function id */
      *result->curpos++ = SLP_FUNCT_SAADVERT;

      /* length */
      PutUINT24(&result->curpos, size);

      /* flags */
      PutUINT16(&result->curpos, (size > (size_t)G_SlpdProperty.MTU?
            SLP_FLAG_OVERFLOW: 0));

      /* ext offset */
      PutUINT24(&result->curpos, 0);

      /* xid */
      PutUINT16(&result->curpos, message->header.xid);

      /* lang tag len */
      PutUINT16(&result->curpos, message->header.langtaglen);

      /* lang tag */
      memcpy(result->curpos, message->header.langtag,
            message->header.langtaglen);
      result->curpos += message->header.langtaglen;

      /* Add rest of the SAAdvert */

      /* url len */
      PutUINT16(&result->curpos, G_SlpdProperty.urlPrefixLen
            + strlen(localaddr_str));

      /* url */
      memcpy(result->curpos, G_SlpdProperty.urlPrefix,
            G_SlpdProperty.urlPrefixLen);
      result->curpos += G_SlpdProperty.urlPrefixLen;
      memcpy(result->curpos, localaddr_str, strlen(localaddr_str));
      result->curpos += strlen(localaddr_str);

      /* scope list len */
      PutUINT16(&result->curpos, G_SlpdProperty.useScopesLen);

      /* scope list */
      memcpy(result->curpos, G_SlpdProperty.useScopes,
            G_SlpdProperty.useScopesLen);
      result->curpos += G_SlpdProperty.useScopesLen;

      /* attr list len */
      /* PutUINT16(&result->curpos, G_SlpdProperty.SAAttributesLen) */
      PutUINT16(&result->curpos, 0);

      /* attr list */
      /* memcpy(result->curpos, G_SlpdProperty.SAAttributes,
            G_SlpdProperty.SAAttributesLen) */

      /* authblock count */
      *result->curpos++ = 0;
   }
   else
      errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;

FINISHED:

   *sendbuf = result;

   return errorcode;
}

int CheckAndResizeBuffer(SLPBuffer * sendbuf, SLPBuffer tmp, size_t grow_size)
{
   int retVal = 0;

    if( (*sendbuf)->curpos + (tmp->end - tmp->start) > ( (*sendbuf)->start + (*sendbuf)->allocated) )
    {
        /* Grow the sendbuf buffer - note that SLPBufferRealloc may potentially do a memset if the DEBUG flag is set
           Therefore we have to copy the old contents into the buffer afterwards to be sure
        */
#ifdef DEBUG
        SLPBuffer duplicate = SLPBufferDup( (*sendbuf) );
#endif
        /* store how far we are from the start of the buffer */
        size_t currentPosFromStart = (*sendbuf)->curpos - (*sendbuf)->start;

        /* check to make sure we're growing by at least the size of the tmp buffer */
        *sendbuf = SLPBufferRealloc( (*sendbuf), ((*sendbuf)->allocated + ((size_t)(tmp->end - tmp->start) > grow_size?(tmp->end - tmp->start):grow_size)) );
         if (*sendbuf == 0)
         {
            retVal = SLP_ERROR_INTERNAL_ERROR;
         }
         else
         {
           /* update the current position to what it was before */
           (*sendbuf)->curpos = (*sendbuf)->start + currentPosFromStart;
#ifdef DEBUG
            memcpy( (*sendbuf)->start, duplicate->start, duplicate->end - duplicate->start );
#endif
         }
#ifdef DEBUG
        SLPBufferFree(duplicate);
        duplicate = 0;
#endif
    }

    return retVal;
}

/** Process a DA service request message.
 *
 * @param[in] message - The message to process.
 * @param[out] sendbuf - The response buffer to fill.
 * @param[in] errorcode - The error code from the client request.
 *
 * @return Zero on success, or a non-zero SLP error on failure.
 *
 * @internal
 */
static int ProcessDASrvRqst(SLPMessage * message, SLPBuffer * sendbuf, int errorcode)
{
   SLPBuffer tmp = 0;
   SLPMessage * msg = 0;
   void * eh = 0;
   /* TODO should really be a configurable property, maybe G_SlpdProperty.MTU? Left at 4096 to retain same behaviour */
   size_t initial_buffer_size = 4096;
   size_t grow_size = initial_buffer_size;

   /* Special case for when libslp asks slpd (through the loopback) about
    * a known DAs. Fill sendbuf with DAAdverts from all known DAs.
    */
   if (SLPNetIsLoopback(&message->peer))
   {
      *sendbuf = SLPBufferRealloc(*sendbuf, initial_buffer_size);
      if (*sendbuf == 0)
         return SLP_ERROR_INTERNAL_ERROR;

      if (errorcode == 0)
      {
         /* Note: The weird *sendbuf code is making a single SLPBuffer
          * that contains multiple DAAdverts.  This is a special
          * process that only happens for the DA SrvRqst through
          * loopback to the SLPAPI
          */

         /* If we are a DA, always have ourself at the start of the list, so the
          * lib requests can be handled locally for speed
          */
         if (G_SlpdProperty.isDA)
         {
            struct sockaddr_storage loaddr;

            if (SLPNetIsIPV4())
            {
               int addr = INADDR_LOOPBACK;
               SLPNetSetAddr(&loaddr, AF_INET, G_SlpdProperty.port, &addr);
            }
            else
            {
               SLPNetSetAddr(&loaddr, AF_INET6, G_SlpdProperty.port, &slp_in6addr_loopback);
            }

            if (0 == SLPDKnownDAGenerateMyDAAdvert(&loaddr, 0, 0, 0, message->header.xid, &tmp))
            {
               memcpy((*sendbuf)->curpos, tmp->start, tmp->end - tmp->start);
               (*sendbuf)->curpos = ((*sendbuf)->curpos) + (tmp->end - tmp->start);
               SLPBufferFree(tmp);
               tmp = 0;
            }
            else
            {
               SLPDLog("Unable to add initial DAAdvert due to error\n");
            }
         }

         eh = SLPDKnownDAEnumStart();
         if (eh)
         {
            while (1)
            {
               /* iterate over all database entries */
               if (SLPDKnownDAEnum(eh, &msg, &tmp) == 0)
               {
                  break;
               }

               /* if we resize succesfully.. */
               if ( CheckAndResizeBuffer(sendbuf, tmp, grow_size) == 0 )
               {
                  /* buffer should now be resized to an appropriate size to handle all current database entries */

                  /* TRICKY: Fix up the XID and clear the flags. */
                  tmp->curpos = tmp->start + 10;
                  TO_UINT16(tmp->curpos, message->header.xid);
                  if (*(tmp->start) == 1)
                     *(tmp->start + 4) = 0;
                  else
                     TO_UINT16(tmp->start + 5, 0);

                  /* copy all data out of tmp into the sendbuf */
                  memcpy((*sendbuf)->curpos, tmp->start, tmp->end - tmp->start);
                  /* increment the current position in sendbuf */
                  (*sendbuf)->curpos = ((*sendbuf)->curpos) + (tmp->end - tmp->start);
               }
               else
               {
                  errorcode = SLP_ERROR_INTERNAL_ERROR;
               }
            }
            SLPDKnownDAEnumEnd(eh);
         }
         /* tmp can store database entries which should not be freed by anyone else so reset the pointer to prevent double deletion */
         tmp = 0;

         /* Tack on a "terminator" DAAdvert
            Note that this function *always* returns the error code passed as its second parameter (or SLP_ERROR_INTERNAL_ERROR if the buffer fails to resize)
            The errorcode is also inserted into the srvrply header by this function
         */
         SLPDKnownDAGenerateMyDAAdvert(&message->localaddr, SLP_ERROR_INTERNAL_ERROR, 0, 0, message->header.xid, &tmp);
         if (!tmp)
            return SLP_ERROR_INTERNAL_ERROR;

         /* if we resize succesfully.. */
         if ( CheckAndResizeBuffer(sendbuf, tmp, grow_size) == 0 )
         {
            memcpy((*sendbuf)->curpos, tmp->start, tmp->end - tmp->start);
            (*sendbuf)->curpos = ((*sendbuf)->curpos) + (tmp->end - tmp->start);

            /* mark the end of the sendbuf */
            (*sendbuf)->end = (*sendbuf)->curpos;

         }
         else
         {
            errorcode = SLP_ERROR_INTERNAL_ERROR;
         }

         SLPBufferFree(tmp);
         tmp = 0;
      }
      return errorcode;
   }

   /* Normal case where a remote Agent asks for a DA */

   *sendbuf = SLPBufferRealloc(*sendbuf, G_SlpdProperty.MTU);
   if (*sendbuf == 0)
      return SLP_ERROR_INTERNAL_ERROR;
   if (G_SlpdProperty.isDA)
   {
      if (message->body.srvrqst.scopelistlen == 0
            || SLPIntersectStringList(message->body.srvrqst.scopelistlen,
                  message->body.srvrqst.scopelist, G_SlpdProperty.useScopesLen,
                  G_SlpdProperty.useScopes))
      {
         errorcode = SLPDKnownDAGenerateMyDAAdvert(&message->localaddr,
               errorcode, 0, 0, message->header.xid, sendbuf);
      }
      else
         errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;
   }
   else
      errorcode = SLP_ERROR_MESSAGE_NOT_SUPPORTED;

   /* don't return errorcodes to multicast messages */
   if (errorcode != 0 && *sendbuf)
   {
      if (message->header.flags & SLP_FLAG_MCAST
            || SLPNetIsMCast(&(message->peer)))
         (*sendbuf)->end = (*sendbuf)->start;
   }
   return errorcode;
}

/** Process a general service request message.
 *
 * @param[in] message - The message to process.
 * @param[out] sendbuf - The response buffer to fill.
 * @param[in] errorcode - The error code from the client request.
 *
 * @return Zero on success, or a non-zero SLP error on failure.
 *
 * @internal
 */
static int ProcessSrvRqst(SLPMessage * message, SLPBuffer * sendbuf,
      int errorcode)
{
   int i;
   SLPUrlEntry * urlentry;
   SLPDDatabaseSrvRqstResult * db = 0;
   size_t size = 0;
   SLPBuffer result = *sendbuf;

#ifdef ENABLE_SLPv2_SECURITY
   SLPAuthBlock * authblock = 0;
   int j;
#endif

   /* If errorcode is set, we can not be sure that message is good
      Go directly to send response code
    */
   if (errorcode)
      goto RESPOND;

   /* Check for one of our IP addresses in the prlist */
   if (SLPIntersectStringList(message->body.srvrqst.prlistlen,
         message->body.srvrqst.prlist, G_SlpdProperty.interfacesLen,
         G_SlpdProperty.interfaces))
   {
      /* silently ignore */
      result->end = result->start;
      goto FINISHED;
   }

   /* Make sure that we handle at least verify registrations made with
      the requested SPI.  If we can't then have to return an error
      because there is no way we can return URL entries that are
      signed in a way the requester can understand
    */
#ifdef ENABLE_SLPv2_SECURITY
   if (G_SlpdProperty.securityEnabled)
   {
      if (SLPSpiCanVerify(G_SlpdSpiHandle, message->body.srvrqst.spistrlen,
            message->body.srvrqst.spistr) == 0)
      {
         errorcode = SLP_ERROR_AUTHENTICATION_UNKNOWN;
         goto RESPOND;
      }
   }
   else if (message->body.srvrqst.spistrlen)
   {
      errorcode = SLP_ERROR_AUTHENTICATION_UNKNOWN;
      goto RESPOND;
   }
#else
   if (message->body.srvrqst.spistrlen)
   {
      errorcode = SLP_ERROR_AUTHENTICATION_UNKNOWN;
      goto RESPOND;
   }
#endif

   /* check to to see if a this is a special SrvRqst */
   if (SLPCompareString(message->body.srvrqst.srvtypelen,
         message->body.srvrqst.srvtype, 23, SLP_DA_SERVICE_TYPE) == 0)
   {
      errorcode = ProcessDASrvRqst(message, sendbuf, errorcode);
      if (errorcode == 0)
      {
         /* Since we have an errorcode of 0, we were successful,
            and have already formed a response packet; return now.
          */
         return errorcode;
      }
      goto RESPOND;
   }
   if (SLPCompareString(message->body.srvrqst.srvtypelen,
         message->body.srvrqst.srvtype, 21, SLP_SA_SERVICE_TYPE) == 0)
   {
      errorcode = ProcessSASrvRqst(message, sendbuf, errorcode);
      if (errorcode == 0)
      {
         /* Since we have an errorcode of 0, we were successful,
            and have already formed a response packet; return now.
          */
         return errorcode;
      }
      goto RESPOND;
   }

   /* make sure that we handle the scope */
   if (SLPIntersectStringList(message->body.srvrqst.scopelistlen,
         message->body.srvrqst.scopelist, G_SlpdProperty.useScopesLen,
         G_SlpdProperty.useScopes) != 0)
      errorcode = SLPDDatabaseSrvRqstStart(message, &db);
   else
      errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;

RESPOND:

   /* do not send error codes or empty replies to multicast requests */
   if (!db || errorcode != 0 || db->urlcount == 0)
   {
      if (message->header.flags & SLP_FLAG_MCAST
            || SLPNetIsMCast(&(message->peer)))
      {
         result->end = result->start;
         goto FINISHED;
      }
   }

   /* ensure the buffer is big enough to handle the whole srvrply */
   size = message->header.langtaglen + 18;/* 14 bytes for header     */
                                          /*  2 bytes for error code */
                                          /*  2 bytes for url count  */
   if (db && errorcode == 0)
   {
      for (i = 0; i < db->urlcount; i++)
      {
         /* urlentry is the url from the db result */
         urlentry = db->urlarray[i];

         size += urlentry->urllen + 6; /*  1 byte for reserved  */
                                       /*  2 bytes for lifetime */
                                       /*  2 bytes for urllen   */
                                       /*  1 byte for authcount */
#ifdef ENABLE_SLPv2_SECURITY
         /* make room to include the authblock that was asked for */
         if (G_SlpdProperty.securityEnabled
               && message->body.srvrqst.spistrlen)
         {
            for (j = 0; j < urlentry->authcount; j++)
            {
               if (SLPCompareString(urlentry->autharray[j].spistrlen,
                     urlentry->autharray[j].spistr,
                     message->body.srvrqst.spistrlen,
                     message->body.srvrqst.spistr) == 0)
               {
                  authblock = &(urlentry->autharray[j]);
                  size += authblock->length;
                  break;
               }
            }
         }
#endif
      }
   }

   /* reallocate the result buffer */
   result = SLPBufferRealloc(result, size);
   if (result == 0)
   {
      errorcode = SLP_ERROR_INTERNAL_ERROR;
      goto FINISHED;
   }

   /* add the header */

   /* version */
   *result->curpos++ = 2;

   /* function id */
   *result->curpos++ = SLP_FUNCT_SRVRPLY;

   /* length */
   PutUINT24(&result->curpos, size);

   /* flags */
   PutUINT16(&result->curpos, (size > (size_t)G_SlpdProperty.MTU?
         SLP_FLAG_OVERFLOW: 0));

   /* ext offset */
   PutUINT24(&result->curpos, 0);

   /* xid */
   PutUINT16(&result->curpos, message->header.xid);

   /* lang tag len */
   PutUINT16(&result->curpos, message->header.langtaglen);

   /* lang tag */
   memcpy(result->curpos, message->header.langtag,
         message->header.langtaglen);
   result->curpos += message->header.langtaglen;

   /* add rest of the SrvRply */

   /* error code*/
   PutUINT16(&result->curpos, errorcode);
   if (db && errorcode == 0)
   {
      /* urlentry count */
      PutUINT16(&result->curpos, db->urlcount);
      for (i = 0; i < db->urlcount; i++)
      {
         /* urlentry is the url from the db result */
         urlentry = db->urlarray[i];

#ifdef ENABLE_SLPv1
         if (urlentry->opaque == 0)
         {
            /* url-entry reserved */
            *result->curpos++ = 0;

            /* url-entry lifetime */
            PutUINT16(&result->curpos, urlentry->lifetime);

            /* url-entry urllen */
            PutUINT16(&result->curpos, urlentry->urllen);

            /* url-entry url */
            memcpy(result->curpos, urlentry->url, urlentry->urllen);
            result->curpos += urlentry->urllen;

            /* url-entry auths */
            *result->curpos++ = 0;
         }
         else
#endif
         {
            /* Use an opaque copy if available (and authentication is
             * not being used).
             */

            /* TRICKY: Fix up the lifetime. */
            TO_UINT16(urlentry->opaque + 1, urlentry->lifetime);
            memcpy(result->curpos, urlentry->opaque, urlentry->opaquelen);
            result->curpos += urlentry->opaquelen;
         }
      }
   }
   else
      PutUINT16(&result->curpos, 0); /* set urlentry count to 0*/

FINISHED:

   if (db)
      SLPDDatabaseSrvRqstEnd(db);

   *sendbuf = result;

   return errorcode;
}

/** Process a general service registration message.
 *
 * @param[in] message - The message to process.
 * @param[in] recvbuf - The buffer associated with @p message.
 * @param[out] sendbuf - The response buffer to fill.
 * @param[in] errorcode - The error code from the client request.
 *
 * @return A non-zero value if @p message should be silently dropped.
 *
 * @internal
 */
static int ProcessSrvReg(SLPMessage * message, SLPBuffer recvbuf,
      SLPBuffer * sendbuf, int errorcode)
{
   SLPBuffer result = *sendbuf;

   /* If errorcode is set, we can not be sure that message is good
      Go directly to send response code  also do not process mcast
      srvreg or srvdereg messages
    */
   if (errorcode || message->header.flags & SLP_FLAG_MCAST
         || SLPNetIsMCast(&(message->peer)))
      goto RESPOND;

   /* make sure that we handle the scope */
   if (SLPIntersectStringList(message->body.srvreg.scopelistlen,
         message->body.srvreg.scopelist, G_SlpdProperty.useScopesLen,
         G_SlpdProperty.useScopes))
   {

#ifdef ENABLE_SLPv2_SECURITY

      /* Validate the authblocks       */
      errorcode = SLPAuthVerifyUrl(G_SlpdSpiHandle, 0,
            &message->body.srvreg.urlentry);
      if (errorcode == 0)
      {
         errorcode = SLPAuthVerifyString(G_SlpdSpiHandle, 0,
               message->body.srvreg.attrlistlen,
               message->body.srvreg.attrlist,
               message->body.srvreg.authcount,
               message->body.srvreg.autharray);
      }
      if (errorcode == 0)
#endif
      {
         /* Put the registration in the database. */

         /* TRICKY: Remember the recvbuf was duplicated back in
          * SLPDProcessMessage
          */
         if (SLPNetIsLoopback(&(message->peer)))
            message->body.srvreg.source= SLP_REG_SOURCE_LOCAL;
         else
            message->body.srvreg.source = SLP_REG_SOURCE_REMOTE;

         errorcode = SLPDDatabaseReg(message, recvbuf);
      }
   }
   else
      errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;

RESPOND:

   /* don't send back reply anything multicast SrvReg (set result empty) */
   if (message->header.flags & SLP_FLAG_MCAST
         || SLPNetIsMCast(&(message->peer)))
   {
      result->end = result->start;
      goto FINISHED;
   }

   /* ensure the buffer is big enough to handle the whole srvack */
   result = SLPBufferRealloc(result, message->header.langtaglen + 16);
   if (result == 0)
   {
      errorcode = SLP_ERROR_INTERNAL_ERROR;
      goto FINISHED;
   }

   /* Add the header */

   /* version */
   *result->curpos++ = 2;

   /* function id */
   *result->curpos++ = SLP_FUNCT_SRVACK;

   /* length */
   PutUINT24(&result->curpos, message->header.langtaglen + 16);

   /* flags */
   PutUINT16(&result->curpos, 0);

   /* ext offset */
   PutUINT24(&result->curpos, 0);

   /* xid */
   PutUINT16(&result->curpos, message->header.xid);

   /* lang tag len */
   PutUINT16(&result->curpos, message->header.langtaglen);

   /* lang tag */
   memcpy(result->curpos, message->header.langtag,
         message->header.langtaglen);
   result->curpos += message->header.langtaglen;

   /* Add the errorcode */
   PutUINT16(&result->curpos, errorcode);

FINISHED:

   *sendbuf = result;
   return errorcode;
}

/** Process a general service deregistration message.
 *
 * @param[in] message - The message to process.
 * @param[out] sendbuf - The response buffer to fill.
 * @param[in] errorcode - The error code from the client request.
 *
 * @return A non-zero value if @p message should be silently dropped.
 *
 * @internal
 */
static int ProcessSrvDeReg(SLPMessage * message, SLPBuffer * sendbuf,
      int errorcode)
{
   SLPBuffer result = *sendbuf;

   /* If errorcode is set, we can not be sure that message is good
      Go directly to send response code  also do not process mcast
      srvreg or srvdereg messages
    */
   if (errorcode || message->header.flags & SLP_FLAG_MCAST)
      goto RESPOND;

   /* make sure that we handle the scope */
   if (SLPIntersectStringList(message->body.srvdereg.scopelistlen,
         message->body.srvdereg.scopelist, G_SlpdProperty.useScopesLen,
         G_SlpdProperty.useScopes))
   {

#ifdef ENABLE_SLPv2_SECURITY
      /* malidate the authblocks */
      errorcode = SLPAuthVerifyUrl(G_SlpdSpiHandle, 0,
            &message->body.srvdereg.urlentry);
      if (errorcode == 0)
#endif
      {
         /* remove the service from the database */
         errorcode = SLPDDatabaseDeReg(message);
      }
   }
   else
      errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;

RESPOND:

   /* don't do anything multicast SrvDeReg (set result empty) */
   if (message->header.flags & SLP_FLAG_MCAST
         || SLPNetIsMCast(&message->peer))
   {
      result->end = result->start;
      goto FINISHED;
   }

   /* ensure the buffer is big enough to handle the whole srvack */
   result = SLPBufferRealloc(result, message->header.langtaglen + 16);
   if (result == 0)
   {
      errorcode = SLP_ERROR_INTERNAL_ERROR;
      goto FINISHED;
   }

   /* Add the header */

   /* version */
   *result->curpos++ = 2;

   /* function id */
   *result->curpos++ = SLP_FUNCT_SRVACK;

   /* length */
   PutUINT24(&result->curpos, message->header.langtaglen + 16);

   /* flags */
   PutUINT16(&result->curpos, 0);

   /* ext offset */
   PutUINT24(&result->curpos, 0);

   /* xid */
   PutUINT16(&result->curpos, message->header.xid);

   /* lang tag len */
   PutUINT16(&result->curpos, message->header.langtaglen);

   /* lang tag */
   memcpy(result->curpos, message->header.langtag,
         message->header.langtaglen);
   result->curpos += message->header.langtaglen;

   /* Add the errorcode */
   PutUINT16(&result->curpos, errorcode);

FINISHED:

   *sendbuf = result;
   return errorcode;
}

/** Process a general request ACK message.
 *
 * @param[in] message - The message to process.
 * @param[out] sendbuf - The response buffer to fill.
 * @param[in] errorcode - The error code from the client request.
 *
 * @return Zero - always.
 *
 * @internal
 */
static int ProcessSrvAck(SLPMessage * message, SLPBuffer * sendbuf,
      int errorcode)
{
   /* Ignore SrvAck. Just return errorcode to caller */
   SLPBuffer result = *sendbuf;

   (void)message;
   (void)errorcode;

   result->end = result->start;
   return 0;
}

/** Process a general attribute request message.
 *
 * @param[in] message - The message to process.
 * @param[out] sendbuf - The response buffer to fill.
 * @param[in] errorcode - The error code from the client request.
 *
 * @return Zero on success, or a non-zero SLP error on failure.
 *
 * @internal
 */
static int ProcessAttrRqst(SLPMessage * message, SLPBuffer * sendbuf,
      int errorcode)
{
   SLPDDatabaseAttrRqstResult * db = 0;
   size_t size = 0;
   SLPBuffer result = *sendbuf;

#ifdef ENABLE_SLPv2_SECURITY
   int i;
   uint8_t * generatedauth = 0;
   int generatedauthlen = 0;
   uint8_t * opaqueauth = 0;
   int opaqueauthlen = 0;
#endif

   /* if errorcode is set, we can not be sure that message is good
      Go directly to send response code
    */
   if (errorcode)
      goto RESPOND;

   /* check for one of our IP addresses in the prlist */
   if (SLPIntersectStringList(message->body.attrrqst.prlistlen,
         message->body.attrrqst.prlist, G_SlpdProperty.interfacesLen,
         G_SlpdProperty.interfaces))
   {
      /* Silently ignore */
      result->end = result->start;
      goto FINISHED;
   }

   /* make sure that we handle the scope */
   if (SLPIntersectStringList(message->body.attrrqst.scopelistlen,
         message->body.attrrqst.scopelist, G_SlpdProperty.useScopesLen,
         G_SlpdProperty.useScopes))
   {
      /* Make sure that we handle at least verify registrations made with
         the requested SPI.  If we can't then have to return an error
         because there is no way we can return URL entries that ares
         signed in a way the requester can understand
       */
#ifdef ENABLE_SLPv2_SECURITY
      if (G_SlpdProperty.securityEnabled)
      {
         if (message->body.attrrqst.taglistlen == 0)
         {
            /* We can send back entire attribute strings without
               generating a new attribute authentication block
               we just use the one sent by the registering agent
               which we have to have been able to verify
             */
            if (SLPSpiCanVerify(G_SlpdSpiHandle,
                  message->body.attrrqst.spistrlen,
                  message->body.attrrqst.spistr) == 0)
            {
               errorcode = SLP_ERROR_AUTHENTICATION_UNKNOWN;
               goto RESPOND;
            }
         }
         else
         {
            /* We have to be able to *generate* (sign) authentication
               blocks for attrrqst with taglists since it is possible
               that the returned attributes are a subset of what the
               original registering agent sent
             */
            if (SLPSpiCanSign(G_SlpdSpiHandle,
                  message->body.attrrqst.spistrlen,
                  message->body.attrrqst.spistr) == 0)
            {
               errorcode = SLP_ERROR_AUTHENTICATION_UNKNOWN;
               goto RESPOND;
            }
         }
      }
      else
      {
         if (message->body.attrrqst.spistrlen)
         {
            errorcode = SLP_ERROR_AUTHENTICATION_UNKNOWN;
            goto RESPOND;
         }
      }
#else
      if (message->body.attrrqst.spistrlen)
      {
         errorcode = SLP_ERROR_AUTHENTICATION_UNKNOWN;
         goto RESPOND;
      }
#endif
      /* Find attributes in the database */
      errorcode = SLPDDatabaseAttrRqstStart(message,&db);
   }
   else
      errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;

RESPOND:

   /* do not send error codes or empty replies to multicast requests */
   if (errorcode != 0 || db->attrlistlen == 0)
   {
      if (message->header.flags & SLP_FLAG_MCAST
            || SLPNetIsMCast(&(message->peer)))
      {
         result->end = result->start;
         goto FINISHED;
      }
   }

   /* ensure the buffer is big enough to handle the whole attrrply */
   size = message->header.langtaglen + 19; /* 14 bytes for header     */
   /*  2 bytes for error code */
   /*  2 bytes for attr-list len */
   /*  1 byte for the authcount */
   if (errorcode == 0)
   {
      size += db->attrlistlen;

#ifdef ENABLE_SLPv2_SECURITY
      /* Generate authblock if necessary or just use the one was included
         by registering agent.  Reserve sufficent space for either case.
       */
      if (G_SlpdProperty.securityEnabled
            && message->body.attrrqst.spistrlen)
      {
         if (message->body.attrrqst.taglistlen == 0)
         {
            for (i = 0; i < db->authcount; i++)
            {
               if (SLPCompareString(db->autharray[i].spistrlen,
                     db->autharray[i].spistr, message->body.attrrqst.spistrlen,
                     message->body.attrrqst.spistr) == 0)
               {
                  opaqueauth = db->autharray[i].opaque;
                  opaqueauthlen = db->autharray[i].opaquelen;
                  break;
               }
            }
         }
         else
         {
            errorcode = SLPAuthSignString(G_SlpdSpiHandle,
                  message->body.attrrqst.spistrlen,
                  message->body.attrrqst.spistr,
                  db->attrlistlen, db->attrlist,
                  &generatedauthlen, &generatedauth);
            opaqueauthlen = generatedauthlen;
            opaqueauth = generatedauth;
         }
         size += opaqueauthlen;
      }
#endif

   }

   /* alloc the  buffer */
   result = SLPBufferRealloc(result, size);
   if (result == 0)
   {
      errorcode = SLP_ERROR_INTERNAL_ERROR;
      goto FINISHED;
   }

   /* Add the header */

   /* version */
   *result->curpos++ = 2;

   /* function id */
   *result->curpos++ = SLP_FUNCT_ATTRRPLY;

   /* length */
   PutUINT24(&result->curpos, size);

   /* flags */
   PutUINT16(&result->curpos, (size > (size_t)G_SlpdProperty.MTU?
         SLP_FLAG_OVERFLOW: 0));

   /* ext offset */
   PutUINT24(&result->curpos, 0);

   /* xid */
   PutUINT16(&result->curpos, message->header.xid);

   /* lang tag len */
   PutUINT16(&result->curpos, message->header.langtaglen);

   /* lang tag */
   memcpy(result->curpos, message->header.langtag,
         message->header.langtaglen);
   result->curpos += message->header.langtaglen;

   /* Add rest of the AttrRqst */

   /* error code*/
   PutUINT16(&result->curpos, errorcode);
   if (errorcode == 0)
   {
      /* attr-list len */
      PutUINT16(&result->curpos, db->attrlistlen);
      if (db->attrlistlen)
         memcpy(result->curpos, db->attrlist, db->attrlistlen);
      result->curpos += db->attrlistlen;

      /* authentication block */
#ifdef ENABLE_SLPv2_SECURITY
      if (opaqueauth)
      {
         /* authcount */
         *result->curpos++ = 1;
         memcpy(result->curpos, opaqueauth, opaqueauthlen);
         result->curpos += opaqueauthlen;
      }
      else
#endif
         *result->curpos++ = 0; /* authcount */
   }

FINISHED:

#ifdef ENABLE_SLPv2_SECURITY
   /* free the generated authblock if any */
   xfree(generatedauth);
#endif

   if (db)
      SLPDDatabaseAttrRqstEnd(db);

   *sendbuf = result;

   return errorcode;
}

/** Process a DAAdvert message.
 *
 * @param[in] message - The message to process.
 * @param[in] recvbuf - The buffer associated with @p message.
 * @param[out] sendbuf - The response buffer to fill.
 * @param[in] errorcode - The error code from the client request.
 *
 * @return Zero on success, or a non-zero SLP error on failure.
 *
 * @internal
 */
static int ProcessDAAdvert(SLPMessage * message, SLPBuffer recvbuf,
      SLPBuffer * sendbuf, int errorcode)
{
   SLPBuffer result = *sendbuf;

   /* If errorcode is set, we can not be sure that message is good
      Go directly to send response code
    */
   if (errorcode)
      goto RESPOND;

   /* If net.slp.passiveDADetection is turned off then we ignore
      DAAdverts with xid == 0
    */
   if (G_SlpdProperty.passiveDADetection == 0 && message->header.xid == 0)
   {
      /* do not ignore replies of our DiscoveryRequests made for
       * static and dhcp configured DAs. For now we check this by
       * testing if the sockaddr is on the outgoing socket list
       */
      if (!SLPDHaveOutgoingConnectedSocket(&message->peer))
         goto RESPOND;
   }

   /* If net.slp.DAActiveDiscoveryInterval == 0 then we ignore
      DAAdverts with xid != 0
    */
   if (G_SlpdProperty.DAActiveDiscoveryInterval == 0
         && message->header.xid != 0)
      goto RESPOND;

   /* Validate the authblocks       */
#ifdef ENABLE_SLPv2_SECURITY
   errorcode = SLPAuthVerifyDAAdvert(G_SlpdSpiHandle, 0,
         &message->body.daadvert);
   if (errorcode == 0)
#endif
   {
      /* Only process if errorcode is not set */
      if (message->body.daadvert.errorcode == SLP_ERROR_OK)
         errorcode = SLPDKnownDAAdd(message, recvbuf);
   }

RESPOND:

   /* DAAdverts should never be replied to.  Set result buffer to empty*/
   result->end = result->start;

   *sendbuf = result;

   return errorcode;
}

/** Process a SrvTypeRequest message.
 *
 * @param[in] message - The message to process.
 * @param[out] sendbuf - The response buffer to fill.
 * @param[in] errorcode - The error code from the client request.
 *
 * @return Zero on success, or a non-zero SLP error on failure.
 *
 * @internal
 */
static int ProcessSrvTypeRqst(SLPMessage * message, SLPBuffer * sendbuf,
      int errorcode)
{
   size_t size = 0;
   SLPDDatabaseSrvTypeRqstResult * db = 0;
   SLPBuffer result = *sendbuf;

   /* If errorcode is set, we can not be sure that message is good
      Go directly to send response code
    */
   if (errorcode)
      goto RESPOND;

   /* check for one of our IP addresses in the prlist */
   if (SLPIntersectStringList(message->body.srvtyperqst.prlistlen,
         message->body.srvtyperqst.prlist, G_SlpdProperty.interfacesLen,
         G_SlpdProperty.interfaces))
   {
      /* Silently ignore */
      result->end = result->start;
      goto FINISHED;
   }

   /* make sure that we handle the scope */
   if (SLPIntersectStringList(message->body.srvtyperqst.scopelistlen,
         message->body.srvtyperqst.scopelist, G_SlpdProperty.useScopesLen,
         G_SlpdProperty.useScopes) != 0)
      errorcode = SLPDDatabaseSrvTypeRqstStart(message, &db);
   else
      errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;

RESPOND:

   /* do not send error codes or empty replies to multicast requests */
   if (errorcode != 0 || db->srvtypelistlen == 0)
      if (message->header.flags & SLP_FLAG_MCAST
            || SLPNetIsMCast(&(message->peer)))
      {
         result->end = result->start;
         goto FINISHED;
      }

   /* ensure the buffer is big enough to handle the whole srvtyperply */
   size = message->header.langtaglen + 18;   /* 14 bytes for header     */
                                             /*  2 bytes for error code */
                                             /*  2 bytes for srvtype len */
   if (errorcode == 0)
      size += db->srvtypelistlen;

   /* Reallocate the result buffer */
   result = SLPBufferRealloc(result, size);
   if (result == 0)
   {
      errorcode = SLP_ERROR_INTERNAL_ERROR;
      goto FINISHED;
   }

   /* Add the header */

   /* version */
   *result->curpos++ = 2;

   /* function id */
   *result->curpos++ = SLP_FUNCT_SRVTYPERPLY;

   /* length */
   PutUINT24(&result->curpos, size);

   /* flags */
   PutUINT16(&result->curpos, (size > (size_t)G_SlpdProperty.MTU?
         SLP_FLAG_OVERFLOW: 0));

   /* ext offset */
   PutUINT24(&result->curpos, 0);

   /* xid */
   PutUINT16(&result->curpos, message->header.xid);

   /* lang tag len */
   PutUINT16(&result->curpos, message->header.langtaglen);

   /* lang tag */
   memcpy(result->curpos, message->header.langtag,
         message->header.langtaglen);
   result->curpos += message->header.langtaglen;

   /* Add rest of the SrvTypeRply */

   /* error code*/
   PutUINT16(&result->curpos, errorcode);
   if (errorcode == 0)
   {
      /* length of srvtype-list */
      PutUINT16(&result->curpos, db->srvtypelistlen);
      memcpy(result->curpos, db->srvtypelist, db->srvtypelistlen);
      result->curpos += db->srvtypelistlen;
   }

FINISHED:

   if (db)
      SLPDDatabaseSrvTypeRqstEnd(db);

   *sendbuf = result;

   return errorcode;
}

/** Process an SAAdvert message.
 *
 * @param[in] message - The message to process.
 * @param[out] sendbuf - The response buffer to fill.
 * @param[in] errorcode - The error code from the client request.
 *
 * @return The value of @p errorcode..
 *
 * @internal
 */
static int ProcessSAAdvert(SLPMessage * message, SLPBuffer * sendbuf,
      int errorcode)
{
   (void)message;

   /* Ignore all SAADVERTS */
   (*sendbuf)->end = (*sendbuf)->start;
   return errorcode;
}

/** Processes the recvbuf and places the results in sendbuf
 *
 * @param[in] peerinfo - The remote address the message was received from.
 * @param[in] localaddr - The local address the message was received on.
 * @param[in] recvbuf - The message to process.
 * @param[out] sendbuf - The address of storage for the results of the
 *    processed message.
 * @param[out] sendlist - if non-0, this function will prune the message
 *    with the processed xid from the sendlist.
 *
 * @return Zero on success if @p sendbuf contains a response to send,
 *    or a non-zero value if @p sendbuf does not contain a response
 *    to send.
 */
int SLPDProcessMessage(struct sockaddr_storage * peerinfo,
      struct sockaddr_storage * localaddr, SLPBuffer recvbuf,
      SLPBuffer * sendbuf, SLPList * psendlist)
{
   SLPHeader header;
   SLPMessage * message = 0;
   int errorcode = 0;

#ifdef DEBUG
   char addr_str[INET6_ADDRSTRLEN];
#endif

   SLPDLogMessage(SLPDLOG_TRACEMSG_IN, peerinfo, localaddr, recvbuf);

   /* set the sendbuf empty */
   if (*sendbuf)
      (*sendbuf)->end = (*sendbuf)->start;

   /* zero out the header before parsing it */
   memset(&header, 0, sizeof(header));

   /* Parse just the message header */
   recvbuf->curpos = recvbuf->start;
   errorcode = SLPMessageParseHeader(recvbuf, &header);

   /* Reset the buffer "curpos" pointer so that full message can be
      parsed later
    */
   recvbuf->curpos = recvbuf->start;

#if defined(ENABLE_SLPv1)
   /* if version == 1 and the header was correct then parse message as a version 1 message */
   if ((errorcode == 0) && (header.version == 1))
      errorcode = SLPDv1ProcessMessage(peerinfo, recvbuf, sendbuf);
   else
#endif
   if (errorcode == 0)
   {
      /* TRICKY: Duplicate SRVREG recvbufs *before* parsing them
       * we do this because we are going to keep track of in the
       * registration database.
       */
      if (header.functionid == SLP_FUNCT_SRVREG
            || header.functionid == SLP_FUNCT_DAADVERT)
      {
         recvbuf = SLPBufferDup(recvbuf);
         if (recvbuf == 0)
            return SLP_ERROR_INTERNAL_ERROR;
      }

      /* Allocate the message descriptor */
      message = SLPMessageAlloc();
      if (message)
      {
         /* Parse the message and fill out the message descriptor */
         errorcode = SLPMessageParseBuffer(peerinfo, localaddr,
               recvbuf, message);
         if (errorcode == 0)
         {
            /* Process messages based on type */
            switch (message->header.functionid)
            {
               case SLP_FUNCT_SRVRQST:
                  errorcode = ProcessSrvRqst(message, sendbuf, errorcode);
                  break;

               case SLP_FUNCT_SRVREG:
                  errorcode = ProcessSrvReg(message, recvbuf,
                        sendbuf, errorcode);
                  if (errorcode == 0)
                     SLPDKnownDAEcho(message, recvbuf);
                  break;

               case SLP_FUNCT_SRVDEREG:
                  errorcode = ProcessSrvDeReg(message, sendbuf, errorcode);
                  if (errorcode == 0)
                     SLPDKnownDAEcho(message, recvbuf);
                  break;

               case SLP_FUNCT_SRVACK:
                  errorcode = ProcessSrvAck(message, sendbuf, errorcode);
                  break;

               case SLP_FUNCT_ATTRRQST:
                  errorcode = ProcessAttrRqst(message, sendbuf, errorcode);
                  break;

               case SLP_FUNCT_DAADVERT:
                  errorcode = ProcessDAAdvert(message, recvbuf,
                        sendbuf, errorcode);
                  break;

               case SLP_FUNCT_SRVTYPERQST:
                  errorcode = ProcessSrvTypeRqst(message, sendbuf, errorcode);
                  break;

               case SLP_FUNCT_SAADVERT:
                  errorcode = ProcessSAAdvert(message, sendbuf, errorcode);
                  break;

               default:
                  /* Should never happen... but we're paranoid */
                  errorcode = SLP_ERROR_PARSE_ERROR;
                  break;
            }
         }
         else
            SLPDLogParseWarning(peerinfo, recvbuf);

       /*If there was a send list, prune the xid, since the request has been processed*/
       if(psendlist)
       {
          SLPHeader bufhead;
          SLPBuffer pnext;
          SLPBuffer pbuf = (SLPBuffer) psendlist->head;

          while(pbuf)
          {
            pnext = (SLPBuffer) pbuf->listitem.next;

            if((0 == SLPMessageParseHeader(pbuf, &bufhead)) && (bufhead.xid == header.xid))
               SLPBufferFree((SLPBuffer)SLPListUnlink(psendlist, (SLPListItem*)pbuf));
            else
               pbuf->curpos = pbuf->start;  /*We parsed the buffer enough to attempt the xid check, we need to reset it for the next parse*/

            pbuf = pnext;
          }
       }

         if (header.functionid == SLP_FUNCT_SRVREG
               || header.functionid == SLP_FUNCT_DAADVERT)
         {
            /* TRICKY: If this is a reg or daadvert message we do not free
             * the message descriptor or duplicated recvbuf because they are
             * being kept in the database!
             */
            if (errorcode == 0)
               goto FINISHED;

            /* TRICKY: If there is an error we need to free the
             * duplicated recvbuf
             */
            SLPBufferFree(recvbuf);
         }
         SLPMessageFree(message);
      }
      else
         errorcode = SLP_ERROR_INTERNAL_ERROR;  /* out of memory */
   }
   else
      SLPDLogParseWarning(peerinfo,recvbuf);

FINISHED:

#ifdef DEBUG
   if (errorcode)
      SLPDLog("\n*** DEBUG *** errorcode %i during processing "
            "of message from %s\n", errorcode, SLPNetSockAddrStorageToString(
            peerinfo, addr_str, sizeof(addr_str)));
#endif

   /* Log message silently ignored because of an error */
   if (errorcode)
      if (*sendbuf == 0 || (*sendbuf)->end == (*sendbuf)->start)
         SLPDLogMessage(SLPDLOG_TRACEDROP,peerinfo,localaddr,recvbuf);

   /* Log trace message */
   SLPDLogMessage(SLPDLOG_TRACEMSG_OUT, peerinfo, localaddr, *sendbuf);

   return errorcode;
}

/*=========================================================================*/
