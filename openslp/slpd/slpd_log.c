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
/* Copyright (c) 1995, 1999  Caldera Systems, Inc.                         */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify it */
/* under the terms of the GNU Lesser General Public License as published   */
/* by the Free Software Foundation; either version 2.1 of the License, or  */
/* (at your option) any later version.                                     */
/*                                                                         */
/*     This program is distributed in the hope that it will be useful,     */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       */
/*     GNU Lesser General Public License for more details.                 */
/*                                                                         */
/*     You should have received a copy of the GNU Lesser General Public    */
/*     License along with this program; see the file COPYING.  If not,     */
/*     please obtain a copy from http://www.gnu.org/copyleft/lesser.html   */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*     Please submit patches to http://www.openslp.org                     */
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
    	
        #ifdef USE_PREDICATES
        {/* Print attributes. */
    		char *str;
    		SLPError err;
    		//err = SLPAttrSerialize(entry->attr, NULL, &str, SLP_FALSE);
    		if (err != SLP_OK) 
    		{
        		SLPLog("\nerror %i when building attributes", err);
    		} 
    		else
    		{
    			SLPLog("\nattributes = %s", str);
    			free(str);
    		}      
    	}
        #else
        SLPLog("\nAttributes = ");
        SLPLogBuffer(entry->attrlist, entry->attrlistlen);
        #endif
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
                    struct in_addr* peeraddr)
/*=========================================================================*/
{
    SLPLog("----------------------------------------\n");
    SLPLog("Known DA %s:\n",prefix);
    SLPLog("----------------------------------------\n");
    SLPLog("DA Peer\n");
    SLPLog("   IP address: %s\n",inet_ntoa(*peeraddr));
    SLPLog("\n\n");
}


