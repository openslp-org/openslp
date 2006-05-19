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

/** Processes incoming SLPv1 messages
 *
 * @file       slpd_v1process.c
 * @author     Matthew Peterson, Ganesan Rajagopal, 
 *             John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#include "slpd_process.h"
#include "slpd_property.h"
#include "slpd_database.h"
#include "slpd_knownda.h"
#include "slpd_log.h"

#include "slp_xmalloc.h"
#include "slp_message.h"
#include "slp_v1message.h"
#include "slp_utf8.h"
#include "slp_compare.h"
#include "slp_net.h"

/** Process an SLPv1 DA Service Request.
 *
 * @param[in] peeraddr - The address of the remote client.
 * @param[in] localaddr - The locally bound address for this message.
 * @param[in] message - The SRV request message.
 * @param[out] sendbuf - The response buffer to be sent back to the client.
 * @param[in] errorcode - The error code from the client request. 
 *
 * @return Zero on success, or a non-zero value on failure.
 *
 * @internal
 */
static int v1ProcessDASrvRqst(struct sockaddr_storage * peeraddr, 
      struct sockaddr_storage * localaddr, SLPMessage * message, 
      SLPBuffer * sendbuf, int errorcode)
{
   if (message->body.srvrqst.scopelistlen == 0 
         || SLPIntersectStringList(message->body.srvrqst.scopelistlen, 
               message->body.srvrqst.scopelist, G_SlpdProperty.useScopesLen,
               G_SlpdProperty.useScopes))
      errorcode = SLPDKnownDAGenerateMyV1DAAdvert(localaddr, errorcode,
            message->header.encoding, message->header.xid, sendbuf);
   else
      errorcode =  SLP_ERROR_SCOPE_NOT_SUPPORTED;

   /* don't return errorcodes to multicast messages */
   if (errorcode == 0)
      if (message->header.flags & SLP_FLAG_MCAST || SLPNetIsMCast(peeraddr))
      {
         (*sendbuf)->end = (*sendbuf)->start;
         return errorcode;
      }

   return errorcode;
}

/** Process an SLPv1 general Service Request.
 *
 * @internal
 */
