/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_log.c                                                 */
/*                                                                         */
/* Abstract:    Processes slp messages                                     */
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


#include "slpd.h"

/*-------------------------------------------------------------------------*/
void SLPDLogSrvRqstMessage(SLPSrvRqst* srvrqst)
/*-------------------------------------------------------------------------*/
{
    SLPLog("Message SRVRQST:\n");
    SLPLog("   srvtype = \"%*s\"\n", srvrqst->srvtypelen, srvrqst->srvtype);
    SLPLog("   scopelist = \"%*s\"\n", srvrqst->scopelistlen, srvrqst->scopelist);
    SLPLog("   predicate = \"%*s\"\n", srvrqst->predicatelen, srvrqst->predicate);
}

/*-------------------------------------------------------------------------*/
void SLPDLogSrvRplyMessage(SLPSrvRply* srvrply)
/*-------------------------------------------------------------------------*/
{
    SLPLog("Message SRVRPLY:\n");
    SLPLog("   errorcode = %i\n",srvrply->errorcode);
}
                                 
/*-------------------------------------------------------------------------*/
void SLPDLogSrvRegMessage(SLPSrvReg* srvreg)
/*-------------------------------------------------------------------------*/
{
    SLPLog("Message SRVREG:\n");
    SLPLog("   type = \"%*s\"\n", srvreg->srvtypelen, srvreg->srvtype);
    SLPLog("   scope = \"%*s\"\n", srvreg->scopelistlen, srvreg->scopelist);
    SLPLog("   attributes = \"%*s\"\n", srvreg->attrlistlen, srvreg->attrlist);
}
        
/*-------------------------------------------------------------------------*/
void SLPDLogSrvDeRegMessage(SLPSrvDeReg* srvdereg)
/*-------------------------------------------------------------------------*/
{
    SLPLog("Message SRVDEREG:\n");
}
        
/*-------------------------------------------------------------------------*/
void SLPDLogSrvAckMessage(SLPSrvAck* srvack)
/*-------------------------------------------------------------------------*/
{
    SLPLog("Message SRVACK:\n");
    SLPLog("   errorcode = %i\n",srvack->errorcode);
}
        
/*-------------------------------------------------------------------------*/
void SLPDLogAttrRqstMessage(SLPAttrRqst* attrrqst)
/*-------------------------------------------------------------------------*/
{
    SLPLog("Message ATTRRQST:\n");
}

/*-------------------------------------------------------------------------*/
void SLPDLogAttrRplyMessage(SLPAttrRply* attrrply)
/*-------------------------------------------------------------------------*/
{
    SLPLog("Message ATTRRPLY:\n");
    SLPLog("   errorcode = %i\n",attrrply->errorcode);
} 

/*-------------------------------------------------------------------------*/
void SLPDLogDAAdvertMessage(SLPDAAdvert* daadvert)
/*-------------------------------------------------------------------------*/
{
    SLPLog("Message DAADVERT:\n");
}
     
/*-------------------------------------------------------------------------*/
void SLPDLogSrvTypeRqstMessage(SLPSrvTypeRqst* srvtyperqst)
/*-------------------------------------------------------------------------*/
{
    SLPLog("Message SRVTYPERQST:\n");
}

/*-------------------------------------------------------------------------*/
void SLPDLogSrvTypeRplyMessage(SLPSrvTypeRply* srvtyperply)
/*-------------------------------------------------------------------------*/ 
{
    SLPLog("Message SRVTYPERPLY:\n");
    SLPLog("   errorcode = %i\n",srvtyperply->errorcode);
}        

/*-------------------------------------------------------------------------*/
void SLPDLogSAAdvertMessage(SLPSAAdvert* saadvert)
/*-------------------------------------------------------------------------*/
{
    SLPLog("Message SAADVERT:\n");
}


/*-------------------------------------------------------------------------*/
void SLPDLogPeerAddr(struct sockaddr_in* peeraddr)
/*-------------------------------------------------------------------------*/
{
    SLPLog("Peer Information:\n");
    SLPLog("   IP address: %s\n",inet_ntoa(peeraddr->sin_addr));
}

