/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_process.c                                             */
/*                                                                         */
/* Abstract:    Processes incoming SLP messages                            */
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

#if defined(ENABLE_SLPv1)
extern int SLPDv1ProcessMessage(struct sockaddr_in* peeraddr,
								SLPBuffer recvbuf,
								SLPBuffer* sendbuf,
								SLPMessage message,
								int errocode);
#endif



/*-------------------------------------------------------------------------*/
int ProcessSASrvRqst(struct sockaddr_in* peeraddr,
					 SLPMessage message,
					 SLPBuffer* sendbuf,
					 int errorcode)
/*-------------------------------------------------------------------------*/
{
	int size = 0;
	SLPBuffer result = *sendbuf;

	if(message->body.srvrqst.scopelistlen == 0 ||
	   SLPIntersectStringList(message->body.srvrqst.scopelistlen,
							  message->body.srvrqst.scopelist,
							  G_SlpdProperty.useScopesLen,
							  G_SlpdProperty.useScopes) != 0)
	{
		/*----------------------*/
		/* Send back a SAAdvert */
		/*----------------------*/

		/*--------------------------------------------------------------*/
		/* ensure the buffer is big enough to handle the whole SAAdvert */
		/*--------------------------------------------------------------*/
		size = message->header.langtaglen + 21;	/* 14 bytes for header     */
												/*  2 bytes for url count  */
												/*  2 bytes for scope list len */
												/*  2 bytes for attr list len */
												/*  1 byte for authblock count */
		size += G_SlpdProperty.myUrlLen;
		size += G_SlpdProperty.useScopesLen;
		/* TODO: size += G_SlpdProperty.SAAttributes */

		result = SLPBufferRealloc(result,size);
		if(result == 0)
		{
			/* TODO: out of memory, what should we do here! */
			errorcode = SLP_ERROR_INTERNAL_ERROR;
			goto FINISHED;
		}

		/*----------------*/
		/* Add the header */
		/*----------------*/
		/*version*/
		*(result->start)       = 2;
		/*function id*/
		*(result->start + 1)   = SLP_FUNCT_SAADVERT;
		/*length*/
		ToUINT24(result->start + 2, size);
		/*flags*/
		ToUINT16(result->start + 5,
				 size > SLP_MAX_DATAGRAM_SIZE ? SLP_FLAG_OVERFLOW : 0);
		/*ext offset*/
		ToUINT24(result->start + 7,0);
		/*xid*/
		ToUINT16(result->start + 10,message->header.xid);
		/*lang tag len*/
		ToUINT16(result->start + 12,message->header.langtaglen);
		/*lang tag*/
		memcpy(result->start + 14,
			   message->header.langtag,
			   message->header.langtaglen);

		/*--------------------------*/
		/* Add rest of the SAAdvert */
		/*--------------------------*/
		result->curpos = result->start + 14 + message->header.langtaglen;
		/* url len */
		ToUINT16(result->curpos, G_SlpdProperty.myUrlLen);
		result->curpos = result->curpos + 2;
		/* url */
		memcpy(result->curpos,G_SlpdProperty.myUrl,G_SlpdProperty.myUrlLen);
		result->curpos = result->curpos + G_SlpdProperty.myUrlLen;
		/* scope list len */
		ToUINT16(result->curpos, G_SlpdProperty.useScopesLen);
		result->curpos = result->curpos + 2;
		/* scope list */
		memcpy(result->curpos,G_SlpdProperty.useScopes,G_SlpdProperty.useScopesLen);
		result->curpos = result->curpos + G_SlpdProperty.useScopesLen;
		/* attr list len */
		/* ToUINT16(result->curpos,G_SlpdProperty.SAAttributesLen) */
		ToUINT16(result->curpos, 0);
		result->curpos = result->curpos + 2;
		/* attr list */
		/* memcpy(result->start,G_SlpdProperty.SAAttributes,G_SlpdProperty.SAAttributesLen) */
		/* authblock count */
		*(result->curpos) = 0;
	}

	FINISHED:

	*sendbuf = result;

	return errorcode;
}


/*-------------------------------------------------------------------------*/
int ProcessDASrvRqst(struct sockaddr_in* peeraddr,
					 SLPMessage message,
					 SLPBuffer* sendbuf,
					 int errorcode)
