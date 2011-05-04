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

/** Logging functions.
 *
 * @file       slpd_log.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#include "slpd_log.h"
#include "slpd_property.h"

#include "slp_net.h"

/** The internal log file object.
 *
 * @internal
 */
static FILE * G_SlpdLogFile = 0;

/** Prepares the file at the specified path as the log file.
 *
 * @param[in] path - The path to the log file. If path is the empty
 *    string (""), then we log to stdout.
 * @param[in] append - If zero log file will be truncated.
 *
 * @return Zero on success, or a non-zero value on failure with errno set.
 */
int SLPDLogFileOpen(const char * path, int append)
{
   SLPDLogFileClose(); /* close logfile if it was already open */

   if (*path == 0)
      G_SlpdLogFile = stdout; /* Log to console. */
   else
   {
      /* Log to file. */
#ifndef _WIN32
      /* only owner can read/write */
      umask(0077); 
#endif        
      if (append)
         G_SlpdLogFile = fopen(path,"a");
      else
         G_SlpdLogFile = fopen(path,"w");

      if (G_SlpdLogFile == 0)
      {
         G_SlpdLogFile = stdout;
         SLPDLog("Could not open log file %s: %s\n", path, strerror(errno));
         return -1;  /* could not open the log file */
      }
   }
   return 0;
}

/** Releases resources associated with the log file.
 *
 * @return Zero - always.
 */
int SLPDLogFileClose(void)
{
   if (G_SlpdLogFile)
   {
      if (G_SlpdLogFile != stdout)
         fclose(G_SlpdLogFile);
      G_SlpdLogFile = 0;
   }
   return 0;
}

/** Logs a message.
 *
 * @param[in] msg - The message to log.
 * 
 * @note This is a variadic function. The number and types of actual 
 *    parameters is determined by the string contents of @p msg.
 */
void SLPDLog(const char * msg, ... )
{
   va_list ap;

   if (G_SlpdLogFile)
   {
      va_start(ap, msg);
      vfprintf(G_SlpdLogFile, msg, ap); 
      va_end(ap);
      fflush(G_SlpdLogFile);
   }
}

/** Logs a message and halts the process.
 *
 * @param[in] msg - The message to log.
 * 
 * @note This is a variadic function. The number and types of actual 
 *    parameters is determined by the string contents of @p msg.
 */
void SLPDFatal(const char * msg, ... )
{
   va_list ap;

   if (G_SlpdLogFile)
   {
      fprintf(G_SlpdLogFile, "A FATAL Error has occured:\n");
      va_start(ap, msg);
      vfprintf(G_SlpdLogFile, msg, ap);
      va_end(ap);
      fflush(G_SlpdLogFile);
   }
   else
   {
      fprintf(stderr, "A FATAL Error has occured:\n");
      va_start(ap, msg);
      vprintf(msg, ap);
      va_end(ap);
   }
   exit(1);
}

/** Writes a buffer to the log file.
 *
 * @param[in] prefix - The prefix string to write before @p buf.
 * @param[in] bufsize - The size of @p buf in bytes.
 * @param[in] buf - A pointer to the buffer to write.
 */
void SLPDLogBuffer(const char * prefix, size_t bufsize, const char * buf)
{
   if (G_SlpdLogFile)
   {
      fprintf(G_SlpdLogFile, "%s", prefix);
      if (buf == NULL || bufsize < 0)
      {
         fprintf(G_SlpdLogFile,"<invalid data!>");
      }
      else if (bufsize == 0)
      {
         fprintf(G_SlpdLogFile,"<empty>");
      }
      else
      {
         fwrite(buf,bufsize,1,G_SlpdLogFile);
      }
      fprintf(G_SlpdLogFile, "\n");
      fflush(G_SlpdLogFile);
   }
}

/** Logs the current time and date to the log file.
 */
void SLPDLogTime(void)
{
   time_t curtime = time(0);
   SLPDLog("%s", ctime(&curtime)); 
}

/** Logs information about a SrvRequest message to the log file.
 *
 * @param[in] srvrqst - The message to log.
 *
 * @internal
 */
static void SLPDLogSrvRqstMessage(SLPSrvRqst * srvrqst)
{
   SLPDLog("Message SRVRQST:\n");
   SLPDLogBuffer("   srvtype = ", srvrqst->srvtypelen, srvrqst->srvtype);
   SLPDLogBuffer("   scopelist = ", srvrqst->scopelistlen, srvrqst->scopelist);
   SLPDLogBuffer("   predicate = ", srvrqst->predicatelen, srvrqst->predicate);
}

