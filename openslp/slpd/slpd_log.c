/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_log.c                                                 */
/*                                                                         */
/* Abstract:    slpd logging functions                                     */
/*                                                                         */
/* WARNING:     NOT thread safe!                                           */
/*                                                                         */
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

/*=========================================================================*/
/* slpd includes                                                           */
/*=========================================================================*/
#include "slpd_log.h"
#include "slpd_property.h"

#include <time.h>

/********************************************/
/* TODO: Make these functions thread safe!! */
/********************************************/

/*=========================================================================*/
static FILE*   G_SlpdLogFile    = 0;
/*=========================================================================*/


/*=========================================================================*/
int SLPDLogFileOpen(const char* path, int append)                           
/* Prepares the file at the specified path as the log file.                */
/*                                                                         */
/* path     - (IN) the path to the log file. If path is the empty string   */
/*            (""), then we log to stdout.                                 */
/*                                                                         */
/* append   - (IN) if zero log file will be truncated.                     */
/*                                                                         */
/* Returns  - zero on success. errno on failure.                           */
/*=========================================================================*/
{
    if(G_SlpdLogFile)
    {
        /* logfile was already open close it */
        fclose(G_SlpdLogFile);
    }

    if(*path == 0)
    {
        /* Log to console. */
        G_SlpdLogFile = stdout;
    }
    else
    {
        /* Log to file. */
#ifndef WIN32        
        /* only owner can read/write */
        umask(0077); 
#endif        
        if(append)
        {
            G_SlpdLogFile = fopen(path,"a");
        }
        else
        {
            G_SlpdLogFile = fopen(path,"w");
        }

        if(G_SlpdLogFile == 0)
        {
            /* could not open the log file */
            return -1;
        }
    }

    return 0;
}

#ifdef DEBUG
/*=========================================================================*/
int SLPDLogFileClose()
/* Releases resources associated with the log file                         */
/*=========================================================================*/
{
    fclose(G_SlpdLogFile);

    return 0;
}
#endif


/*=========================================================================*/
void SLPDLog(const char* msg, ...)
/* Logs a message                                                          */
/*=========================================================================*/
{
    va_list ap;

    if(G_SlpdLogFile)
    {
        va_start(ap,msg);
        vfprintf(G_SlpdLogFile,msg,ap); 
        va_end(ap);
        fflush(G_SlpdLogFile);
    }
}

/*=========================================================================*/
void SLPDFatal(const char* msg, ...)
/* Logs a message and halts the process                                    */
/*=========================================================================*/
{
    va_list ap;

    if(G_SlpdLogFile)
    {
        fprintf(G_SlpdLogFile,"A FATAL Error has occured:\n");
        va_start(ap,msg);
        vfprintf(G_SlpdLogFile,msg,ap);
        va_end(ap);
        fflush(G_SlpdLogFile);
    }
    else
    {
        fprintf(stderr,"A FATAL Error has occured:\n");
        va_start(ap,msg);
        vprintf(msg,ap);
        va_end(ap);
    }

    exit(1);
}

/*=========================================================================*/
void SLPDLogBuffer(const char* prefix, int bufsize, const char* buf)
/* Writes a buffer to the logfile                                          */
/*=========================================================================*/
{
    if(G_SlpdLogFile)
    {
        fprintf(G_SlpdLogFile,"%s",prefix);
        fwrite(buf,bufsize,1,G_SlpdLogFile);
        fprintf(G_SlpdLogFile,"\n");
        fflush(G_SlpdLogFile);
    }
}



/*=========================================================================*/
void SLPDLogTime()
/* Logs a timestamp                                                        */
/*=========================================================================*/
{
    time_t curtime = time(NULL);
    SLPDLog("%s",ctime(&curtime)); 
}


/*-------------------------------------------------------------------------*/
void SLPDLogSrvRqstMessage(SLPSrvRqst* srvrqst)
/*-------------------------------------------------------------------------*/
{
    SLPDLog("Message SRVRQST:\n");
    SLPDLogBuffer("   srvtype = ", srvrqst->srvtypelen, srvrqst->srvtype);
    SLPDLogBuffer("   scopelist = ", srvrqst->scopelistlen, srvrqst->scopelist);
    SLPDLogBuffer("   predicate = ", srvrqst->predicatelen, srvrqst->predicate);
}

/*-------------------------------------------------------------------------*/
void SLPDLogSrvRplyMessage(SLPSrvRply* srvrply)
/*-------------------------------------------------------------------------*/
{
    SLPDLog("Message SRVRPLY:\n");
    SLPDLog("   errorcode = %i\n",srvrply->errorcode);
}