static int v1ProcessSrvRqst(struct sockaddr_storage * peeraddr, 
      struct sockaddr_storage * localaddr, SLPMessage * message, 
      SLPBuffer * sendbuf, int errorcode)
{
   int i;
   size_t urllen;
   size_t size = 0;
   SLPDDatabaseSrvRqstResult * db = 0;
   SLPBuffer result = *sendbuf;

   /* If errorcode is set, we can not be sure that message is good
      Go directly to send response code */
   if (errorcode)
      goto RESPOND;

   /* check for one of our IP addresses in the prlist */
   if (SLPIntersectStringList(message->body.srvrqst.prlistlen, 
         message->body.srvrqst.prlist, G_SlpdProperty.interfacesLen, 
         G_SlpdProperty.interfaces))
   {
      result->end = result->start;
      goto FINISHED;
   }

   /* check to to see if a this is a special SrvRqst */
   if (SLPCompareString(message->body.srvrqst.srvtypelen, 
         message->body.srvrqst.srvtype, 15, "directory-agent") == 0)
   {
      errorcode = v1ProcessDASrvRqst(peeraddr, localaddr, message, 
            sendbuf, errorcode);
      return errorcode;
   }

   /* make sure that we handle the scope */
   if (SLPIntersectStringList(message->body.srvrqst.scopelistlen,
         message->body.srvrqst.scopelist, G_SlpdProperty.useScopesLen,
         G_SlpdProperty.useScopes) != 0) /* find services in the database */
      errorcode = SLPDDatabaseSrvRqstStart(message, &db);
   else
      errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;

RESPOND:

   /* do not send error codes or empty replies to multicast requests */
   if (message->header.flags & SLP_FLAG_MCAST)
      if (errorcode != 0 || db->urlcount == 0)
      {
         result->end = result->start;
         goto FINISHED;  
      }

   /* ensure the buffer is big enough to handle the whole srvrply */
   size = 16; /* 12 bytes for header, 2 bytes for error code, 2 bytes
   for url count */
   if (errorcode == 0)
   {
      for (i = 0; i < db->urlcount; i++)
      {
         urllen = INT_MAX;
         errorcode = SLPv1ToEncoding(0, &urllen, message->header.encoding,
               db->urlarray[i]->url, db->urlarray[i]->urllen);
         if (errorcode)
            break;
         size += urllen + 4; /* 2 bytes for lifetime, 2 bytes for urllen */
      } 
      result = SLPBufferRealloc(result, size);
      if (result == 0)
         errorcode = SLP_ERROR_INTERNAL_ERROR;
   }

   /* add the header */

   /* version */
   *result->curpos++ = 1;

   /* function id */
   *result->curpos++ = SLP_FUNCT_SRVRPLY;

   /* length */
   PutUINT16(&result->curpos, size);

   /* flags */
   /** @todo Set the flags correctly. */
   *result->curpos++ = (uint8_t)(message->header.flags 
         | (size > SLP_MAX_DATAGRAM_SIZE? SLPv1_FLAG_OVERFLOW: 0));

   /* dialect */
   *result->curpos++ = 0;

   /* language code */
   memcpy(result->curpos, message->header.langtag, 2);
   result->curpos += 2;
   PutUINT16(&result->curpos, message->header.encoding);

   /* xid */
   PutUINT16(&result->curpos, message->header.xid);

   /* add rest of the SrvRply */

   /* error code*/
   PutUINT16(&result->curpos, errorcode);
   if (errorcode == 0)
   {
      /* urlentry count */
      PutUINT16(&result->curpos, db->urlcount);
      for (i = 0; i < db->urlcount; i++)
      {
         /* url-entry lifetime */
         PutUINT16(&result->curpos, db->urlarray[i]->lifetime);

         /* url-entry url and urllen */
         urllen = size;      
         errorcode = SLPv1ToEncoding((char *)(result->curpos + 2), 
               &urllen, message->header.encoding, db->urlarray[i]->url, 
               db->urlarray[i]->urllen);
         PutUINT16(&result->curpos, urllen);
         result->curpos += urllen;
      }
   }
   else  /* urlentry count */
      PutUINT16(&result->curpos, 0);

FINISHED:   

   SLPDDatabaseSrvRqstEnd(db);
   *sendbuf = result;
   return errorcode;
}

/** Process an SLPv1 general Service Registration.
 *
 * @param[in] peeraddr - The remote address of the client.
 * @param[in] message - The client message object.
 * @param[in] recvbuf - The client message receive buffer.
 * @param[in] sendbuf - The server response send buffer.
 * @param[in] errorcode - The error code from the client message.
 *
 * @return Non-zero if message should be silently dropped.
 *
 * @internal
 */