/** Logs information about a SrvReply message to the log file.
 *
 * @param[in] srvrply - The message to log.
 *
 * @internal
 */
static void SLPDLogSrvRplyMessage(SLPSrvRply * srvrply)
{
   SLPDLog("Message SRVRPLY:\n");
   SLPDLog("   errorcode = %i\n",srvrply->errorcode);
}

/** Logs information about a SrvReg message to the log file.
 *
 * @param[in] srvreg - The message to log.
 *
 * @internal
 */
static void SLPDLogSrvRegMessage(SLPSrvReg * srvreg)
{
   SLPDLog("Message SRVREG:\n");
   SLPDLogBuffer("   srvtype = ", srvreg->srvtypelen, srvreg->srvtype);
   SLPDLogBuffer("   scope = ", srvreg->scopelistlen, srvreg->scopelist);
   SLPDLogBuffer("   url = ", srvreg->urlentry.urllen, srvreg->urlentry.url);
   SLPDLogBuffer("   attributes = ", srvreg->attrlistlen, srvreg->attrlist);
}

/** Logs information about a SrvDereg message to the log file.
 *
 * @param[in] srvdereg - The message to log.
 *
 * @internal
 */
static void SLPDLogSrvDeRegMessage(SLPSrvDeReg * srvdereg)
{
   (void)srvdereg;
   SLPDLog("Message SRVDEREG:\n");
}

/** Logs information about a SrvAck message to the log file.
 *
 * @param[in] srvack - The message to log.
 *
 * @internal
 */
static void SLPDLogSrvAckMessage(SLPSrvAck * srvack)
{
   SLPDLog("Message SRVACK:\n");
   SLPDLog("   errorcode = %i\n",srvack->errorcode);
}

/** Logs information about an AttrReq message to the log file.
 *
 * @param[in] attrrqst - The message to log.
 *
 * @internal
 */
static void SLPDLogAttrRqstMessage(SLPAttrRqst * attrrqst)
{
   (void)attrrqst;
   SLPDLog("Message ATTRRQST:\n");
}

/** Logs information about an AttrReply message to the log file.
 *
 * @param[in] attrrply - The message to log.
 *
 * @internal
 */
static void SLPDLogAttrRplyMessage(SLPAttrRply * attrrply)
{
   SLPDLog("Message ATTRRPLY:\n");
   SLPDLog("   errorcode = %i\n", attrrply->errorcode);
} 

/** Logs information about a DAAdvert message to the log file.
 *
 * @param[in] daadvert - The message to log.
 *
 * @internal
 */
static void SLPDLogDAAdvertMessage(SLPDAAdvert * daadvert)
{
   SLPDLog("Message DAADVERT:\n");
   SLPDLogBuffer("   scope = ", daadvert->scopelistlen, daadvert->scopelist);
   SLPDLogBuffer("   url = ", daadvert->urllen, daadvert->url);
   SLPDLogBuffer("   attributes = ", daadvert->attrlistlen, daadvert->attrlist);
}

/** Logs information about a SrvTypeReq message to the log file.
 *
 * @param[in] srvtyperqst - The message to log.
 *
 * @internal
 */
static void SLPDLogSrvTypeRqstMessage(SLPSrvTypeRqst * srvtyperqst)
{
   (void)srvtyperqst;
   SLPDLog("Message SRVTYPERQST:\n");
}

/** Logs information about a SrvTypeReply message to the log file.
 *
 * @param[in] srvtyperply - The message to log.
 *
 * @internal
 */
static void SLPDLogSrvTypeRplyMessage(SLPSrvTypeRply * srvtyperply)
{
   SLPDLog("Message SRVTYPERPLY:\n");
   SLPDLog("   errorcode = %i\n", srvtyperply->errorcode);
}        

/** Logs information about an SAAdvert message to the log file.
 *
 * @param[in] saadvert - The message to log.
 *
 * @internal
 */
static void SLPDLogSAAdvertMessage(SLPSAAdvert * saadvert)
{
   (void)saadvert;
   SLPDLog("Message SAADVERT:\n");
}

/** Logs a peer address to the log file.
 *
 * @param[in] peeraddr - The address to write.
 *
 * @internal
 */
static void SLPDLogPeerAddr(struct sockaddr_storage * peeraddr)
{
   char addr_str[INET6_ADDRSTRLEN];

   SLPDLog("Peer IP address: %s\n", 
         SLPNetSockAddrStorageToString(peeraddr, 
               addr_str, sizeof(addr_str)));
}

/** Logs common message information to the log file.
 *
 * @param[in] message - The message to log.
 */
