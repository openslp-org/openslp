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
#include <limits.h>

/*-------------------------------------------------------------------------*/
int v1ProcessDASrvRqst(struct sockaddr_in* peeraddr,
                       SLPMessage message,
                       SLPBuffer* sendbuf,
                       int errorcode)
/*-------------------------------------------------------------------------*/
{
    int size = 0, noscopes = 0, urllen = INT_MAX, scopeslen = INT_MAX;
    SLPBuffer result = *sendbuf;

    /*--------------------------------------------------------------*/
    /* If errorcode is set, we can not be sure that message is good */
    /* Go directly to send response code                            */
    /*--------------------------------------------------------------*/
    if(errorcode)
    {
        goto RESPOND;
    }

    if (message->body.srvrqst.scopelistlen != 0 &&
	SLPIntersectStringList(message->body.srvrqst.scopelistlen,
			       message->body.srvrqst.scopelist,
			       G_SlpdProperty.useScopesLen,
			       G_SlpdProperty.useScopes) == 0 )
    {
        errorcode = SLP_ERROR_SCOPE_NOT_SUPPORTED;
    }

RESPOND:
    /*----------------------------------------------------------------*/
    /* Do not send error codes or empty replies to multicast requests */
    /*----------------------------------------------------------------*/
    if (message->header.flags & SLP_FLAG_MCAST)
    {
        if (errorcode != 0)
        {
	    errorcode = 0;	/* don't log these errors */
            result->end = result->start;
            goto FINISHED;   
        }
    }


    /*-------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvrply */
    /*-------------------------------------------------------------*/
    size = 18; /* 12 bytes for header, 2 errorcode,  2 bytes for url
		  len, 2 bytes for scope list len */

    errorcode = SLPv1ToEncoding(0, &urllen, message->header.encoding,
				G_SlpdProperty.myUrl,
				G_SlpdProperty.myUrlLen);
    if (errorcode)
	goto FINISHED;
    size += urllen;
    
    if (G_SlpdProperty.useScopesLen == 7 &&
	strncasecmp(G_SlpdProperty.useScopes, "default", 7) == 0)
    {
	/* Do not advertise any scope if we operate only in the
	   default scope. For SLPv1 UAs we are an "unscoped" DA */
	noscopes = 1;
    }
    else
    {
	errorcode = SLPv1ToEncoding(0, &scopeslen,
				    message->header.encoding, 
				    G_SlpdProperty.useScopes,
				    G_SlpdProperty.useScopesLen);
	if (errorcode)
	    goto FINISHED;
	size += scopeslen;
    }

    result = SLPBufferRealloc(result,size);
    if (result == 0)
    {
        /* TODO: out of memory, what should we do here! */
        errorcode = SLP_ERROR_INTERNAL_ERROR;
        goto FINISHED;                       
    }

    /*----------------*/
    /* Add the header */
    /*----------------*/
    /*version*/
    *(result->start)       = 1;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_DAADVERT;
    /*length*/
    ToUINT16(result->start + 2, size);
    /*flags - TODO set the flags correctly */
    *(result->start + 4) = message->header.flags |
	(size > SLP_MAX_DATAGRAM_SIZE ? SLPv1_FLAG_OVERFLOW : 0);  
    /*dialect*/
    *(result->start + 5) = 0;
    /*language code*/
    memcpy(result->start + 6, message->header.langtag, 2);
    ToUINT16(result->start + 8, message->header.encoding);
    /*xid*/
    ToUINT16(result->start + 10, message->header.xid);

    /*--------------------------*/
    /* Add rest of the DAAdvert */
    /*--------------------------*/
    result->curpos = result->start + 12;
    /* error code */
    ToUINT16(result->curpos,errorcode);
    result->curpos = result->curpos + 2;
    /* url len */
    ToUINT16(result->curpos, urllen);
    result->curpos = result->curpos + 2;
    /* url */
    SLPv1ToEncoding(result->curpos, &urllen, message->header.encoding, 
		    G_SlpdProperty.myUrl,
		    G_SlpdProperty.myUrlLen);
    result->curpos = result->curpos + urllen;
    if (noscopes)
    {
	/* Advertise ourselves as a unscoped DA for SLPv1 */
	ToUINT16(result->curpos, 0);
	result->curpos = result->curpos + 2;
    }
    else
    {
	/* scope list len */
	ToUINT16(result->curpos, G_SlpdProperty.useScopesLen);
	result->curpos = result->curpos + 2;
	/* scope list */
	SLPv1ToEncoding(result->curpos, &scopeslen,
			message->header.encoding,  
			G_SlpdProperty.useScopes,
			G_SlpdProperty.useScopesLen);
	result->curpos = result->curpos + scopeslen;
    }

FINISHED:
    *sendbuf = result;
    return errorcode;
}