/*-------------------------------------------------------------------------*/
{
	SLPDAEntry      daentry;
	SLPBuffer       tmp     = 0;
	SLPDAEntry*     entry   = 0;
	void*           i       = 0;

	/* set up local data */
	memset(&daentry,0,sizeof(daentry));    

	/*---------------------------------------------------------------------*/
	/* Special case for when libslp asks slpd (through the loopback) about */
	/* a known DAs. Fill sendbuf with DAAdverts from all known DAs.        */
	/*---------------------------------------------------------------------*/
	if(ISLOCAL(peeraddr->sin_addr))
	{
		/* TODO: be smarter about how much memory is allocated here! */
		/* 4096 may not be big enough to handle all DAAdverts        */
		*sendbuf = SLPBufferRealloc(*sendbuf, 4096);
		if(*sendbuf == 0)
		{
			return SLP_ERROR_INTERNAL_ERROR;
		}

		if(errorcode == 0)
		{

			/* Note: The weird *sendbuf code is making a single SLPBuffer */
			/*       that contains multiple DAAdverts.  This is a special */
			/*       process that only happens for the DA SrvRqst through */
			/*       loopback to the SLPAPI                               */
			while(SLPDKnownDAEnum(&i,&entry) == 0)
			{
				if(SLPDKnownDAEntryToDAAdvert(errorcode,
											  message->header.xid,
											  entry,
											  &tmp) == 0)
				{
					if(((*sendbuf)->curpos) + (tmp->end - tmp->start) > (*sendbuf)->end)
					{
						break;
					}

					memcpy((*sendbuf)->curpos, tmp->start, tmp->end - tmp->start);
					(*sendbuf)->curpos = ((*sendbuf)->curpos) + (tmp->end - tmp->start);
				}
			}

			/* Tack on a "terminator" DAAdvert */
			SLPDKnownDAEntryToDAAdvert(SLP_ERROR_INTERNAL_ERROR,
									   message->header.xid,
									   &daentry,
									   &tmp);
			if(((*sendbuf)->curpos) + (tmp->end - tmp->start) <= (*sendbuf)->end)
			{
				memcpy((*sendbuf)->curpos, tmp->start, tmp->end - tmp->start);
				(*sendbuf)->curpos = ((*sendbuf)->curpos) + (tmp->end - tmp->start);
			}

			/* mark the end of the sendbuf */
			(*sendbuf)->end = (*sendbuf)->curpos;


			if(tmp)
			{
				SLPBufferFree(tmp);
			}
		}

		return errorcode;
	}


	/*---------------------------------------------------------------------*/
	/* Normal case where a remote Agent asks for a DA                      */
	/*---------------------------------------------------------------------*/
	if(G_SlpdProperty.isDA)
	{
		if(message->body.srvrqst.scopelistlen == 0 ||
		   SLPIntersectStringList(message->body.srvrqst.scopelistlen, 
								  message->body.srvrqst.scopelist,
								  G_SlpdProperty.useScopesLen,
								  G_SlpdProperty.useScopes))
		{
			/* fill out real structure */
			daentry.bootstamp = G_SlpdProperty.DATimestamp;
			daentry.langtaglen = G_SlpdProperty.localeLen;
			daentry.langtag = (char*)G_SlpdProperty.locale;
			daentry.urllen = G_SlpdProperty.myUrlLen;
			daentry.url = (char*)G_SlpdProperty.myUrl;
			daentry.scopelistlen = G_SlpdProperty.useScopesLen;
			daentry.scopelist = (char*)G_SlpdProperty.useScopes;
			daentry.attrlistlen = 0;
			daentry.attrlist = 0;
			daentry.spilistlen = 0;
			daentry.spilist = 0;
		}
		else
		{
			errorcode =  SLP_ERROR_SCOPE_NOT_SUPPORTED;
		}
	}
	else
	{
		errorcode = SLP_ERROR_MESSAGE_NOT_SUPPORTED;       
	}

	/* don't return errorcodes to multicast messages */
	if(errorcode != 0)
	{
		if(message->header.flags & SLP_FLAG_MCAST ||
		   ISMCAST(peeraddr->sin_addr))
		{
			(*sendbuf)->end = (*sendbuf)->start;
			return errorcode;
		}
	}

	errorcode = SLPDKnownDAEntryToDAAdvert(errorcode,
										   message->header.xid,
										   &daentry,
										   sendbuf);

	return errorcode;
}


/*-------------------------------------------------------------------------*/
int ProcessSrvRqst(struct sockaddr_in* peeraddr,
				   SLPMessage message,
				   SLPBuffer* sendbuf,
				   int errorcode)