static int v1ProcessSrvReg(struct sockaddr_storage * peeraddr, 
      SLPMessage * message, SLPBuffer recvbuf, SLPBuffer * sendbuf, 
      int errorcode)
{
   SLPBuffer result = *sendbuf;

   (void)peeraddr;

   /* If errorcode is set, we can not be sure that message is good
      Go directly to send response code  also do not process mcast
      srvreg or srvdereg messages */

   if (errorcode || message->header.flags & SLP_FLAG_MCAST)
      goto RESPOND;

   /* make sure that we handle the scope */
   if (SLPIntersectStringList(message->body.srvreg.scopelistlen,
         message->body.srvreg.scopelist, G_SlpdProperty.useScopesLen,
         G_SlpdProperty.useScopes))
   {
      /* put the service in the database */
      if (SLPNetIsLocal(&message->peer))
         message->body.srvreg.source= SLP_REG_SOURCE_LOCAL;
      else
         message->body.srvreg.source = SLP_REG_SOURCE_REMOTE;

      errorcode = SLPDDatabaseReg(message,recvbuf);
   }
   else
      errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;

RESPOND:    

   /* don't send back reply anything multicast SrvReg (set result empty) */
   if (message->header.flags & SLP_FLAG_MCAST)
   {
      result->end = result->start;
      goto FINISHED;
   }

   /* ensure the buffer is big enough to handle the whole srvack */
   result = SLPBufferRealloc(result, 14);
   if (result == 0)
   {
      errorcode = SLP_ERROR_INTERNAL_ERROR;
      goto FINISHED;
   }

   /* add the header */

   /* version */
   *result->curpos++ = 1;

   /* function id */
   *result->curpos++   = SLP_FUNCT_SRVACK;

   /* length */
   PutUINT16(&result->curpos, 14);

   /* flags */
   /** @todo Set the flags correctly. */
   *result->curpos++ = 0;

   /* dialect */
   *result->curpos++ = 0;

   /* language code */
   memcpy(result->curpos, message->header.langtag, 2);
   result->curpos += 2;
   PutUINT16(&result->curpos, message->header.encoding);

   /* xid */
   PutUINT16(&result->curpos, message->header.xid);

   /* add the errorcode */
   PutUINT16(&result->curpos, errorcode);

FINISHED:

   *sendbuf = result;
   return errorcode;
}

/** Process an SLPv1 general Service Deregistration.
 *
 * @param[in] peeraddr - The remote client's address.
 * @param[in] message - The inbound SrvDereg message.
 * @param[out] sendbuf - The outbound reply buffer.
 * @param[in] errorcode - The inbound request error code.
 *
 * @return Non-zero if message should be silently dropped.
 */
static int v1ProcessSrvDeReg(struct sockaddr_storage * peeraddr, 
      SLPMessage * message, SLPBuffer * sendbuf, int errorcode)
{
   SLPBuffer result = *sendbuf;

   (void)peeraddr;

   /* If errorcode is set, we can not be sure that message is good
      Go directly to send response code  also do not process mcast
      srvreg or srvdereg messages */

   if (errorcode || message->header.flags & SLP_FLAG_MCAST)
      goto RESPOND;

   /* make sure that we handle the scope */
   if (SLPIntersectStringList(message->body.srvdereg.scopelistlen, 
         message->body.srvdereg.scopelist, G_SlpdProperty.useScopesLen,
         G_SlpdProperty.useScopes))
      errorcode = SLPDDatabaseDeReg(message);
   else
      errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;

RESPOND:

   /* don't do anything multicast SrvDeReg (set result empty) */
   if (message->header.flags & SLP_FLAG_MCAST)
   {
      result->end = result->start;
      goto FINISHED;
   }

   /* ensure the buffer is big enough to handle the whole srvack */
   result = SLPBufferRealloc(result, 14);
   if (result == 0)
   {
      errorcode = SLP_ERROR_INTERNAL_ERROR;
      goto FINISHED;
   }

   /* add the header */

   /* version */
   *result->curpos++ = 1;

   /* function id */
   *result->curpos++ = SLP_FUNCT_SRVACK;

   /* length */
   PutUINT16(&result->curpos, 14);

   /* flags */
   /** @todo Set the flags correctly. */
   *result->curpos++ = 0;

   /* dialect */
   *result->curpos++ = 0;

   /* language code */
   memcpy(result->curpos, message->header.langtag, 2);
   result->curpos += 2;
   PutUINT16(&result->curpos, message->header.encoding);

   /*xid*/
   PutUINT16(&result->curpos, message->header.xid);

   /* add the errorcode */
   PutUINT16(&result->curpos, errorcode);

FINISHED:

   *sendbuf = result;
   return errorcode;
}

/** Process an SLPv1 Attribute Request message.
 *
 * @param[in] peeraddr - The remote client's address.
 * @param[in] message - The inbound request message.
 * @param[out] sendbuf - The outbound response buffer.
 * @param[in] errorcode - The inbound request error code.
 *
 * @return Zero on success, or a non-zero value on failure.
 */