void SLPDLogMessageInternals(SLPMessage * message)
{
   char addr_str[INET6_ADDRSTRLEN];

   SLPDLog("Peer: \n");
   SLPDLog("   IP address: %s\n", SLPNetSockAddrStorageToString(
         &message->peer, addr_str, sizeof(addr_str)));
   SLPDLog("Header:\n");
   SLPDLog("   version = %i\n", message->header.version);
   SLPDLog("   functionid = %i\n", message->header.functionid);
   SLPDLog("   length = %i\n", message->header.length);
   SLPDLog("   flags = %i\n", message->header.flags);
   SLPDLog("   extoffset = %i\n", message->header.extoffset);
   SLPDLog("   xid = %i\n", message->header.xid);
   SLPDLogBuffer("   langtag = ", 
         message->header.langtaglen, message->header.langtag); 

   switch (message->header.functionid)
   {
      case SLP_FUNCT_SRVRQST:
         SLPDLogSrvRqstMessage(&message->body.srvrqst);
         break;

      case SLP_FUNCT_SRVRPLY:
         SLPDLogSrvRplyMessage(&message->body.srvrply);
         break;

      case SLP_FUNCT_SRVREG:
         SLPDLogSrvRegMessage(&message->body.srvreg);
         break;

      case SLP_FUNCT_SRVDEREG:
         SLPDLogSrvDeRegMessage(&message->body.srvdereg);
         break;

      case SLP_FUNCT_SRVACK:
         SLPDLogSrvAckMessage(&message->body.srvack);
         break;

      case SLP_FUNCT_ATTRRQST:
         SLPDLogAttrRqstMessage(&message->body.attrrqst);
         break;

      case SLP_FUNCT_ATTRRPLY:
         SLPDLogAttrRplyMessage(&message->body.attrrply);
         break;

      case SLP_FUNCT_DAADVERT:
         SLPDLogDAAdvertMessage(&message->body.daadvert);
         break;

      case SLP_FUNCT_SRVTYPERQST:
         SLPDLogSrvTypeRqstMessage(&message->body.srvtyperqst);
         break;

      case SLP_FUNCT_SRVTYPERPLY:
         SLPDLogSrvTypeRplyMessage(&message->body.srvtyperply);
         break;

      case SLP_FUNCT_SAADVERT:
         SLPDLogSAAdvertMessage(&message->body.saadvert);
         break;

      default:
         SLPDLog("Message %i UNKNOWN:\n", message->header.functionid);
         SLPDLog("   This is really bad\n");
         break;
   }
}

/** Log record of receiving or sending an SLP message.
 *
 * @param[in] msglogflags - The type of message to log.
 * @param[in] peerinfo - The source or destination peer.
 * @param[in] localaddr - The locally bound address.
 * @param[in] buf - The message data buffer to be logged.
 *
 * @note Logging will only occur if message logging is enabled.
 */
void SLPDLogMessage(int msglogflags, struct sockaddr_storage * peerinfo,
      struct sockaddr_storage * localaddr, SLPBuffer buf)
{
   SLPMessage * msg;
   char addr_str[INET6_ADDRSTRLEN];

   if (peerinfo == 0 || buf == 0)
      return;

   if ((G_SlpdProperty.traceMsg && (msglogflags & SLPDLOG_TRACEMSG)) 
         || (G_SlpdProperty.traceDrop && (msglogflags & SLPDLOG_TRACEDROP)))
   {
      /* Don't log localhost traffic since it is probably IPC
         and don't log empty messages
       */
      if (!SLPNetIsLocal(peerinfo) && buf->end != buf->start)
      {
         msg = SLPMessageAlloc();
         if (msg)
         {
            SLPDLogTime();
            SLPDLog("MESSAGE - ");
            if (msglogflags == SLPDLOG_TRACEMSG_OUT)
               SLPDLog("Trace message (OUT)\n");
            else if (msglogflags == SLPDLOG_TRACEMSG_IN)
               SLPDLog("Trace message (IN)\n");
            else if (msglogflags == SLPDLOG_TRACEDROP)
               SLPDLog("Dropped message (following message silently ignored)\n");
            else
               SLPDLog("\n");

            if (SLPMessageParseBuffer(peerinfo, localaddr, buf, msg) == 0)
               SLPDLogMessageInternals(msg);
            else
            {
               SLPDLog("Message parsing failed\n");
               SLPDLog("Peer: \n");
               SLPDLog("   IP address: %s\n", SLPNetSockAddrStorageToString(
                     &msg->peer, addr_str, sizeof(addr_str)));
            }
            SLPMessageFree(msg);
            SLPDLog("\n");
         }
      }
   }
}

