/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_v1message.c                                            */
/*                                                                         */
/* Abstract:    Source file specific to the SLPv1 wire protocol messages.  */
/*                                                                         */
/*-------------------------------------------------------------------------*/
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

#include <string.h>
#include <ctype.h>

#include <slp_message.h>

#ifndef WIN32
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#endif

/* Implementation Note:
 * 
 * This file duplicates the parsing routines in slp_message.c. Even
 * though the format of the messages are mostly identical, handling of
 * the character encoding makes it enough of a problem to keep this
 * code independent.
 *
 * Unicode handling is currently a hack. We assume that we have enough
 * space the UTF-8 string in place instead of the unicode string. This
 * assumption is not always correct 16-bit unicode encoding.
 */

/*-------------------------------------------------------------------------*/
int v1ParseHeader(SLPBuffer buffer, SLPHeader* header)
/*                                                                         */
/* Returns  - Zero on success, SLP_ERROR_VER_NOT_SUPPORTED, or             */
/*            SLP_ERROR_PARSE_ERROR.                                       */
/*-------------------------------------------------------------------------*/
{
    header->length      = AsUINT16(buffer->curpos + 2);
    header->flags       = *(buffer->curpos + 4);
    header->encoding    = AsUINT16(buffer->curpos + 8);
    header->extoffset   = 0; /* not used for SLPv1 */
    header->xid         = AsUINT16(buffer->curpos + 10);
    header->langtaglen  = 2;
    header->langtag     = buffer->curpos + 6;

    if(header->functionid > SLP_FUNCT_SRVTYPERQST)
    {
        /* invalid function id */
        return SLP_ERROR_PARSE_ERROR;
    }

    if(header->encoding != SLP_CHAR_ASCII &&
       header->encoding != SLP_CHAR_UTF8 &&
       header->encoding != SLP_CHAR_UNICODE16 &&
       header->encoding != SLP_CHAR_UNICODE32)
    {
	return SLP_ERROR_CHARSET_NOT_UNDERSTOOD;
    }

    if(header->length != buffer->end - buffer->start ||
       header->length < 12)
    {
	/* invalid length 12 bytes is the smallest v1 message*/
	return SLP_ERROR_PARSE_ERROR;
    }

    /* TODO - do something about the other flags */
    if(header->flags & 0x07)
    {
	/* invalid flags */
	return SLP_ERROR_PARSE_ERROR;
    }

    buffer->curpos += 12;

    return 0;
}

