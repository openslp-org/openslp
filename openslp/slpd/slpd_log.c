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
void SLPDLogSLPHeader(SLPHeader* header)
/*-------------------------------------------------------------------------*/
{
 }

/*-------------------------------------------------------------------------*/
void SLPDLogPeerInfo(SLPDPeerInfo* peerinfo)
/*-------------------------------------------------------------------------*/
{
    switch(peerinfo->peertype)
    {
    case SLPD_PEER_REMOTE:
        SLPLog("Peer Remote:\n");
        SLPLog("   IP address: %s\n",inet_ntoa(peerinfo->peeraddr.sin_addr));
        break;
        
    case SLPD_PEER_LOCAL:
        SLPLog("Peer Local:\n");
        SLPLog("   pid = %i\n",peerinfo->peerpid);
        SLPLog("   uid = %i\n",peerinfo->peeruid);
        SLPLog("   gid = %i\n",peerinfo->peergid);
        break;

    default:
        SLPLog("Peer Unknown:\n");
        SLPLog("   This is really bad\n");
    }
}

void SLPDLogMessage(SLPMessage message)
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
void SLPDLogTraceMsg(SLPDPeerInfo* peerinfo,
                     SLPBuffer recvbuf,
                     SLPBuffer sendbuf)
/*=========================================================================*/
{
    SLPMessage recvmsg;
    SLPMessage sendmsg;                
    
    recvmsg = SLPMessageAlloc();
    sendmsg = SLPMessageAlloc();
    if(recvmsg && sendmsg)
    {
        if(SLPMessageParseBuffer(recvbuf, recvmsg) == 0)
        {
            SLPLog("----------------------------------------\n");
            SLPLog("TRACEMSG IN:\n");
            SLPDLogPeerInfo(peerinfo);
            SLPDLogMessage(recvmsg);
            SLPLog("----------------------------------------\n");
        }

        if(SLPMessageParseBuffer(sendbuf, sendmsg) == 0)
        {
            SLPLog("----------------------------------------\n");
            SLPLog("TRACEMSG OUT:\n");
            SLPDLogPeerInfo(peerinfo);
            SLPDLogMessage(sendmsg);
            SLPLog("----------------------------------------\n");
        }
    }

    SLPMessageFree(recvmsg);
    SLPMessageFree(sendmsg);
}