/*-------------------------------------------------------------------------*/
{
	int                     i;
	int                     size        = 0;
	int                     count       = 0;
	int                     found       = 0;
	SLPDDatabaseSrvUrl*     srvarray    = 0;
	SLPBuffer               result      = *sendbuf;

	/*--------------------------------------------------------------*/
	/* If errorcode is set, we can not be sure that message is good */
	/* Go directly to send response code                            */
	/*--------------------------------------------------------------*/
	if(errorcode)
	{
		goto RESPOND;
	}

	/*-------------------------------------------------*/
	/* Check for one of our IP addresses in the prlist */
	/*-------------------------------------------------*/
	if(SLPIntersectStringList(message->body.srvrqst.prlistlen,
							  message->body.srvrqst.prlist,
							  G_SlpdProperty.interfacesLen,
							  G_SlpdProperty.interfaces))
	{
		result->end = result->start;
		goto FINISHED;
	}

	/*------------------------------------------------*/
	/* Check to to see if a this is a special SrvRqst */
	/*------------------------------------------------*/
	if(SLPCompareString(message->body.srvrqst.srvtypelen,
						message->body.srvrqst.srvtype,
						23,
						"service:directory-agent") == 0)
	{
		errorcode = ProcessDASrvRqst(peeraddr, message, sendbuf, errorcode);
		return errorcode;
	}
	if(SLPCompareString(message->body.srvrqst.srvtypelen,
						message->body.srvrqst.srvtype,
						21,
						"service:service-agent") == 0)
	{
		errorcode = ProcessSASrvRqst(peeraddr, message, sendbuf, errorcode);
		return errorcode;
	}

	/* TODO: check the spi list of the message and return                  */
	/*       AUTHENTICATION_UNKNOWN since we do not do authentication yet  */

	/*------------------------------------*/
	/* Make sure that we handle the scope */
	/*------ -----------------------------*/
	if(SLPIntersectStringList(message->body.srvrqst.scopelistlen,
							  message->body.srvrqst.scopelist,
							  G_SlpdProperty.useScopesLen,
							  G_SlpdProperty.useScopes) != 0)
	{
		/*-------------------------------*/
		/* Find services in the database */
		/*-------------------------------*/
		while(found == count)
		{
			count += G_SlpdProperty.maxResults;

			if(srvarray) free(srvarray);
			srvarray = (SLPDDatabaseSrvUrl*)malloc(sizeof(SLPDDatabaseSrvUrl) * count);
			if(srvarray == 0)
			{
				found       = 0;
				errorcode   = SLP_ERROR_INTERNAL_ERROR;
				break;
			}

			found = SLPDDatabaseFindSrv(&(message->body.srvrqst), srvarray, count);
			if(found < 0)
			{
				found = 0;
				errorcode   = SLP_ERROR_INTERNAL_ERROR;
				break;
			}
		}

		/* remember the amount found if is really big for next time */
		if(found > G_SlpdProperty.maxResults)
		{
			G_SlpdProperty.maxResults = found;
		}
	}
	else
	{
		errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;
	}


	RESPOND:
	/*----------------------------------------------------------------*/
	/* Do not send error codes or empty replies to multicast requests */
	/*----------------------------------------------------------------*/
	if(found == 0 ||
	   errorcode != 0)
	{
		if(message->header.flags & SLP_FLAG_MCAST ||
		   ISMCAST(peeraddr->sin_addr))
		{
			result->end = result->start;
			goto FINISHED;  
		}
	}

	/*-------------------------------------------------------------*/
	/* ensure the buffer is big enough to handle the whole srvrply */
	/*-------------------------------------------------------------*/
	size = message->header.langtaglen + 18;	/* 14 bytes for header     */
											/*  2 bytes for error code */
											/*  2 bytes for url count  */
	if(errorcode == 0)
	{
		for(i=0;i<found;i++)
		{
			size += srvarray[i].urllen + 6;	/*  1 byte for reserved  */
			/*  2 bytes for lifetime */
			/*  2 bytes for urllen   */
			/*  1 byte for authcount */

			/* TODO: Fix this for authentication */
		} 
		result = SLPBufferRealloc(result,size);
		if(result == 0)
		{
			found = 0;
			errorcode = SLP_ERROR_INTERNAL_ERROR;
			goto FINISHED;
		}
	}

	/*----------------*/
	/* Add the header */
	/*----------------*/
	/*version*/
	*(result->start)       = 2;
	/*function id*/
	*(result->start + 1)   = SLP_FUNCT_SRVRPLY;
	/*length*/
	ToUINT24(result->start + 2, size);
	/*flags*/
	ToUINT16(result->start + 5,
			 size > SLP_MAX_DATAGRAM_SIZE ? SLP_FLAG_OVERFLOW : 0);
	/*ext offset*/
	ToUINT24(result->start + 7,0);
	/*xid*/
	ToUINT16(result->start + 10,message->header.xid);
	/*lang tag len*/
	ToUINT16(result->start + 12,message->header.langtaglen);
	/*lang tag*/
	memcpy(result->start + 14,
		   message->header.langtag,
		   message->header.langtaglen);


	/*-------------------------*/
	/* Add rest of the SrvRply */
	/*-------------------------*/
	result->curpos = result->start + 14 + message->header.langtaglen;
	/* error code*/
	ToUINT16(result->curpos, errorcode);
	result->curpos = result->curpos + 2;
	/* urlentry count */
	ToUINT16(result->curpos, found);
	result->curpos = result->curpos + 2;
	for(i=0;i<found;i++)
	{
		/* url-entry reserved */
		*result->curpos = 0;        
		result->curpos = result->curpos + 1;
		/* url-entry lifetime */
		ToUINT16(result->curpos,srvarray[i].lifetime);
		result->curpos = result->curpos + 2;
		/* url-entry urllen */
		ToUINT16(result->curpos,srvarray[i].urllen);
		result->curpos = result->curpos + 2;
		/* url-entry url */
		memcpy(result->curpos,srvarray[i].url,srvarray[i].urllen);
		result->curpos = result->curpos + srvarray[i].urllen;
		/* url-entry authcount */
		*result->curpos = 0;        
		result->curpos = result->curpos + 1;

		/* TODO: put in authentication stuff too */
	}

	FINISHED:   
	if(srvarray) free(srvarray);

	*sendbuf = result;
	return errorcode;
}