/*-------------------------------------------------------------------------*/
void SLPDLogSrvRegMessage(SLPSrvReg* srvreg)
/*-------------------------------------------------------------------------*/
{
    SLPDLog("Message SRVREG:\n");
    SLPDLog("   source type = ");
    switch( srvreg->source)
    {
    case SLP_REG_SOURCE_REMOTE:
        SLPDLog("REMOTE\n");
        break;
    case SLP_REG_SOURCE_LOCAL:
        SLPDLog("LOCAL\n");
        break;
    case SLP_REG_SOURCE_STATIC:
        SLPDLog("STATIC\n");
        break;         
    default:
        SLPDLog("UNKNOWN\n");
        break;         
    }
    SLPDLogBuffer("   srvtype = ", srvreg->srvtypelen, srvreg->srvtype);
    SLPDLogBuffer("   scope = ", srvreg->scopelistlen, srvreg->scopelist);
    SLPDLogBuffer("   url = ", srvreg->urlentry.urllen, srvreg->urlentry.url);
    SLPDLogBuffer("   attributes = ", srvreg->attrlistlen, srvreg->attrlist);
}

/*-------------------------------------------------------------------------*/
void SLPDLogSrvDeRegMessage(SLPSrvDeReg* srvdereg)
/*-------------------------------------------------------------------------*/
{
    SLPDLog("Message SRVDEREG:\n");
}

/*-------------------------------------------------------------------------*/
void SLPDLogSrvAckMessage(SLPSrvAck* srvack)
/*-------------------------------------------------------------------------*/
{
    SLPDLog("Message SRVACK:\n");
    SLPDLog("   errorcode = %i\n",srvack->errorcode);
}

/*-------------------------------------------------------------------------*/
void SLPDLogAttrRqstMessage(SLPAttrRqst* attrrqst)
/*-------------------------------------------------------------------------*/
{
    SLPDLog("Message ATTRRQST:\n");
}

/*-------------------------------------------------------------------------*/
void SLPDLogAttrRplyMessage(SLPAttrRply* attrrply)
/*-------------------------------------------------------------------------*/
{
    SLPDLog("Message ATTRRPLY:\n");
    SLPDLog("   errorcode = %i\n",attrrply->errorcode);
} 

/*-------------------------------------------------------------------------*/
void SLPDLogDAAdvertMessage(SLPDAAdvert* daadvert)
/*-------------------------------------------------------------------------*/
{
    SLPDLog("Message DAADVERT:\n");
    SLPDLogBuffer("   scope = ", daadvert->scopelistlen, daadvert->scopelist);
    SLPDLogBuffer("   url = ", daadvert->urllen, daadvert->url);
    SLPDLogBuffer("   attributes = ", daadvert->attrlistlen, daadvert->attrlist);
}

/*-------------------------------------------------------------------------*/
void SLPDLogSrvTypeRqstMessage(SLPSrvTypeRqst* srvtyperqst)
/*-------------------------------------------------------------------------*/
{
    SLPDLog("Message SRVTYPERQST:\n");
}

/*-------------------------------------------------------------------------*/
void SLPDLogSrvTypeRplyMessage(SLPSrvTypeRply* srvtyperply)
/*-------------------------------------------------------------------------*/ 
{
    SLPDLog("Message SRVTYPERPLY:\n");
    SLPDLog("   errorcode = %i\n",srvtyperply->errorcode);
}        

/*-------------------------------------------------------------------------*/
void SLPDLogSAAdvertMessage(SLPSAAdvert* saadvert)
/*-------------------------------------------------------------------------*/
{
    SLPDLog("Message SAADVERT:\n");
}


/*-------------------------------------------------------------------------*/
void SLPDLogPeerAddr(struct sockaddr_in* peeraddr)
/*-------------------------------------------------------------------------*/
{
    SLPDLog("Peer IP address: %s\n", inet_ntoa(peeraddr->sin_addr));
}

/*=========================================================================*/
void SLPDLogMessageInternals(SLPMessage message)
/*=========================================================================*/
{
    SLPDLog("Peer: \n");
    SLPDLog("   IP address: %s\n", inet_ntoa(message->peer.sin_addr));
    SLPDLog("Header:\n");
    SLPDLog("   version = %i\n",message->header.version);
    SLPDLog("   functionid = %i\n",message->header.functionid);
    SLPDLog("   length = %i\n",message->header.length);
    SLPDLog("   flags = %i\n",message->header.flags);
    SLPDLog("   extoffset = %i\n",message->header.extoffset);
    SLPDLog("   xid = %i\n",message->header.xid);
    SLPDLogBuffer("   langtag = ", message->header.langtaglen, message->header.langtag); 
    
    switch(message->header.functionid)
    {
    case SLP_FUNCT_SRVRQST:
        SLPDLogSrvRqstMessage(&(message->body.srvrqst));
        break;

    case SLP_FUNCT_SRVRPLY:
        SLPDLogSrvRplyMessage(&(message->body.srvrply));
        break;

    case SLP_FUNCT_SRVREG:
        SLPDLogSrvRegMessage(&(message->body.srvreg));
        break;

    case SLP_FUNCT_SRVDEREG:
        SLPDLogSrvDeRegMessage(&(message->body.srvdereg));
        break;

    case SLP_FUNCT_SRVACK:
        SLPDLogSrvAckMessage(&(message->body.srvack));
        break;

    case SLP_FUNCT_ATTRRQST:
        SLPDLogAttrRqstMessage(&(message->body.attrrqst));
        break;

    case SLP_FUNCT_ATTRRPLY:
        SLPDLogAttrRplyMessage(&(message->body.attrrply));
        break;

    case SLP_FUNCT_DAADVERT:
        SLPDLogDAAdvertMessage(&(message->body.daadvert));
        break;

    case SLP_FUNCT_SRVTYPERQST:
        SLPDLogSrvTypeRqstMessage(&(message->body.srvtyperqst));
        break;

    case SLP_FUNCT_SRVTYPERPLY:
        SLPDLogSrvTypeRplyMessage(&(message->body.srvtyperply));
        break;

    case SLP_FUNCT_SAADVERT:
        SLPDLogSAAdvertMessage(&(message->body.saadvert));
        break;

    default:
        SLPDLog("Message %i UNKNOWN:\n",message->header.functionid);
        SLPDLog("   This is really bad\n");
        break;
    }
}