static int v1ProcessAttrRqst(struct sockaddr_storage * peeraddr, 
      SLPMessage * message, SLPBuffer * sendbuf, int errorcode)
{
   SLPDDatabaseAttrRqstResult * db = 0;
   size_t attrlen = 0;
   size_t size = 0;
   SLPBuffer result = *sendbuf;

   (void)peeraddr;

   /* If errorcode is set, we can not be sure that message is good
      Go directly to send response code */
   if (errorcode)
      goto RESPOND;

   /* check for one of our IP addresses in the prlist */
   if (SLPIntersectStringList(message->body.attrrqst.prlistlen, 
         message->body.attrrqst.prlist, G_SlpdProperty.interfacesLen,
         G_SlpdProperty.interfaces))
   {
      result->end = result->start;
      goto FINISHED;
   }

   /* make sure that we handle the scope */
   if (SLPIntersectStringList(message->body.attrrqst.scopelistlen,
         message->body.attrrqst.scopelist, G_SlpdProperty.useScopesLen,
         G_SlpdProperty.useScopes))
      errorcode = SLPDDatabaseAttrRqstStart(message,&db);
   else
      errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;

RESPOND:

   /* do not send error codes or empty replies to multicast requests */
   if (message->header.flags & SLP_FLAG_MCAST)
      if (errorcode != 0 || db->attrlistlen == 0)
      {
         result->end = result->start;
         goto FINISHED;  
      }

   /* ensure the buffer is big enough to handle the whole attrrply */
   size = 16; /* 12 bytes for header, 2 bytes for error code, 2 bytes
   for attr-list len */
   if (errorcode == 0)
   {
      attrlen = INT_MAX;
      errorcode = SLPv1ToEncoding(0, &attrlen, message->header.encoding, 
            db->attrlist, db->attrlistlen);
      size += attrlen;
   }

   /* alloc the buffer */
   result = SLPBufferRealloc(result, size);
   if (result == 0)
   {
      errorcode = SLP_ERROR_INTERNAL_ERROR;
      goto FINISHED;
   }

   /* Add the header */

   /* version */
   *result->curpos++ = 1;

   /* function id */
   *result->curpos++ = SLP_FUNCT_ATTRRPLY;

   /* length */
   PutUINT16(&result->curpos, size);

   /* flags */
   /** @todo Set the flags correctly. */
   *result->curpos++ = (uint8_t)(message->header.flags 
         | (size > SLP_MAX_DATAGRAM_SIZE? 
               SLPv1_FLAG_OVERFLOW: 0));  

   /* dialect */
   *result->curpos++ = 0;

   /* language code */
   memcpy(result->curpos, message->header.langtag, 2);
   result->curpos += 2;
   PutUINT16(&result->curpos, message->header.encoding);

   /* xid */
   PutUINT16(&result->curpos, message->header.xid);

   /* add rest of the AttrRply */

   /* error code */

   PutUINT16(&result->curpos, errorcode);
   if (errorcode == 0)
   {
      /* attr-list len */
      PutUINT16(&result->curpos, attrlen);
      attrlen = size;
      SLPv1ToEncoding((char *)result->curpos, &attrlen, 
            message->header.encoding, db->attrlist, db->attrlistlen);
      result->curpos += attrlen; 
   }

FINISHED:

   *sendbuf = result;
   if (db) 
      SLPDDatabaseAttrRqstEnd(db);
   return errorcode;
}        

/** Process an SLPv1 Service Type request message.
 *
 * @param[in] peeraddr - The remote client's address.
 * @param[in] message - The inbound request message.
 * @param[out] sendbuf - The outbound response buffer.
 * @param[in] errorcode - The inbound request error code.
 *
 * @return Zero on success, or a non-zero value on failure.
 */