/** Log record of having added a registration to the database.  
 *
 * @param[in] prefix - An informative prefix for the log entry.
 * @param[in] entry - The database entry that was affected.
 *
 * @note Logging of registraions will only occur if registration trace 
 *    is enabled.
 */
void SLPDLogRegistration(const char * prefix, SLPDatabaseEntry * entry)
{
   char addr_str[INET6_ADDRSTRLEN];

   if (prefix == 0 || entry == 0)
      return;

   if (G_SlpdProperty.traceReg)
   {
      SLPDLogTime();
      SLPDLog("DATABASE - %s:\n", prefix);
      SLPDLog("    SA address = ");
      switch (entry->msg->body.srvreg.source)
      {
         case SLP_REG_SOURCE_UNKNOWN:
            SLPDLog("<unknown>\n");
            break;

         case SLP_REG_SOURCE_REMOTE:
            SLPDLog("remote (%s)\n", 
                  SLPNetSockAddrStorageToString(&entry->msg->peer, 
                        addr_str, sizeof(addr_str)));
            break;

         case SLP_REG_SOURCE_LOCAL:
            SLPDLog("IPC (libslp)\n");
            break;

         case SLP_REG_SOURCE_STATIC:
            SLPDLog("static (slp.reg)\n");
            break;
      }
      SLPDLogBuffer("    service-url = ",
            entry->msg->body.srvreg.urlentry.urllen,
            entry->msg->body.srvreg.urlentry.url);
      SLPDLogBuffer("    scope = ",
            entry->msg->body.srvreg.scopelistlen,
            entry->msg->body.srvreg.scopelist);
      SLPDLogBuffer("    attributes = ",
            entry->msg->body.srvreg.attrlistlen,
            entry->msg->body.srvreg.attrlist);
      SLPDLog("\n");
   }
}

/** Log record of addition or removal of a DA to the store of known DAs.
 *
 * @param[in] prefix - An informative prefix for the log entry.
 * @param[in] entry - The database entry that was affected.
 *
 * @note Logging of DA modifications will only occur if DA Advertisment 
 *    message logging is enabled.
 */
void SLPDLogDAAdvertisement(const char * prefix, SLPDatabaseEntry * entry)
{
   char addr_str[INET6_ADDRSTRLEN];

   if (prefix == 0 || entry == 0)
      return;

   if (G_SlpdProperty.traceDATraffic)
   {
      SLPDLogTime();
      SLPDLog("KNOWNDA - %s:\n",prefix);
      SLPDLog("    DA address = %s\n", SLPNetSockAddrStorageToString(
            &entry->msg->peer, addr_str, sizeof(addr_str)));
      SLPDLogBuffer("    directory-agent-url = ",
            entry->msg->body.daadvert.urllen,
            entry->msg->body.daadvert.url);
      SLPDLog("    bootstamp = %x\n", 
            entry->msg->body.daadvert.bootstamp);
      SLPDLogBuffer("    scope = ",
            entry->msg->body.daadvert.scopelistlen,
            entry->msg->body.daadvert.scopelist);
      SLPDLogBuffer("    attributes = ",
            entry->msg->body.daadvert.attrlistlen,
            entry->msg->body.daadvert.attrlist);
#ifdef ENABLE_SLPV2_SECURITY
      SLPDLogBuffer("    SPI list = ",
            entry->msg->body.daadvert.spilistlen,
            entry->msg->body.daadvert.spilist);
#endif /*ENABLE_SLPV2_SECURITY*/
      SLPDLog("\n");
   }
}

/** Log a parse warning and dump the invalid message.
 *
 * @param[in] peeraddr - The remote address of the message sender.
 * @param[in] buf - The message buffer with the parse problem.
 */
void SLPDLogParseWarning(struct sockaddr_storage * peeraddr, SLPBuffer buf)
{
   unsigned char * curpos;
   int i = 0;

   if (peeraddr == 0 || buf == 0)
      return;

   SLPDLogTime();
   SLPDLog("*** WARNING Parse Error ***\n");
   SLPDLogPeerAddr(peeraddr);
   SLPDLog("message size = %i\n",buf->end - buf->start);
   SLPDLog("message dump follows:\n");    
   for (curpos = buf->start; curpos < buf->end; curpos++)
   {
      SLPDLog("0x%02x", *curpos);
      if (*curpos < 0x20 || *curpos > 0x7f)
         SLPDLog("(' ') ");
      else
         SLPDLog("('%c') ",*curpos);

      /* newline every 70 columns */
      i++;
      if (i == 10)
      {
         i = 0;
         SLPDLog("\n");
      }
   }    
   SLPDLog("\n");
}

/*=========================================================================*/