/*-------------------------------------------------------------------------*/
int ProcessSrvReg(struct sockaddr_in* peeraddr,
				  SLPMessage message,
				  SLPBuffer* sendbuf,
				  int errorcode)
/*                                                                         */
/* Returns: non-zero if message should be silently dropped                 */
/*-------------------------------------------------------------------------*/
{
	SLPBuffer result = *sendbuf;

	/*--------------------------------------------------------------*/
	/* If errorcode is set, we can not be sure that message is good */
	/* Go directly to send response code  also do not process mcast */
	/* srvreg or srvdereg messages                                  */
	/*--------------------------------------------------------------*/
	if(errorcode || message->header.flags & SLP_FLAG_MCAST)
	{
		goto RESPOND;
	}

	/*------------------------------------*/
	/* Make sure that we handle the scope */
	/*------ -----------------------------*/
	if(SLPIntersectStringList(message->body.srvreg.scopelistlen,
							  message->body.srvreg.scopelist,
							  G_SlpdProperty.useScopesLen,
							  G_SlpdProperty.useScopes))
	{

		/*-------------------------------*/
		/* TODO: Validate the authblocks */
		/*-------------------------------*/


		/*---------------------------------*/
		/* put the service in the database */
		/*---------------------------------*/
		errorcode = SLPDDatabaseReg(&(message->body.srvreg),
									message->header.flags & SLP_FLAG_FRESH,
									ISLOCAL(peeraddr->sin_addr));
		if(errorcode > 0)
		{
			errorcode = SLP_ERROR_INVALID_REGISTRATION;
		}
		else if(errorcode < 0)
		{
			errorcode = SLP_ERROR_INTERNAL_ERROR;
		}
	}
	else
	{
		errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;
	}

	RESPOND:    
	/*--------------------------------------------------------------------*/
	/* don't send back reply anything multicast SrvReg (set result empty) */
	/*--------------------------------------------------------------------*/
	if(message->header.flags & SLP_FLAG_MCAST ||
	   ISMCAST(peeraddr->sin_addr))
	{
		result->end = result->start;
		goto FINISHED;
	}


	/*------------------------------------------------------------*/
	/* ensure the buffer is big enough to handle the whole srvack */
	/*------------------------------------------------------------*/
	result = SLPBufferRealloc(result,message->header.langtaglen + 16);
	if(result == 0)
	{
		errorcode = SLP_ERROR_INTERNAL_ERROR;
		goto FINISHED;
	}


	/*----------------*/
	/* Add the header */
	/*----------------*/
	/*version*/
	*(result->start)       = 2;
	/*function id*/
	*(result->start + 1)   = SLP_FUNCT_SRVACK;
	/*length*/
	ToUINT24(result->start + 2,message->header.langtaglen + 16);
	/*flags*/
	ToUINT16(result->start + 5,0);
	/*ext offset*/
	ToUINT24(result->start + 7,0);
	/*xid*/
	ToUINT16(result->start + 10,message->header.xid);
	/*lang tag len*/
	ToUINT16(result->start + 12,message->header.langtaglen);
	/*lang tag*/
	memcpy(result->start + 14,
		   message->header.langtag,
		   message->header.langtaglen);

	/*-------------------*/
	/* Add the errorcode */
	/*-------------------*/
	ToUINT16(result->start + 14 + message->header.langtaglen, errorcode);

	FINISHED:
	*sendbuf = result;
	return errorcode;
}


/*-------------------------------------------------------------------------*/
int ProcessSrvDeReg(struct sockaddr_in* peeraddr,
					SLPMessage message,
					SLPBuffer* sendbuf,
					int errorcode)