static int v1ProcessSrvTypeRqst(struct sockaddr_storage * peeraddr, 
      SLPMessage * message, SLPBuffer * sendbuf, int errorcode)
{
   char * type;
   char * end;
   char * slider;
   int i;
   size_t typelen;
   size_t size = 0;
   int numsrvtypes = 0;
   SLPDDatabaseSrvTypeRqstResult * db = 0;
   SLPBuffer result = *sendbuf;

   (void)peeraddr;

   /* check for one of our IP addresses in the prlist */
   if (SLPIntersectStringList(message->body.srvtyperqst.prlistlen, 
         message->body.srvtyperqst.prlist, G_SlpdProperty.interfacesLen,
         G_SlpdProperty.interfaces))
   {
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

   /* do not send error codes or empty replies to multicast requests */
   if (message->header.flags & SLP_FLAG_MCAST)
      if (errorcode != 0 || db->srvtypelistlen == 0)
      {
         result->end = result->start;
         goto FINISHED;  
      }

   /* ensure the buffer is big enough to handle the whole srvtyperply */
   size = 16;  /* 12 bytes for header, 2 bytes for error code, 2 bytes
                  for num of service types */
   if (errorcode == 0)
   {
      if (db->srvtypelistlen)
      {
         /* there has to be at least one service type*/
         numsrvtypes = 1;

         /* count the rest of the service types */
         type = db->srvtypelist;
         for (i = 0; i < (int)db->srvtypelistlen; i++)
            if (type[i] == ',')
               numsrvtypes += 1;

         /* figure out how much memory is required for srvtype strings */
         typelen = INT_MAX;
         errorcode = SLPv1ToEncoding(0, &typelen, message->header.encoding,
               db->srvtypelist, db->srvtypelistlen);

         /* TRICKY: We add in the numofsrvtypes + 1 to make room for the
          * type length.  We can do this because the ',' of the comma
          * delimited list is one byte. 
          */
         size = size + typelen + numsrvtypes + 1;
      }
      else
         numsrvtypes = 0;
   }

   /* allocate memory */
   result = SLPBufferRealloc(result,size);
   if (result == 0)
   {
      errorcode = SLP_ERROR_INTERNAL_ERROR;
      goto FINISHED;
   }

   /* Add the header */

   /* version */
   *result->curpos++ = 1;

   /* function id */
   *result->curpos++ = SLP_FUNCT_SRVTYPERPLY;

   /* length */
   PutUINT16(&result->curpos, size);

   /* flags */
   /** @todo Set the flags correctly. */
   *result->curpos++ = (uint8_t)(message->header.flags 
         | (size > SLP_MAX_DATAGRAM_SIZE?
               SLPv1_FLAG_OVERFLOW: 0));

   /* dialect */
   *result->curpos++ = 0;

   /* language code */
   memcpy(result->curpos, message->header.langtag, 2);
   result->curpos += 2;

   PutUINT16(&result->curpos, message->header.encoding);

   /* xid */
   PutUINT16(&result->curpos, message->header.xid);

   /* Add rest of the SrvTypeRply */

   /* error code */
   PutUINT16(&result->curpos, errorcode);
   if (errorcode == 0)
   {
      /* num of service types */
      PutUINT16(&result->curpos, numsrvtypes);

      /* service type strings */
      type = db->srvtypelist;
      slider = db->srvtypelist;
      end = &type[db->srvtypelistlen];
      for (i = 0; i < numsrvtypes; i++)
      {
         while (slider < end && *slider != ',') 
            slider++;

         /* put in the encoded service type */
         typelen = size;
         SLPv1ToEncoding((char *)(result->curpos + 2), &typelen, 
               message->header.encoding, type, slider - type);

         /* slip in the typelen */
         PutUINT16(&result->curpos, typelen);
         result->curpos += typelen;

         slider++; /* skip comma */
         type = slider;
      }
      /** @todo Make sure we don't return generic types */
   }

FINISHED:   

   if (db) 
      SLPDDatabaseSrvTypeRqstEnd(db);

   *sendbuf = result;

   return errorcode;
}

/** Process an SLPv1 message.
 *
 * Process an SLPv1 message, and place the results in an outbound
 * buffer object.
 *
 * @param[in] peeraddr - The remote client's address.
 * @param[in] recvbuf  - The inbound message to process.
 * @param[out] sendbuf - The outbound response buffer.
 *
 * @return Zero on success, or SLP_ERROR_PARSE_ERROR on general failure, 
 *    or SLP_ERROR_INTERNAL_ERROR under out-of-memory conditions.
 */
int SLPDv1ProcessMessage(struct sockaddr_storage * peeraddr, 
      SLPBuffer recvbuf, SLPBuffer * sendbuf)
{
   SLPHeader header;
   SLPMessage * message;
   int errorcode = 0;

   /* SLPv1 messages are handled only by DAs */
   if (!G_SlpdProperty.isDA)
      errorcode = SLP_ERROR_VER_NOT_SUPPORTED;

   /* Parse just the message header the reset the buffer "curpos" pointer */
   recvbuf->curpos = recvbuf->start;
   errorcode = SLPv1MessageParseHeader(recvbuf, &header);

   /* Reset the buffer "curpos" pointer so that full message can be 
      parsed later 
    */
   recvbuf->curpos = recvbuf->start;


   /* TRICKY: Duplicate SRVREG recvbufs *before* parsing them it because we 
    * are going to keep them in the 
    */
   if (header.functionid == SLP_FUNCT_SRVREG)
   {
      recvbuf = SLPBufferDup(recvbuf);
      if (recvbuf == 0)
         return SLP_ERROR_INTERNAL_ERROR;
   }

   /* Allocate the message descriptor */
   message = SLPMessageAlloc();
   if (message)
   {
      /* Copy in the remote address */
      memcpy(&message->peer, peeraddr, sizeof(message->peer));

      /* Parse the message and fill out the message descriptor */
      errorcode = SLPv1MessageParseBuffer(recvbuf, message);
      if (errorcode == 0)
      {
         /* Process messages based on type */
         switch (message->header.functionid)
         {
            case SLP_FUNCT_SRVRQST:
               errorcode = v1ProcessSrvRqst(peeraddr, peeraddr, message, 
                     sendbuf, errorcode);
               break;

            case SLP_FUNCT_SRVREG:
               errorcode = v1ProcessSrvReg(peeraddr, message, recvbuf, 
                     sendbuf, errorcode);
               if (errorcode == 0)
                  SLPDKnownDAEcho(message, recvbuf);      
               break;

            case SLP_FUNCT_SRVDEREG:
               errorcode = v1ProcessSrvDeReg(peeraddr, message, 
                     sendbuf, errorcode);
               if (errorcode == 0)
                  SLPDKnownDAEcho(message, recvbuf);      
               break;

            case SLP_FUNCT_ATTRRQST:
               errorcode = v1ProcessAttrRqst(peeraddr, message, 
                     sendbuf, errorcode);
               break;

            case SLP_FUNCT_SRVTYPERQST:
               errorcode = v1ProcessSrvTypeRqst(peeraddr, message, 
                     sendbuf, errorcode);
               break;

            case SLP_FUNCT_DAADVERT:
               /* we are a SLPv2 DA, ignore other v1 DAs */
               (*sendbuf)->end = (*sendbuf)->start;
               break;

            default:
               /* Should never happen... but we're paranoid */
               errorcode = SLP_ERROR_PARSE_ERROR;
               break;
         }   
      }

      if (header.functionid == SLP_FUNCT_SRVREG)
      {
         /* TRICKY: Do not free the message descriptor for SRVREGs because we 
          * are keeping them in the database unless there is an error then we 
          * free memory.
          */
         if (errorcode)
         {
            SLPMessageFree(message);
            SLPBufferFree(recvbuf);
         }
      }
      else
         SLPMessageFree(message);
   }
   else
      errorcode = SLP_ERROR_INTERNAL_ERROR;

   return errorcode;
}                

/*=========================================================================*/