/*--------------------------------------------------------------------------*/
int v1ParseUrlEntry(SLPBuffer buffer, SLPHeader* header, SLPUrlEntry* urlentry)
/*                                                                          */
/* Returns  - Zero on success, SLP_ERROR_INTERNAL_ERROR (out of memory) or  */
/*            SLP_ERROR_PARSE_ERROR.                                        */
/*--------------------------------------------------------------------------*/
{
    int result;

    /* make sure that min size is met */
    #if(defined(PARANOID) || defined(DEBUG))
    if(buffer->end - buffer->curpos < 6)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif

    /* no reserved stuff for SLPv1 */
    urlentry->reserved = 0;

    /* parse out lifetime */
    urlentry->lifetime = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;

    /* parse out url */
    urlentry->urllen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(urlentry->urllen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif

    urlentry->url = buffer->curpos;
    buffer->curpos = buffer->curpos + urlentry->urllen;
    
    result = SLPv1AsUTF8(header->encoding, (char *) urlentry->url,
			 &urlentry->urllen); 
    if (result)
    {
	return result;
    }

    /* we don't support auth blocks for SLPv1 - no one uses them anyway */
    urlentry->authcount = 0;
    urlentry->autharray = 0;
        
    return 0;
}


/*--------------------------------------------------------------------------*/
int v1ParseSrvRqst(SLPBuffer buffer, SLPHeader* header, SLPSrvRqst* srvrqst)
/*--------------------------------------------------------------------------*/
{
    char *tmp;
    int result;
    
    /* make sure that min size is met */
    #if(defined(PARANOID) || defined(DEBUG))
    if(buffer->end - buffer->curpos < 10)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    
    /* parse the prlist */
    srvrqst->prlistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(srvrqst->prlistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    srvrqst->prlist = buffer->curpos;
    buffer->curpos = buffer->curpos + srvrqst->prlistlen;

    result = SLPv1AsUTF8(header->encoding, (char *) srvrqst->prlist,
			 &srvrqst->prlistlen);
    if (result)
	return result;

    /* parse the predicate string */
    srvrqst->predicatelen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(srvrqst->predicatelen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    srvrqst->predicate = buffer->curpos;
    buffer->curpos = buffer->curpos + srvrqst->predicatelen;

    result = SLPv1AsUTF8(header->encoding, (char *) srvrqst->predicate,
			 &srvrqst->predicatelen);
    if (result)
    {
	return result;
    }
	
    /* null terminate the predicate */
    * (char *) (srvrqst->predicate + srvrqst->predicatelen) = '\0';	

    /* Now split out the service type */
    srvrqst->srvtype = srvrqst->predicate;
    tmp = strchr(srvrqst->srvtype, '/');
    if (!tmp)
	return SLP_ERROR_PARSE_ERROR;
    *tmp = 0;			/* null terminate service type */
    srvrqst->srvtypelen = tmp - srvrqst->srvtype;
    srvrqst->predicate += srvrqst->srvtypelen + 1;
    srvrqst->predicatelen -= srvrqst->srvtypelen + 1;

    /* Now split out the scope (if any) */
    if (*srvrqst->predicate == '/')
    {
	/* no scope - so set default scope */
	srvrqst->scopelist = "default";
	srvrqst->scopelistlen = 7;
	srvrqst->predicate++;
	srvrqst->predicatelen--;
    }
    else
    {
	srvrqst->scopelist = srvrqst->predicate;
	tmp = strchr(srvrqst->scopelist, '/');
	if (!tmp)
	    return SLP_ERROR_PARSE_ERROR;
	*tmp = 0;		/* null terminate scope list */
	srvrqst->scopelistlen = tmp - srvrqst->scopelist;
	srvrqst->predicate += srvrqst->scopelistlen + 1;
	srvrqst->predicatelen -= srvrqst->scopelistlen + 1;
    }

#if 1				/* TODO */
    srvrqst->predicate = "";
    srvrqst->predicatelen = 1;
    
#endif
    
    /* SLPv1 service requests don't have SPI strings */
    srvrqst->spistrlen = 0;
    srvrqst->spistr = 0;

    return 0;
}


/*--------------------------------------------------------------------------*/
int v1ParseSrvReg(SLPBuffer buffer, SLPHeader* header, SLPSrvReg* srvreg)
/*--------------------------------------------------------------------------*/
{
    char           *tmp;
    int             result;

    /* Parse out the url entry */
    result = v1ParseUrlEntry(buffer, header, &(srvreg->urlentry));
    if(result)
    {
        return result;
    }                       
    
    /* SLPv1 registration requests don't have a separate service type.
       They must be parsed from the url */
    srvreg->srvtype = srvreg->urlentry.url;
    tmp = strstr(srvreg->srvtype, ":/");
    if (!tmp)
	return SLP_ERROR_PARSE_ERROR;
    srvreg->srvtypelen = tmp - srvreg->srvtype;
	
    /* parse the attribute list */
    srvreg->attrlistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(srvreg->attrlistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    srvreg->attrlist = buffer->curpos;
    buffer->curpos = buffer->curpos + srvreg->attrlistlen;

    result = SLPv1AsUTF8(header->encoding, (char *) srvreg->attrlist,
			 &srvreg->attrlistlen);
    if (result)
    {
	return result;
    }
    /* SLPv1 registration requests don't include a scope either. The
       scope is included in the attribute list */
    if ((tmp = strstr(srvreg->attrlist, "SCOPE")) ||
	(tmp = strstr(srvreg->attrlist, "scope")))
    {
	tmp += 5;		/* go past the scope */
	while (*tmp && (isspace((unsigned char)*tmp) || *tmp == '='))
	    tmp++;		/* go past = and white space */
	srvreg->scopelist = tmp;
	while (*tmp && !isspace((unsigned char)*tmp) && *tmp != ')')
	    tmp++;		/* find end of scope */
	srvreg->scopelistlen = tmp - srvreg->scopelist;
    }
    else
    {
	srvreg->scopelist = "default";
	srvreg->scopelistlen = 7;
    }

    /* we don't support auth blocks for SLPv1 - no one uses them anyway */
    srvreg->authcount = 0;
    srvreg->autharray = 0;
    
    return 0;
}


/*--------------------------------------------------------------------------*/
int v1ParseSrvDeReg(SLPBuffer buffer, SLPHeader* header, SLPSrvDeReg* srvdereg)
/*--------------------------------------------------------------------------*/
{
    int            result;

    /* make sure that min size is met */
    #if(defined(PARANOID) || defined(DEBUG))
    if(buffer->end - buffer->curpos < 4)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    

    /* SLPv1 deregistrations do not have a separate scope list */
    srvdereg->scopelistlen = 0;
    srvdereg->scopelist = 0;

    /* parse the url */
    srvdereg->urlentry.reserved = 0; /* not present in SLPv1 */
    srvdereg->urlentry.lifetime = 0; /* not present in SLPv1 */
    srvdereg->urlentry.urllen = AsUINT16(buffer->curpos);
    buffer->curpos += 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(srvdereg->urlentry.urllen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    srvdereg->urlentry.url = buffer->curpos;
    buffer->curpos += srvdereg->urlentry.urllen;
    result = SLPv1AsUTF8(header->encoding, (char *) srvdereg->urlentry.url,
			 &srvdereg->urlentry.urllen);
    if(result)
    {
        return result;
    }
    
    /* parse the tag list */
    srvdereg->taglistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(srvdereg->taglistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    srvdereg->taglist = buffer->curpos;
    buffer->curpos = buffer->curpos + srvdereg->taglistlen;

    result = SLPv1AsUTF8(header->encoding, (char *) srvdereg->taglist,
			 &srvdereg->taglistlen);
    if (result)
	return result;
    return 0;
}


/*--------------------------------------------------------------------------*/
int v1ParseAttrRqst(SLPBuffer buffer, SLPHeader* header, SLPAttrRqst* attrrqst)
/*--------------------------------------------------------------------------*/
{
    int result;
    
    /* make sure that min size is met */
    #if(defined(PARANOID) || defined(DEBUG))
    if(buffer->end - buffer->curpos < 10)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    
    /* parse the prlist */
    attrrqst->prlistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(attrrqst->prlistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    attrrqst->prlist = buffer->curpos;
    buffer->curpos = buffer->curpos + attrrqst->prlistlen;
    
    result = SLPv1AsUTF8(header->encoding, (char *) attrrqst->prlist,
			 &attrrqst->prlistlen);
    if (result)
	return result;

    /* parse the url */
    attrrqst->urllen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(attrrqst->urllen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    attrrqst->url = buffer->curpos;
    buffer->curpos = buffer->curpos + attrrqst->urllen;    

    result = SLPv1AsUTF8(header->encoding, (char *) attrrqst->url,
			 &attrrqst->urllen);
    if (result)
	return result;

    /* parse the scope list */
    attrrqst->scopelistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(attrrqst->scopelistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    if (attrrqst->scopelistlen)
    {
	attrrqst->scopelist = buffer->curpos;
	buffer->curpos += attrrqst->scopelistlen;    
	result = SLPv1AsUTF8(header->encoding, (char *) attrrqst->scopelist,
			     &attrrqst->scopelistlen);
	if (result)
	    return result;
    }
    else
    {
	attrrqst->scopelist = "default";
	attrrqst->scopelistlen = 7;
    }

    /* parse the taglist string */
    attrrqst->taglistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(attrrqst->taglistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    attrrqst->taglist = buffer->curpos;
    buffer->curpos = buffer->curpos + attrrqst->taglistlen;

    result = SLPv1AsUTF8(header->encoding, (char *) attrrqst->taglist,
			 &attrrqst->taglistlen);
    if (result)
	return result;

    /* SLPv1 service requests don't have SPI strings */
    attrrqst->spistrlen = 0;
    attrrqst->spistr = 0;

    return 0;
}


/*--------------------------------------------------------------------------*/
int v1ParseSrvTypeRqst(SLPBuffer buffer, SLPHeader* header,
		       SLPSrvTypeRqst* srvtyperqst) 
/*--------------------------------------------------------------------------*/
{
    int result;
    
    /* make sure that min size is met */
    #if(defined(PARANOID) || defined(DEBUG))
    if(buffer->end - buffer->curpos < 6)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    
    /* parse the prlist */
    srvtyperqst->prlistlen = AsUINT16(buffer->curpos);
    buffer->curpos += 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(srvtyperqst->prlistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    srvtyperqst->prlist = srvtyperqst->prlistlen ? buffer->curpos : 0;
    buffer->curpos += srvtyperqst->prlistlen;
    
    result = SLPv1AsUTF8(header->encoding, (char *) srvtyperqst->prlist,
			 &srvtyperqst->prlistlen);
    if (result)
	return result;

    /* parse the naming authority if present */
    srvtyperqst->namingauthlen = AsUINT16(buffer->curpos);
    buffer->curpos += 2;
    if (!srvtyperqst->namingauthlen || srvtyperqst->namingauthlen == 0xffff)
    {
	srvtyperqst->namingauth = 0;
    } else
    {
        #if(defined(PARANOID) || defined(DEBUG))
        if(srvtyperqst->namingauthlen > buffer->end - buffer->curpos)
        {
            return SLP_ERROR_PARSE_ERROR;
        }
        #endif
	srvtyperqst->namingauth = buffer->curpos;
	buffer->curpos += srvtyperqst->namingauthlen;
	result = SLPv1AsUTF8(header->encoding,
			     (char *) srvtyperqst->namingauth,
			     &srvtyperqst->namingauthlen);
	if (result)
	    return result;
    }
    
    /* parse the scope list */
    srvtyperqst->scopelistlen = AsUINT16(buffer->curpos);
    buffer->curpos += 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(srvtyperqst->scopelistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    if (srvtyperqst->scopelistlen)
    {
	srvtyperqst->scopelist = buffer->curpos;
	buffer->curpos += srvtyperqst->scopelistlen;    
	result = SLPv1AsUTF8(header->encoding,
			     (char *) srvtyperqst->scopelist,
			     &srvtyperqst->scopelistlen);
	if (result)
	    return result;
    }
    else
    {
	srvtyperqst->scopelist = "default";
	srvtyperqst->scopelistlen = 7;
    }

    return 0;
}

/*=========================================================================*/
int SLPv1MessageParseBuffer(SLPBuffer buffer, SLPHeader* header,
			    SLPMessage message) 
/* Initializes a SLPv1 message descriptor by parsing the specified buffer. */
/*                                                                         */
/* buffer   - (IN) pointer the SLPBuffer to parse                          */
/*                                                                         */
/* header   - (IN) pointer to the SLPv1 Header                             */
/*                                                                         */
/* message  - (OUT) set to describe the message from the buffer            */
/*                                                                         */
/* Returns  - Zero on success, SLP_ERROR_PARSE_ERROR, or                   */
/*            SLP_ERROR_INTERNAL_ERROR if out of memory.  SLPMessage is    */
/*            invalid return is not successful.                            */
/*                                                                         */
/* WARNING  - If successful, pointers in the SLPMessage reference memory in*/ 
/*            the parsed SLPBuffer.  If SLPBufferFree() is called then the */
/*            pointers in SLPMessage will be invalidated.                  */
/*=========================================================================*/
{
    int result;
    
    /* switch on the function id to parse the body */
    switch(message->header.functionid)
    {
    case SLP_FUNCT_SRVRQST:
	result = v1ParseSrvRqst(buffer, header, &(message->body.srvrqst));
	break;

    case SLP_FUNCT_SRVREG:
	result = v1ParseSrvReg(buffer, header, &(message->body.srvreg));
	break;
            
    case SLP_FUNCT_SRVDEREG:
	result = v1ParseSrvDeReg(buffer, header, &(message->body.srvdereg));
	break;
    
    case SLP_FUNCT_ATTRRQST:
	result = v1ParseAttrRqst(buffer, header, &(message->body.attrrqst));
	break;
    
    case SLP_FUNCT_SRVTYPERQST:
	result = v1ParseSrvTypeRqst(buffer, header,
				    &(message->body.srvtyperqst));
	break;
    
    default:
	result = SLP_ERROR_MESSAGE_NOT_SUPPORTED;
    }

    return result;
}