/*                                                                         */
/* Returns: non-zero if message should be silently dropped                 */
/*-------------------------------------------------------------------------*/
{
	SLPBuffer result = *sendbuf;

	/*--------------------------------------------------------------*/
	/* If errorcode is set, we can not be sure that message is good */
	/* Go directly to send response code  also do not process mcast */
	/* srvreg or srvdereg messages                                  */
	/*--------------------------------------------------------------*/
	if(errorcode || message->header.flags & SLP_FLAG_MCAST)
	{
		goto RESPOND;
	}


	/*------------------------------------*/
	/* Make sure that we handle the scope */
	/*------------------------------------*/
	if(SLPIntersectStringList(message->body.srvdereg.scopelistlen,
							  message->body.srvdereg.scopelist,
							  G_SlpdProperty.useScopesLen,
							  G_SlpdProperty.useScopes))
	{
		/*-------------------------------*/
		/* TODO: Validate the authblocks */
		/*-------------------------------*/

		/*--------------------------------------*/
		/* remove the service from the database */
		/*--------------------------------------*/
		if(SLPDDatabaseDeReg(&(message->body.srvdereg)) == 0)
		{
			errorcode = 0;
		}
		else
		{
			errorcode = SLP_ERROR_INTERNAL_ERROR;
		}
	}
	else
	{
		errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;
	}

	RESPOND:
	/*---------------------------------------------------------*/
	/* don't do anything multicast SrvDeReg (set result empty) */
	/*---------------------------------------------------------*/
	if(message->header.flags & SLP_FLAG_MCAST ||
	   ISMCAST(peeraddr->sin_addr))
	{

		result->end = result->start;
		goto FINISHED;
	}

	/*------------------------------------------------------------*/
	/* ensure the buffer is big enough to handle the whole srvack */
	/*------------------------------------------------------------*/
	result = SLPBufferRealloc(result,message->header.langtaglen + 16);
	if(result == 0)
	{
		errorcode = SLP_ERROR_INTERNAL_ERROR;
		goto FINISHED;
	}

	/*----------------*/
	/* Add the header */
	/*----------------*/
	/*version*/
	*(result->start)       = 2;
	/*function id*/
	*(result->start + 1)   = SLP_FUNCT_SRVACK;
	/*length*/
	ToUINT24(result->start + 2,message->header.langtaglen + 16);
	/*flags*/
	ToUINT16(result->start + 5,0);
	/*ext offset*/
	ToUINT24(result->start + 7,0);
	/*xid*/
	ToUINT16(result->start + 10,message->header.xid);
	/*lang tag len*/
	ToUINT16(result->start + 12,message->header.langtaglen);
	/*lang tag*/
	memcpy(result->start + 14,
		   message->header.langtag,
		   message->header.langtaglen);

	/*-------------------*/
	/* Add the errorcode */
	/*-------------------*/
	ToUINT16(result->start + 14 + message->header.langtaglen, errorcode);

	FINISHED:
	*sendbuf = result;
	return errorcode;
}


/*-------------------------------------------------------------------------*/
int ProcessSrvAck(struct sockaddr_in* peeraddr,
				  SLPMessage message,
				  SLPBuffer* sendbuf,
				  int errorcode)
/*-------------------------------------------------------------------------*/
{
	/* Ignore SrvAck.  Just return errorcode to caller */
	SLPBuffer result = *sendbuf;

	result->end = result->start;
	return message->body.srvack.errorcode;
}


/*-------------------------------------------------------------------------*/
int ProcessAttrRqst(struct sockaddr_in* peeraddr,
					SLPMessage message,
					SLPBuffer* sendbuf,
					int errorcode)