/*-------------------------------------------------------------------------*/
int v1ProcessSrvRqst(struct sockaddr_in* peeraddr,
		     SLPMessage message,
		     SLPBuffer* sendbuf,
		     int errorcode)
/*-------------------------------------------------------------------------*/
{
    int                     i, urllen;
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
    if (SLPIntersectStringList(message->body.srvrqst.prlistlen,
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
    if (SLPCompareString(message->body.srvrqst.srvtypelen,
                         message->body.srvrqst.srvtype,
                         15,
                         "directory-agent") == 0)
    {
        errorcode = v1ProcessDASrvRqst(peeraddr, message, sendbuf, errorcode);
        return errorcode;
    }

    /*------------------------------------*/
    /* Make sure that we handle the scope */
    /*------ -----------------------------*/
    if (SLPIntersectStringList(message->body.srvrqst.scopelistlen,
                               message->body.srvrqst.scopelist,
                               G_SlpdProperty.useScopesLen,
                               G_SlpdProperty.useScopes) != 0)
    {
        /*-------------------------------*/
        /* Find services in the database */
        /*-------------------------------*/
        while (found == count)
        {
            count += G_SlpdProperty.maxResults;
            
            if (srvarray) free(srvarray);
            srvarray = (SLPDDatabaseSrvUrl*)malloc(sizeof(SLPDDatabaseSrvUrl) * count);
            if (srvarray == 0)
            {
                found       = 0;
                errorcode   = SLP_ERROR_INTERNAL_ERROR;
                break;
            }

            found = SLPDDatabaseFindSrv(&(message->body.srvrqst), srvarray, count);
            if (found < 0)
            {
                found = 0;
                errorcode   = SLP_ERROR_INTERNAL_ERROR;
                break;
            }
        }

        /* remember the amount found if is really big for next time */
        if (found > G_SlpdProperty.maxResults)
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
    if (message->header.flags & SLP_FLAG_MCAST)
    {
        if (found == 0)
        {
            result->end = result->start;
            goto FINISHED;  
        }
    }

    /*-------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvrply */
    /*-------------------------------------------------------------*/
    size = 16; /* 12 bytes for header,  2 bytes for error code,   2
		  bytes for url count */ 
    if (errorcode == 0)
    {
        for (i=0;i<found;i++)
        {
	    urllen = INT_MAX;
	    errorcode = SLPv1ToEncoding(0, &urllen,
					message->header.encoding,  
					srvarray[i].url,
					srvarray[i].urllen);
	    if (errorcode)
		break;
            size += urllen + 4;	/* 2 bytes for lifetime,
				   2 bytes for urllen */ 
        } 
        result = SLPBufferRealloc(result,size);
        if (result == 0)
        {
            found = 0;
            errorcode = SLP_ERROR_INTERNAL_ERROR;
        }
    }

    /*----------------*/
    /* Add the header */
    /*----------------*/
    /*version*/
    *(result->start)       = 1;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_SRVRPLY;
    /*length*/
    ToUINT16(result->start + 2, size);
    /*flags - TODO set the flags correctly */
    *(result->start + 4) = message->header.flags |
	(size > SLP_MAX_DATAGRAM_SIZE ? SLPv1_FLAG_OVERFLOW : 0);  
    /*dialect*/
    *(result->start + 5) = 0;
    /*language code*/
    memcpy(result->start + 6, message->header.langtag, 2);
    ToUINT16(result->start + 8, message->header.encoding);
    /*xid*/
    ToUINT16(result->start + 10, message->header.xid);

    /*-------------------------*/
    /* Add rest of the SrvRply */
    /*-------------------------*/
    result->curpos = result->start + 12;
    /* error code*/
    ToUINT16(result->curpos, errorcode);
    result->curpos = result->curpos + 2;
    /* urlentry count */
    ToUINT16(result->curpos, found);
    result->curpos = result->curpos + 2;
    for (i=0;i<found;i++)
    {
        /* url-entry lifetime */
        ToUINT16(result->curpos,srvarray[i].lifetime);
        result->curpos = result->curpos + 2;
        /* url-entry url and urllen */
	urllen = size;		
	errorcode = SLPv1ToEncoding(result->curpos + 2, &urllen,
				    message->header.encoding,  
				    srvarray[i].url,
				    srvarray[i].urllen);
        ToUINT16(result->curpos, urllen);
        result->curpos = result->curpos + 2 + urllen;
    }

FINISHED:   
    if (srvarray) free(srvarray);

    *sendbuf = result;
    return errorcode;
}

/*-------------------------------------------------------------------------*/
int v1ProcessSrvReg(struct sockaddr_in* peeraddr,
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
    if (SLPIntersectStringList(message->body.srvreg.scopelistlen,
                               message->body.srvreg.scopelist,
                               G_SlpdProperty.useScopesLen,
                               G_SlpdProperty.useScopes))
    {
        int uid;
#ifdef WIN32
        uid = 0; /* no equivalent of UID on Windows */
#else
        uid = getuid();
#endif
        /*---------------------------------*/
        /* put the service in the database */
        /*---------------------------------*/
        if (SLPDDatabaseReg(&(message->body.srvreg),
                            message->header.flags | SLP_FLAG_FRESH,
                            ISLOCAL(peeraddr->sin_addr)) == 0)
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
    /*--------------------------------------------------------------------*/
    /* don't send back reply anything multicast SrvReg (set result empty) */
    /*--------------------------------------------------------------------*/
    if (message->header.flags & SLP_FLAG_MCAST)
    {
        result->end = result->start;
        goto FINISHED;
    }
    
    
    /*------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvack */
    /*------------------------------------------------------------*/
    result = SLPBufferRealloc(result, 14);
    if(result == 0)
    {
        errorcode = SLP_ERROR_INTERNAL_ERROR;
        goto FINISHED;
    }


    /*----------------*/
    /* Add the header */
    /*----------------*/
    /*version*/
    *(result->start)       = 1;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_SRVACK;
    /*length*/
    ToUINT16(result->start + 2, 14);
    /*flags - TODO set the flags correctly */
    *(result->start + 4) = 0;
    /*dialect*/
    *(result->start + 5) = 0;
    /*language code*/
    memcpy(result->start + 6, message->header.langtag, 2);
    ToUINT16(result->start + 8, message->header.encoding);
    /*xid*/
    ToUINT16(result->start + 10, message->header.xid);

    /*-------------------*/
    /* Add the errorcode */
    /*-------------------*/
    ToUINT16(result->start + 12, errorcode);

FINISHED:
    *sendbuf = result;
    return errorcode;
}


/*-------------------------------------------------------------------------*/
int v1ProcessSrvDeReg(struct sockaddr_in* peeraddr,
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
    if (SLPIntersectStringList(message->body.srvdereg.scopelistlen,
                               message->body.srvdereg.scopelist,
                               G_SlpdProperty.useScopesLen,
                               G_SlpdProperty.useScopes))
    {
        /*--------------------------------------*/
        /* remove the service from the database */
        /*--------------------------------------*/
        if (SLPDDatabaseDeReg(&(message->body.srvdereg)) == 0)
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
    if (message->header.flags & SLP_FLAG_MCAST)
    {
        
        result->end = result->start;
        goto FINISHED;
    }

    /*------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvack */
    /*------------------------------------------------------------*/
    result = SLPBufferRealloc(result, 14);
    if(result == 0)
    {
        errorcode = SLP_ERROR_INTERNAL_ERROR;
        goto FINISHED;
    }

    /*----------------*/
    /* Add the header */
    /*----------------*/
    /*version*/
    *(result->start)       = 1;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_SRVACK;
    /*length*/
    ToUINT16(result->start + 2, 14);
    /*flags - TODO set the flags correctly */
    *(result->start + 4) = 0;
    /*dialect*/
    *(result->start + 5) = 0;
    /*language code*/
    memcpy(result->start + 6, message->header.langtag, 2);
    ToUINT16(result->start + 8, message->header.encoding);
    /*xid*/
    ToUINT16(result->start + 10, message->header.xid);

    /*-------------------*/
    /* Add the errorcode */
    /*-------------------*/
    ToUINT16(result->start + 12, errorcode);

FINISHED:
    *sendbuf = result;
    return errorcode;
}

/*-------------------------------------------------------------------------*/
int v1ProcessAttrRqst(struct sockaddr_in* peeraddr,
		      SLPMessage message,
		      SLPBuffer* sendbuf,
		      int errorcode)
/*-------------------------------------------------------------------------*/
{
    
    SLPDDatabaseAttr        attr;
    int                     attrlen     = 0;
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
    /*-------------------------------------------------*/
    if (SLPIntersectStringList(message->body.attrrqst.prlistlen,
                               message->body.attrrqst.prlist,
                               G_SlpdProperty.interfacesLen,
                               G_SlpdProperty.interfaces))
    {
        result->end = result->start;
        goto FINISHED;
    }

    /*------------------------------------*/
    /* Make sure that we handle the scope */
    /*------ -----------------------------*/
    if (SLPIntersectStringList(message->body.attrrqst.scopelistlen,
                               message->body.attrrqst.scopelist,
                               G_SlpdProperty.useScopesLen,
                               G_SlpdProperty.useScopes) )
    {
        /*-------------------------------*/
        /* Find attributes in the database */
        /*-------------------------------*/
        found = SLPDDatabaseFindAttr(&(message->body.attrrqst), &attr);
        if (found < 0)
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
    if (message->header.flags & SLP_FLAG_MCAST)
    {
        if (found == 0)
        {
            result->end = result->start;
            goto FINISHED;  
        }
    }

    /*--------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole attrrply */
    /*--------------------------------------------------------------*/
    size = 16; /* 12 bytes for header, 2 bytes for error code, 2 bytes
		  for attr-list len */ 

    attrlen = INT_MAX;
	errorcode = SLPv1ToEncoding(0, &attrlen,
				    message->header.encoding,  
				    attr.attrlist,
				    attr.attrlistlen);
	if (errorcode)
	{
	    found = 0;   
	}   
    size += attrlen;
    

    /*-------------------*/
    /* Alloc the  buffer */
    /*-------------------*/
    result = SLPBufferRealloc(result,size);
    if (result == 0)
    {
        found = 0;
        errorcode = SLP_ERROR_INTERNAL_ERROR;
    }

    /*----------------*/
    /* Add the header */
    /*----------------*/
    /*version*/
    *(result->start)       = 1;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_ATTRRPLY;
    /*length*/
    ToUINT16(result->start + 2, size);
    /*flags - TODO set the flags correctly */
    *(result->start + 4) = message->header.flags |
	(size > SLP_MAX_DATAGRAM_SIZE ? SLPv1_FLAG_OVERFLOW : 0);  
    /*dialect*/
    *(result->start + 5) = 0;
    /*language code*/
    memcpy(result->start + 6, message->header.langtag, 2);
    ToUINT16(result->start + 8, message->header.encoding);
    /*xid*/
    ToUINT16(result->start + 10, message->header.xid);

    /*--------------------------*/
    /* Add rest of the AttrRply */
    /*--------------------------*/
    result->curpos = result->start + 12;
    /* error code*/
    ToUINT16(result->curpos, errorcode);
    result->curpos = result->curpos + 2;
    /* attr-list len */
    ToUINT16(result->curpos, attrlen);
    result->curpos = result->curpos + 2;
    attrlen = size;
	SLPv1ToEncoding(result->curpos, &attrlen,
                    message->header.encoding,
                    attr.attrlist,
                    attr.attrlistlen);
    result->curpos = result->curpos + attrlen; 

FINISHED:
    *sendbuf = result;
    return errorcode;
}        


/*-------------------------------------------------------------------------*/
int v1ProcessSrvTypeRqst(struct sockaddr_in* peeraddr,
			 SLPMessage message,
			 SLPBuffer* sendbuf,
			 int errorcode)
/*-------------------------------------------------------------------------*/
{
    int                     i, typelen;
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

    /*------------------------------------*/
    /* Make sure that we handle the scope */
    /*------------------------------------*/
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
        if (found > G_SlpdProperty.maxResults)
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
    if (message->header.flags & SLP_FLAG_MCAST)
    {
        if (found == 0 || errorcode != 0)
        {
            result->end = result->start;
            goto FINISHED;  
        }
    }   


    /*-----------------------------------------------------------------*/
    /* ensure the buffer is big enough to handle the whole srvtyperply */
    /*-----------------------------------------------------------------*/
    size = 16; /* 12 bytes for header, 2 bytes for error code, 2 bytes
		  for srvtype count  */
    if (errorcode == 0)
    {
	for(i=0;i<found;i++)
	{
	    typelen = INT_MAX;
	    errorcode = SLPv1ToEncoding(0, &typelen,
					message->header.encoding,  
					srvtypearray[i].type,
					srvtypearray[i].typelen);
	    if (errorcode)
		break;
            size += typelen + 2;  /*  2 byte for length */
	}
    }
    result = SLPBufferRealloc(result,size);
    if(result == 0)
    {
        found = 0;
        errorcode = SLP_ERROR_INTERNAL_ERROR;
    }


    /*----------------*/
    /* Add the header */
    /*----------------*/
    /*version*/
    *(result->start)       = 1;
    /*function id*/
    *(result->start + 1)   = SLP_FUNCT_SRVTYPERPLY;
    /*length*/
    ToUINT16(result->start + 2, size);
    /*flags - TODO set the flags correctly */
    *(result->start + 4) = message->header.flags |
	(size > SLP_MAX_DATAGRAM_SIZE ? SLPv1_FLAG_OVERFLOW : 0);  
    /*dialect*/
    *(result->start + 5) = 0;
    /*language code*/
    memcpy(result->start + 6, message->header.langtag, 2);
    ToUINT16(result->start + 8, message->header.encoding);
    /*xid*/
    ToUINT16(result->start + 10, message->header.xid);

    /*-----------------------------*/
    /* Add rest of the SrvTypeRply */
    /*-----------------------------*/
    result->curpos = result->start + 12;

    /* error code*/
    ToUINT16(result->curpos, errorcode);
    result->curpos += 2;
    
    /* service type count */
    ToUINT16(result->curpos, found);
    result->curpos += 2;

    /* TODO - make sure we don't return generic types */
    for(i=0;i<found;i++)
    {
	typelen = size;
	SLPv1ToEncoding(result->curpos + 2, &typelen,
			message->header.encoding,  
			srvtypearray[i].type,
			srvtypearray[i].typelen);
	ToUINT16(result->curpos, srvtypearray[i].typelen);
	result->curpos += 2 + typelen;
    }

FINISHED:   
    if(srvtypearray) free(srvtypearray);
    *sendbuf = result;
    return errorcode;
}

/*=========================================================================*/
int SLPDv1ProcessMessage(struct sockaddr_in* peeraddr,
			 SLPBuffer recvbuf,
			 SLPBuffer* sendbuf,
			 SLPMessage message,
			 int errorcode)
/* Processes the SLPv1 message and places the results in sendbuf           */
/*                                                                         */
/* recvfd   - the socket the message was received on                       */
/*                                                                         */
/* recvbuf  - message to process                                           */
/*                                                                         */
/* sendbuf  - results of the processed message                             */
/*                                                                         */
/* message  - the parsed message from recvbuf                              */
/*                                                                         */
/* errorcode - error code from parsing the SLPv1 packet                    */
/*                                                                         */
/* Returns  - zero on success SLP_ERROR_PARSE_ERROR or                     */
/*            SLP_ERROR_INTERNAL_ERROR on ENOMEM.                          */
/*=========================================================================*/
{

    if (!G_SlpdProperty.isDA)
    {
	/* SLPv1 messages are handled only by DAs */
	errorcode = SLP_ERROR_VER_NOT_SUPPORTED;
    }

    /* Log trace message */
    SLPDLogTraceMsg("IN",peeraddr,recvbuf);
    
    
    switch (message->header.functionid)
    {
    case SLP_FUNCT_SRVRQST:
        errorcode = v1ProcessSrvRqst(peeraddr, message, sendbuf, errorcode);
        break;

    case SLP_FUNCT_SRVREG:
        errorcode = v1ProcessSrvReg(peeraddr, message, sendbuf, errorcode);
	/* We are a DA so there is no need to register with known DAs */
        break;

    case SLP_FUNCT_SRVDEREG:
        errorcode = v1ProcessSrvDeReg(peeraddr, message, sendbuf, errorcode);
	/* We are a DA so there is no need to deregister with known DAs */
        break;

    case SLP_FUNCT_ATTRRQST:
        errorcode = v1ProcessAttrRqst(peeraddr, message, sendbuf, errorcode);
        break;

    case SLP_FUNCT_SRVTYPERQST:
        errorcode = v1ProcessSrvTypeRqst(peeraddr, message, sendbuf, errorcode);
        break;

    case SLP_FUNCT_DAADVERT:
        errorcode = 0;		/* we are a SLPv2 DA, ignore other DAs */
        break;

    default:
        /* This may happen on a really early parse error or version not */
        /* supported error */

        /* TODO log errorcode here */

        break;
    }

    /* Log traceMsg of message was received and the one that will be sent */
    SLPDLogTraceMsg("OUT", peeraddr, *sendbuf);
    
    SLPMessageFree(message);

    return errorcode;
}                