/*=========================================================================*/
void SLPDLogMessage(const char* prefix, 
                    struct sockaddr_in* peerinfo,
                    SLPBuffer buf)
/* Log record of receiving or sending an SLP Message.  Logging will only   */
/* occur if message logging is enabled G_SlpProperty.traceMsg != 0         */
/*                                                                         */
/* prefix   (IN) an informative prefix for the log entry                   */
/*                                                                         */
/* peerinfo (IN) the source or destination peer                            */
/*                                                                         */
/* msg      (IN) the message to log                                        */
/*                                                                         */
/* Returns: none                                                           */
/*=========================================================================*/
{
    SLPMessage msg;
    
    if(G_SlpdProperty.traceMsg)
    {
        /* Don't log localhost traffic since it is probably IPC */
        /* and don't log empty messages                         */
        if(!ISLOCAL(peerinfo->sin_addr) && buf->end != buf->start)
        {
            msg = SLPMessageAlloc();
            if(msg)
            {
                SLPDLog("\n");
    	        SLPDLogTime();
                SLPDLog("MESSAGE - %s:\n",prefix);
                if(SLPMessageParseBuffer(peerinfo,buf,msg) == 0)
                {
                    SLPDLogMessageInternals(msg);
                }
                else 
                {
                    SLPDLog("Message parsing failed\n");
    	            SLPDLog("Peer: \n");
                    SLPDLog("   IP address: %s\n", inet_ntoa(msg->peer.sin_addr));	        
                }

                SLPMessageFree(msg);
            }
        }
    }
}

/*=========================================================================*/
void SLPDLogRegistration(const char* prefix, SLPDatabaseEntry* entry)
/* Log record of having added a registration to the database.  Logging of  */
/* registraions will only occur if registration trace is enabled           */
/* G_SlpProperty.traceReg != 0                                             */
/*                                                                         */
/* prefix   (IN) an informative prefix for the log entry                   */
/*                                                                         */
/* entry    (IN) the database entry that was affected                      */
/*                                                                         */
/* Returns: none                                                           */
/*=========================================================================*/
{
    if(G_SlpdProperty.traceReg)
    {
        SLPDLog("\n");
        SLPDLogTime();
        SLPDLog("DATABASE - %s:\n",prefix);
        SLPDLog("    SA address = ");
        switch(entry->msg->body.srvreg.source)
        {
        case SLP_REG_SOURCE_UNKNOWN:
            SLPDLog("<unknown>\n");
            break;
        case SLP_REG_SOURCE_REMOTE:
            SLPDLog("%s\n", inet_ntoa(entry->msg->peer.sin_addr));
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
    }
}

/*=========================================================================*/
void SLPDLogDAAdvertisement(const char* prefix,
                            SLPDatabaseEntry* entry)
/* Log record of addition or removal of a DA to the store of known DAs.    */
/* Will only occur if DA Advertisment message logging is enabled           */
/* G_SlpProperty.traceDATraffic != 0                                       */
/*                                                                         */
/* prefix   (IN) an informative prefix for the log entry                   */
/*                                                                         */
/* entry    (IN) the database entry that was affected                      */
/*                                                                         */
/* Returns: none                                                           */
/*=========================================================================*/
{
    if(G_SlpdProperty.traceDATraffic)
    {
        SLPDLog("\n");
        SLPDLogTime();
        SLPDLog("KNOWNDA - %s:\n",prefix);
        SLPDLog("    DA address = %s\n",inet_ntoa(entry->msg->peer.sin_addr));
        SLPDLogBuffer("    directory-agent-url = ",
                      entry->msg->body.daadvert.urllen,
                      entry->msg->body.daadvert.url);
        SLPDLog("    bootstamp = %x\n",entry->msg->body.daadvert.bootstamp);
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
    }
}