/*-------------------------------------------------------------------------*/
{
	SLPDDatabaseAttr        attr;
	int                     size        = 0;
	int                     found       = 0;
	SLPBuffer               result      = *sendbuf;

	/*--------------------------------------------------------------*/
	/* If errorcode is set, we can not be sure that message is good */
	/* Go directly to send response code                            */
	/*--------------------------------------------------------------*/
	if(errorcode)
	{
		goto RESPOND;
	}

	/*-------------------------------------------------*/
	/* Check for one of our IP addresses in the prlist */
	/*-------------------------------------------------*/
	if(SLPIntersectStringList(message->body.attrrqst.prlistlen,
							  message->body.attrrqst.prlist,
							  G_SlpdProperty.interfacesLen,
							  G_SlpdProperty.interfaces))
	{
		result->end = result->start;
		goto FINISHED;
	}

	/* TODO: check the spi list of the message and return                  */
	/*       AUTHENTICATION_UNKNOWN since we do not do authentication yet  */

	/*------------------------------------*/
	/* Make sure that we handle the scope */
	/*------ -----------------------------*/
	if(SLPIntersectStringList(message->body.attrrqst.scopelistlen,
							  message->body.attrrqst.scopelist,
							  G_SlpdProperty.useScopesLen,
							  G_SlpdProperty.useScopes))
	{
		/*-------------------------------*/
		/* Find attributes in the database */
		/*-------------------------------*/
		found = SLPDDatabaseFindAttr(&(message->body.attrrqst), &attr);
		if(found < 0)
		{
			found = 0;
			errorcode   = SLP_ERROR_INTERNAL_ERROR;
		}
	}
	else
	{
		errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;
	}


	RESPOND:
	/*----------------------------------------------------------------*/
	/* Do not send error codes or empty replies to multicast requests */
	/*----------------------------------------------------------------*/
	if(found == 0 ||
	   errorcode != 0)
	{
		if(message->header.flags & SLP_FLAG_MCAST ||
		   ISMCAST(peeraddr->sin_addr))
		{
			result->end = result->start;
			goto FINISHED;  
		}
	}

	/*--------------------------------------------------------------*/
	/* ensure the buffer is big enough to handle the whole attrrply */
	/*--------------------------------------------------------------*/
	size = message->header.langtaglen + 20;	/* 14 bytes for header     */
											/*  2 bytes for error code */
											/*  2 bytes for attr-list len */
											/*  2 bytes for the authblockcount */
	size += attr.attrlistlen;

	/*-------------------*/
	/* Alloc the  buffer */
	/*-------------------*/
	result = SLPBufferRealloc(result,size);
	if(result == 0)
	{
		found = 0;
		errorcode = SLP_ERROR_INTERNAL_ERROR;
		goto FINISHED;
	}

	/*----------------*/
	/* Add the header */
	/*----------------*/
	/*version*/
	*(result->start)       = 2;
	/*function id*/
	*(result->start + 1)   = SLP_FUNCT_ATTRRPLY;
	/*length*/
	ToUINT24(result->start + 2,size);
	/*flags*/
	ToUINT16(result->start + 5,
			 size > SLP_MAX_DATAGRAM_SIZE ? SLP_FLAG_OVERFLOW : 0);
	/*ext offset*/
	ToUINT24(result->start + 7,0);
	/*xid*/
	ToUINT16(result->start + 10,message->header.xid);
	/*lang tag len*/
	ToUINT16(result->start + 12,message->header.langtaglen);
	/*lang tag*/
	memcpy(result->start + 14,
		   message->header.langtag,
		   message->header.langtaglen);

	/*--------------------------*/
	/* Add rest of the AttrRqst */
	/*--------------------------*/
	result->curpos = result->start + 14 + message->header.langtaglen;
	/* error code*/
	ToUINT16(result->curpos, errorcode);
	result->curpos = result->curpos + 2;
	/* attr-list len */
	ToUINT16(result->curpos, attr.attrlistlen);
	result->curpos = result->curpos + 2;
	memcpy(result->curpos, attr.attrlist, attr.attrlistlen);
	result->curpos = result->curpos + attr.attrlistlen;


	/* TODO: no auth block */
	ToUINT16(result->curpos, 0);

	FINISHED:
	*sendbuf = result;

	return errorcode;
}        


/*-------------------------------------------------------------------------*/
int ProcessDAAdvert(struct sockaddr_in* peeraddr,
					SLPMessage message,
					SLPBuffer* sendbuf,
					int errorcode)
/*-------------------------------------------------------------------------*/
{
	SLPDAEntry daentry;
	SLPBuffer result = *sendbuf;

	/*--------------------------------------------------------------*/
	/* If errorcode is set, we can not be sure that message is good */
	/* Go directly to send response code                            */
	/*--------------------------------------------------------------*/
	if(errorcode)
	{
		goto RESPOND;
	}

	/* Only process if errorcode is not set */
	if(message->body.daadvert.errorcode == SLP_ERROR_OK)
	{
		/* TODO: Authentication stuff here */


		daentry.langtaglen = message->header.langtaglen;
		daentry.langtag = message->header.langtag;
		daentry.bootstamp = message->body.daadvert.bootstamp;
		daentry.urllen = message->body.daadvert.urllen;
		daentry.url = message->body.daadvert.url;
		daentry.scopelistlen = message->body.daadvert.scopelistlen;
		daentry.scopelist = message->body.daadvert.scopelist;
		daentry.attrlistlen = message->body.daadvert.attrlistlen;
		daentry.attrlist = message->body.daadvert.attrlist;
		daentry.spilistlen = message->body.daadvert.spilistlen;
		daentry.spilist = message->body.daadvert.spilist;
		SLPDKnownDAAdd(&(peeraddr->sin_addr),&daentry);
	}

	RESPOND:
	/* DAAdverts should never be replied to.  Set result buffer to empty*/
	result->end = result->start;


	*sendbuf = result;
	return errorcode;
}