/*-------------------------------------------------------------------------*/
void SLPDLogMessage(SLPMessage message)
/*-------------------------------------------------------------------------*/
{
    SLPLog("Header:\n");
    SLPLog("   version = %i\n",message->header.version);
    SLPLog("   functionid = %i\n",message->header.functionid);
    SLPLog("   length = %i\n",message->header.length);
    SLPLog("   flags = %i\n",message->header.flags);
    SLPLog("   extoffset = %i\n",message->header.extoffset);
    SLPLog("   xid = %i\n",message->header.xid);
    SLPLog("   langtaglen = %i\n",message->header.langtaglen);
    SLPLog("   langtag = "); 
    SLPLogBuffer(message->header.langtag, message->header.langtaglen);
    SLPLog("\n");

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
        SLPLog("Message UNKNOWN:\n");
        SLPLog("   This is really bad\n");
        break;
    }
}

/*=========================================================================*/
void SLPDLogTraceMsg(const char* prefix,
                     struct sockaddr_in* peeraddr,
                     SLPBuffer buf)
/*=========================================================================*/
{
    SLPMessage msg;
    if(G_SlpdProperty.traceMsg)
    {
        msg = SLPMessageAlloc();
        if(msg)
        {
            if(SLPMessageParseBuffer(buf,msg) == 0)
            {
                SLPLog("----------------------------------------\n");
                SLPLog("traceMsg %s:\n",prefix);
                SLPLog("----------------------------------------\n");
                SLPDLogPeerAddr(peeraddr);
                SLPDLogMessage(msg);
                SLPLog("\n");
            }
        }

        SLPMessageFree(msg);
    }
}



/*=========================================================================*/
void SLPDLogTraceReg(const char* prefix, SLPDDatabaseEntry* entry)
/* Logs at traceReg message to indicate that a service registration (or    */
/* service de-registration has occured.                                    */
/*                                                                         */
/* prefix (IN) an appropriate prefix string like "reg" or "dereg"          */
/*                                                                         */
/* entry  (IN) the database entry of the service                           */
/*                                                                         */
/* returns:  None                                                          */
/*=========================================================================*/
{
    if(G_SlpdProperty.traceReg)
    {
        SLPLog("----------------------------------------\n");
        SLPLog("traceReg %s:\n",prefix);
        SLPLog("----------------------------------------\n");
        SLPLog("language tag = ");
        SLPLogBuffer(entry->langtag, entry->langtaglen);
        SLPLog("\nlifetime = %i\n",entry->lifetime); 
        SLPLog("url = ");
        SLPLogBuffer(entry->url, entry->urllen);
        SLPLog("\nscope = ");
        SLPLogBuffer(entry->scopelist, entry->scopelistlen);
        SLPLog("\nservice type = ");
        SLPLogBuffer(entry->srvtype, entry->srvtypelen);
        SLPLog("\nAttributes = ");
        SLPLogBuffer(entry->attrlist, entry->attrlistlen);
        SLPLog("\n\n");
    }
}

/*=========================================================================*/
void SLPDLogDATrafficMsg(const char* prefix,
                         struct sockaddr_in* peeraddr,
                         SLPMessage daadvert)
/*=========================================================================*/
{
    if(G_SlpdProperty.traceDATraffic)
    {
        SLPLog("----------------------------------------\n");
        SLPLog("traceDATraffic %s:\n",prefix);
        SLPLog("----------------------------------------\n");
        SLPDLogPeerAddr(peeraddr);
        SLPDLogMessage(daadvert);
        SLPLog("\n\n");
    }
}


/*=========================================================================*/
void SLPDLogKnownDA(const char* prefix,
                    SLPDAEntry* daentry)
/*=========================================================================*/
{
    SLPLog("----------------------------------------\n");
    SLPLog("Known DA %s:\n",prefix);
    SLPLog("----------------------------------------\n");
    SLPLog("   url = ");
    SLPLogBuffer(daentry->url, daentry->urllen);
    SLPLog("\n   scope = ");
    SLPLogBuffer(daentry->scopelist, daentry->scopelistlen);
    SLPLog("\n\n");
}