/*-------------------------------------------------------------------------*/
int ProcessSrvTypeRqst(struct sockaddr_in* peeraddr,
					   SLPMessage message,
					   SLPBuffer* sendbuf,
					   int errorcode)
/*-------------------------------------------------------------------------*/
{
	int                     i;
	int                     size         = 0;
	int                     count        = 0;
	int                     found        = 0;
	SLPDDatabaseSrvType*    srvtypearray = 0;
	SLPBuffer               result       = *sendbuf;


	/*-------------------------------------------------*/
	/* Check for one of our IP addresses in the prlist */
	/*-------------------------------------------------*/
	if(SLPIntersectStringList(message->body.srvtyperqst.prlistlen,
							  message->body.srvtyperqst.prlist,
							  G_SlpdProperty.interfacesLen,
							  G_SlpdProperty.interfaces))
	{
		result->end = result->start;
		goto FINISHED;
	}

	/* TODO: check the spi list of the message and return                  */
	/*       AUTHENTICATION_UNKNOWN since we do not do authentication yet  */

	/*------------------------------------*/
	/* Make sure that we handle the scope */
	/*------ -----------------------------*/
	if(SLPIntersectStringList(message->body.srvtyperqst.scopelistlen,
							  message->body.srvtyperqst.scopelist,
							  G_SlpdProperty.useScopesLen,
							  G_SlpdProperty.useScopes) != 0)
	{
		/*------------------------------------*/
		/* Find service types in the database */
		/*------------------------------------*/
		while(found == count)
		{
			count += G_SlpdProperty.maxResults;

			if(srvtypearray) free(srvtypearray);
			srvtypearray = (SLPDDatabaseSrvType*)malloc(sizeof(SLPDDatabaseSrvType) * count);
			if(srvtypearray == 0)
			{
				found       = 0;
				errorcode   = SLP_ERROR_INTERNAL_ERROR;
				break;
			}

			found = SLPDDatabaseFindType(&(message->body.srvtyperqst), srvtypearray, count);
			if(found < 0)
			{
				found = 0;
				errorcode   = SLP_ERROR_INTERNAL_ERROR;
				break;
			}
		}

		/* remember the amount found if is really big for next time */
		if(found > G_SlpdProperty.maxResults)
		{
			G_SlpdProperty.maxResults = found;
		}
	}
	else
	{
		errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;
	}

	/*----------------------------------------------------------------*/
	/* Do not send error codes or empty replies to multicast requests */
	/*----------------------------------------------------------------*/
	if(found == 0 ||
	   errorcode != 0)
	{
		if(message->header.flags & SLP_FLAG_MCAST ||
		   ISMCAST(peeraddr->sin_addr))
		{
			result->end = result->start;
			goto FINISHED;  
		}
	}

	/*-----------------------------------------------------------------*/
	/* ensure the buffer is big enough to handle the whole srvtyperply */
	/*-----------------------------------------------------------------*/
	size = message->header.langtaglen + 18;	/* 14 bytes for header     */
											/*  2 bytes for error code */
											/*  2 bytes for srvtype
												list length  */
	for(i=0;i<found;i++)
	{
		size += srvtypearray[i].typelen + 1; /* 1 byte for comma  */
	}
	if(found)
		size--;			/* remove the extra comma */
	result = SLPBufferRealloc(result,size);
	if(result == 0)
	{
		found = 0;
		errorcode = SLP_ERROR_INTERNAL_ERROR;
		goto FINISHED;
	}


	/*----------------*/
	/* Add the header */
	/*----------------*/
	/*version*/
	*(result->start)       = 2;
	/*function id*/
	*(result->start + 1)   = SLP_FUNCT_SRVTYPERPLY;
	/*length*/
	ToUINT24(result->start + 2,size);
	/*flags*/
	ToUINT16(result->start + 5,
			 size > SLP_MAX_DATAGRAM_SIZE ? SLP_FLAG_OVERFLOW : 0);
	/*ext offset*/
	ToUINT24(result->start + 7,0);
	/*xid*/
	ToUINT16(result->start + 10,message->header.xid);
	/*lang tag len*/
	ToUINT16(result->start + 12,message->header.langtaglen);
	/*lang tag*/
	memcpy(result->start + 14,
		   message->header.langtag,
		   message->header.langtaglen);

	/*-----------------------------*/
	/* Add rest of the SrvTypeRply */
	/*-----------------------------*/
	result->curpos = result->start + 14 + message->header.langtaglen;

	/* error code*/
	ToUINT16(result->curpos, errorcode);
	result->curpos += 2;

	/* length of srvtype-list */
	ToUINT16(result->curpos, size - (message->header.langtaglen + 18));
	result->curpos += 2;

	if(errorcode == 0)
	{
		for(i=0;i<found;i++)
		{
			memcpy(result->curpos, srvtypearray[i].type,
				   srvtypearray[i].typelen);
			result->curpos += srvtypearray[i].typelen;
			if(i < found - 1)
				*result->curpos++ = ',';
		}
	}

	FINISHED:   
	if(srvtypearray) free(srvtypearray);
	*sendbuf = result;
	return errorcode;
}

/*-------------------------------------------------------------------------*/
int ProcessSAAdvert(struct sockaddr_in* peerinfo,
					SLPMessage message,
					SLPBuffer* sendbuf,
					int errorcode)
/*-------------------------------------------------------------------------*/
{
	return errorcode;
}


/*=========================================================================*/
int SLPDProcessMessage(struct sockaddr_in* peerinfo,
					   SLPBuffer recvbuf,
					   SLPBuffer* sendbuf)
/* Processes the recvbuf and places the results in sendbuf                 */
/*                                                                         */
/* recvfd   - the socket the message was received on                       */
/*                                                                         */
/* recvbuf  - message to process                                           */
/*                                                                         */
/* sendbuf  - results of the processed message                             */
/*                                                                         */
/* Returns  - zero on success SLP_ERROR_PARSE_ERROR or                     */
/*            SLP_ERROR_INTERNAL_ERROR on ENOMEM.                          */
/*=========================================================================*/
{
	SLPMessage  message   = 0;
	int         errorcode  = 0;

	message = SLPMessageAlloc();
	if(message == 0)
	{
		return SLP_ERROR_INTERNAL_ERROR;
	}

	errorcode = SLPMessageParseBuffer(recvbuf, message);

#if defined(ENABLE_SLPv1)
	if(message->header.version == 1)
		return SLPDv1ProcessMessage(peerinfo, recvbuf, sendbuf,
									message, errorcode);
#endif


	/* Log trace message */
	SLPDLogTraceMsg("IN",peerinfo,recvbuf);

	switch(message->header.functionid)
	{
	case SLP_FUNCT_SRVRQST:
		errorcode = ProcessSrvRqst(peerinfo, message, sendbuf, errorcode);
		break;

	case SLP_FUNCT_SRVREG:
		errorcode = ProcessSrvReg(peerinfo, message,sendbuf, errorcode);
		if(errorcode)
		{
			SLPDKnownDAEcho(peerinfo, message, recvbuf);
		}
		break;

	case SLP_FUNCT_SRVDEREG:
		errorcode = ProcessSrvDeReg(peerinfo, message,sendbuf, errorcode);
		if(errorcode == 0)
		{
			SLPDKnownDAEcho(peerinfo, message, recvbuf);
		}
		break;

	case SLP_FUNCT_SRVACK:
		errorcode = ProcessSrvAck(peerinfo, message,sendbuf, errorcode);        
		break;

	case SLP_FUNCT_ATTRRQST:
		errorcode = ProcessAttrRqst(peerinfo, message,sendbuf, errorcode);
		break;

	case SLP_FUNCT_DAADVERT:
		errorcode = ProcessDAAdvert(peerinfo, message, sendbuf, errorcode);
		/* If necessary log that we received a DAAdvert */
		SLPDLogDATrafficMsg("IN", peerinfo, message);
		break;

	case SLP_FUNCT_SRVTYPERQST:
		errorcode = ProcessSrvTypeRqst(peerinfo, message, sendbuf, errorcode);
		break;

	case SLP_FUNCT_SAADVERT:
		errorcode = ProcessSAAdvert(peerinfo, message, sendbuf, errorcode);
		break;

	default:
		/* This may happen on a really early parse error or version not */
		/* supported error */

		/* TODO log errorcode here */

		break;
	}

	/* Log traceMsg of message was received and the one that will be sent */
	SLPDLogTraceMsg("OUT",peerinfo,*sendbuf);

	SLPMessageFree(message);

	/* Log reception of important errors */
	switch(errorcode)
	{
	case SLP_ERROR_DA_BUSY_NOW:
		SLPLog("DA_BUSY from %s\n",
			   inet_ntoa(peerinfo->sin_addr));
		break;
	case SLP_ERROR_INTERNAL_ERROR:
		SLPLog("INTERNAL_ERROR from %s\n",
			   inet_ntoa(peerinfo->sin_addr));
		break;
	case SLP_ERROR_PARSE_ERROR:
		SLPLog("PARSE_ERROR from %s\n",
			   inet_ntoa(peerinfo->sin_addr));
		break;
	case SLP_ERROR_VER_NOT_SUPPORTED:
		SLPLog("VER_NOT_SUPPORTED from %s\n",
			   inet_ntoa(peerinfo->sin_addr));
		break;                    
	}

	return errorcode;
}                
