/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_msg.h                                                  */
/*                                                                         */
/* Abstract:    Header file that defines structures and constants that are */
/*              specific to the SLP wire protocol messages.                */
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

#include <slp_message.h>

#ifndef WIN32
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#endif

/*-------------------------------------------------------------------------*/
int ParseHeader(SLPBuffer buffer, SLPHeader* header)
/*                                                                         */
/* Returns  - Zero on success, SLP_ERROR_VER_NOT_SUPPORTED, or             */
/*            SLP_ERROR_PARSE_ERROR.                                       */
/*-------------------------------------------------------------------------*/
{
    header->version     = *(buffer->curpos);
    header->functionid  = *(buffer->curpos + 1);
    header->length      = AsUINT24(buffer->curpos + 2);
    header->flags       = AsUINT16(buffer->curpos + 5);
    header->extoffset   = AsUINT24(buffer->curpos + 7);
    header->xid         = AsUINT16(buffer->curpos + 10);
    header->langtaglen  = AsUINT16(buffer->curpos + 12);
    header->langtag     = buffer->curpos + 14;

    if(header->version != 2)
    {
        return SLP_ERROR_VER_NOT_SUPPORTED;
    }

    if(header->functionid > SLP_FUNCT_SAADVERT)
    {
        /* invalid function id */
        return SLP_ERROR_PARSE_ERROR;
    }

    if(header->length != buffer->end - buffer->start ||
       header->length < 18)
    {
        /* invalid length 18 bytes is the smallest v2 message*/
        return SLP_ERROR_PARSE_ERROR;
    }

    if(header->flags & 0x1fff)
    {
        /* invalid flags */
        return SLP_ERROR_PARSE_ERROR;
    }

    buffer->curpos = buffer->curpos + header->langtaglen + 14;

    return 0;
}

/*--------------------------------------------------------------------------*/
int ParseAuthBlock(SLPBuffer buffer, SLPAuthBlock* authblock)
/* Returns  - Zero on success, SLP_ERROR_INTERNAL_ERROR (out of memory) or  */
/*            SLP_ERROR_PARSE_ERROR.                                        */
/*--------------------------------------------------------------------------*/
{
    /* make sure that min size is met */
    #if(defined(PARANOID) || defined(DEBUG))
    if(buffer->end - buffer->curpos < 10)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    
    authblock->bsd          = AsUINT16(buffer->curpos);
    authblock->length       = AsUINT16(buffer->curpos + 2);
    
    #if(defined(PARANOID) || defined(DEBUG))
    if(authblock->length > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif                           
    
    authblock->timestamp    = AsUINT32(buffer->curpos + 4);
    authblock->spistrlen    = AsUINT16(buffer->curpos + 8);
    authblock->spistr       = buffer->curpos + 10;
    
    #if(defined(PARANOID) || defined(DEBUG))
    if(authblock->spistrlen > buffer->end - buffer->curpos + 10)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif 
    
    authblock->authstruct   = buffer->curpos + authblock->spistrlen + 10;
    
    buffer->curpos = buffer->curpos + authblock->length;
    
    return 0;
}

/*--------------------------------------------------------------------------*/
int ParseUrlEntry(SLPBuffer buffer, SLPUrlEntry* urlentry)
/*                                                                          */
/* Returns  - Zero on success, SLP_ERROR_INTERNAL_ERROR (out of memory) or  */
/*            SLP_ERROR_PARSE_ERROR.                                        */
/*--------------------------------------------------------------------------*/
{
    int             result;
    int             i;
    
    /* make sure that min size is met */
    #if(defined(PARANOID) || defined(DEBUG))
    if(buffer->end - buffer->curpos < 6)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif

    /* parse out reserved */
    urlentry->reserved = *(buffer->curpos);
    buffer->curpos = buffer->curpos + 1;

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

    /* parse out auth block count */
    urlentry->authcount = *(buffer->curpos);
    buffer->curpos = buffer->curpos + 1;

    /* parse out the auth block (if any) */
    if(urlentry->authcount)
    {
        urlentry->autharray = (SLPAuthBlock*)malloc(sizeof(SLPAuthBlock) * urlentry->authcount);
        if(urlentry->autharray == 0)
        {
            return SLP_ERROR_INTERNAL_ERROR;
        }
        memset(urlentry->autharray,0,sizeof(SLPAuthBlock) * urlentry->authcount);

        for(i=0;i<urlentry->authcount;i++)
        {
            result = ParseAuthBlock(buffer,&(urlentry->autharray[i]));
            if(result) return result;
        }
    }
        
    return 0;
}


/*--------------------------------------------------------------------------*/
int ParseSrvRqst(SLPBuffer buffer, SLPSrvRqst* srvrqst)
/*--------------------------------------------------------------------------*/
{
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
    

    /* parse the service type */
    srvrqst->srvtypelen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(srvrqst->srvtypelen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    srvrqst->srvtype = buffer->curpos;
    buffer->curpos = buffer->curpos + srvrqst->srvtypelen;    


    /* parse the scope list */
    srvrqst->scopelistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(srvrqst->scopelistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    srvrqst->scopelist = buffer->curpos;
    buffer->curpos = buffer->curpos + srvrqst->scopelistlen;    


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


    /* parse the slpspi string */
    srvrqst->spistrlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(srvrqst->spistrlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    srvrqst->spistr = buffer->curpos;
    buffer->curpos = buffer->curpos + srvrqst->spistrlen;

    return 0;
}


/*--------------------------------------------------------------------------*/
int ParseSrvRply(SLPBuffer buffer, SLPSrvRply* srvrply)
/*--------------------------------------------------------------------------*/
{
    int             result;
    int             i;
    
    /* make sure that min size is met */
    #if(defined(PARANOID) || defined(DEBUG))
    if(buffer->end - buffer->curpos < 4)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif

    /* parse out the error code */
    srvrply->errorcode = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;

    /* parse out the url entry count */
    srvrply->urlcount = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;

    /* parse out the url entries (if any) */
    if(srvrply->urlcount)
    {
        srvrply->urlarray = (SLPUrlEntry*)malloc(sizeof(SLPUrlEntry) * srvrply->urlcount);
        if(srvrply->urlarray == 0)
        {
           return SLP_ERROR_INTERNAL_ERROR;
        }
        memset(srvrply->urlarray,0,sizeof(SLPUrlEntry) * srvrply->urlcount);
        
        for(i=0;i<srvrply->urlcount;i++)
        {
            result = ParseUrlEntry(buffer,&(srvrply->urlarray[i]));   
            if (result) return result;
        }
    }       

    return 0;
}


/*--------------------------------------------------------------------------*/
int ParseSrvReg(SLPBuffer buffer, SLPSrvReg* srvreg)
/*--------------------------------------------------------------------------*/
{
    int             result;
    int             i;

    /* Parse out the url entry */
    result = ParseUrlEntry(buffer,&(srvreg->urlentry));
    if(result)
    {
        return result;
    }                       
    

    /* parse the service type */
    srvreg->srvtypelen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(srvreg->srvtypelen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    srvreg->srvtype = buffer->curpos;
    buffer->curpos = buffer->curpos + srvreg->srvtypelen;    


    /* parse the scope list */
    srvreg->scopelistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(srvreg->scopelistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    srvreg->scopelist = buffer->curpos;
    buffer->curpos = buffer->curpos + srvreg->scopelistlen;    


    /* parse the attribute list*/
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


    /* parse out attribute auth block count */
    srvreg->authcount = *(buffer->curpos);
    buffer->curpos = buffer->curpos + 1;

    /* parse out the auth block (if any) */
    if(srvreg->authcount)
    {
        srvreg->autharray = (SLPAuthBlock*)malloc(sizeof(SLPAuthBlock) * srvreg->authcount);
        if(srvreg->autharray == 0)
        {
            return SLP_ERROR_INTERNAL_ERROR;
        }
        memset(srvreg->autharray,0,sizeof(SLPAuthBlock) * srvreg->authcount);

        for(i=0;i<srvreg->authcount;i++)
        {
            result = ParseAuthBlock(buffer,&(srvreg->autharray[i]));
            if(result) return result;
        }
    }
    
    return 0;
}


/*--------------------------------------------------------------------------*/
int ParseSrvDeReg(SLPBuffer buffer, SLPSrvDeReg* srvdereg)
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
    

    /* parse the scope list */
    srvdereg->scopelistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(srvdereg->scopelistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    srvdereg->scopelist = buffer->curpos;
    buffer->curpos = buffer->curpos + srvdereg->scopelistlen;

    /* parse the url entry */
    result = ParseUrlEntry(buffer,&(srvdereg->urlentry));
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

    return 0;
}


/*--------------------------------------------------------------------------*/
int ParseSrvAck(SLPBuffer buffer, SLPSrvAck* srvack)
/*--------------------------------------------------------------------------*/
{
    srvack->errorcode = AsUINT16(buffer->curpos);
    return 0;
}


/*--------------------------------------------------------------------------*/
int ParseAttrRqst(SLPBuffer buffer, SLPAttrRqst* attrrqst)
/*--------------------------------------------------------------------------*/
{
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


    /* parse the scope list */
    attrrqst->scopelistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(attrrqst->scopelistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    attrrqst->scopelist = buffer->curpos;
    buffer->curpos = buffer->curpos + attrrqst->scopelistlen;    


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


    /* parse the slpspi string */
    attrrqst->spistrlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(attrrqst->spistrlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    attrrqst->spistr = buffer->curpos;
    buffer->curpos = buffer->curpos + attrrqst->spistrlen;

    return 0;
}


/*--------------------------------------------------------------------------*/
int ParseAttrRply(SLPBuffer buffer, SLPAttrRply* attrrply)
/*--------------------------------------------------------------------------*/
{
    int             result;
    int             i;
    
    /* make sure that min size is met */
    #if(defined(PARANOID) || defined(DEBUG))
    if(buffer->end - buffer->curpos < 4)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif

    /* parse out the error code */
    attrrply->errorcode = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;

    /* parse out the attrlist */
    attrrply->attrlistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(attrrply->attrlistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    attrrply->attrlist = buffer->curpos;
    buffer->curpos = buffer->curpos + attrrply->attrlistlen;

    /* parse out auth block count */
    attrrply->authcount = *(buffer->curpos);
    buffer->curpos = buffer->curpos + 1;

    /* parse out the auth block (if any) */
    if(attrrply->authcount)
    {
        attrrply->autharray = (SLPAuthBlock*)malloc(sizeof(SLPAuthBlock) * attrrply->authcount);
        if(attrrply->autharray == 0)
        {
            return SLP_ERROR_INTERNAL_ERROR;
        }
        memset(attrrply->autharray,0,sizeof(SLPAuthBlock) * attrrply->authcount);

        for(i=0;i<attrrply->authcount;i++)
        {
            result = ParseAuthBlock(buffer,&(attrrply->autharray[i]));
            if(result) return result;
        }
    }

    return 0;
}

/*-------------------------------------------------------------------------*/
int ParseDAAdvert(SLPBuffer buffer, SLPDAAdvert* daadvert)
/*-------------------------------------------------------------------------*/
{
    int             result;
    int             i;
    
    /* make sure that min size is met */
    #if(defined(PARANOID) || defined(DEBUG))
    if(buffer->end - buffer->curpos < 4)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif

    /* parse out the error code */
    daadvert->errorcode = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;

    /* parse out the bootstamp */
    daadvert->bootstamp = AsUINT32(buffer->curpos);
    buffer->curpos = buffer->curpos + 4;

    /* parse out the url */
    daadvert->urllen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(daadvert->urllen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    daadvert->url = buffer->curpos;
    buffer->curpos = buffer->curpos + daadvert->urllen;

    /* parse the scope list */
    daadvert->scopelistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(daadvert->scopelistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    daadvert->scopelist = buffer->curpos;
    buffer->curpos = buffer->curpos + daadvert->scopelistlen;  

    /* parse the attr list */
    daadvert->attrlistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(daadvert->attrlistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    daadvert->attrlist = buffer->curpos;
    buffer->curpos = buffer->curpos + daadvert->attrlistlen;

    /* parse the SPI list */
    daadvert->spilistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    #if(defined(PARANOID) || defined(DEBUG))
    if(daadvert->spilistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    daadvert->spilist = buffer->curpos;
    buffer->curpos = buffer->curpos + daadvert->spilistlen;


     /* parse out auth block count */
    daadvert->authcount = *(buffer->curpos);
    buffer->curpos = buffer->curpos + 1;

    /* parse out the auth block (if any) */
    if(daadvert->authcount)
    {
        daadvert->autharray = (SLPAuthBlock*)malloc(sizeof(SLPAuthBlock) * daadvert->authcount);
        if(daadvert->autharray == 0)
        {
            return SLP_ERROR_INTERNAL_ERROR;
        }
        memset(daadvert->autharray,0,sizeof(SLPAuthBlock) * daadvert->authcount);

        for(i=0;i<daadvert->authcount;i++)
        {
            result = ParseAuthBlock(buffer,&(daadvert->autharray[i]));
            if(result) return result;
        }
    }       

    return 0;
}

/*--------------------------------------------------------------------------*/
int ParseSrvTypeRqst(SLPBuffer buffer, SLPSrvTypeRqst* srvtyperqst)
/*--------------------------------------------------------------------------*/
{
    /* make sure that min size is met */
    #if(defined(PARANOID) || defined(DEBUG))
    if(buffer->end - buffer->curpos < 10)
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
    srvtyperqst->scopelist = buffer->curpos;
    buffer->curpos += srvtyperqst->scopelistlen;    

    return 0;
}


/*--------------------------------------------------------------------------*/
int ParseSrvTypeRply(SLPBuffer buffer, SLPSrvTypeRply* srvtyperply)
/*--------------------------------------------------------------------------*/
{
    /* make sure that min size is met */
    #if(defined(PARANOID) || defined(DEBUG))
    if(buffer->end - buffer->curpos < 4)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif

    /* parse out the error code */
    srvtyperply->errorcode = AsUINT16(buffer->curpos);
    buffer->curpos += 2;

    /* parse out the error srvtype-list length */
    srvtyperply->srvtypelistlen = AsUINT16(buffer->curpos);
    buffer->curpos += 2;

    #if(defined(PARANOID) || defined(DEBUG))
    if(srvtyperply->srvtypelistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    #endif
    srvtyperply->srvtypelist = buffer->curpos;
    
    return 0;
}

/*-------------------------------------------------------------------------*/
void SLPMessageFreeInternals(SLPMessage message)
/*-------------------------------------------------------------------------*/
{
    int i;

    switch(message->header.functionid)
    {   
    case SLP_FUNCT_SRVRPLY:
        if(message->body.srvrply.urlarray)
        {
            for(i=0;i<message->body.srvrply.urlcount;i++)
            {
                if(message->body.srvrply.urlarray[i].autharray)
                {
                    free(message->body.srvrply.urlarray[i].autharray);
                    message->body.srvrply.urlarray[i].autharray = 0;
                }
            }

            free(message->body.srvrply.urlarray);
            message->body.srvrply.urlarray = 0;
        }
        break;
    
    case SLP_FUNCT_SRVREG:
        if(message->body.srvreg.urlentry.autharray)
        {
            free(message->body.srvreg.urlentry.autharray);
            message->body.srvreg.urlentry.autharray = 0;
        }                                        
        if(message->body.srvreg.autharray)
        {
            free(message->body.srvreg.autharray);
            message->body.srvreg.autharray = 0;
        }                                        
        break;
    
      
    case SLP_FUNCT_SRVDEREG:
        if(message->body.srvdereg.urlentry.autharray)
        {
            free(message->body.srvdereg.urlentry.autharray);
            message->body.srvdereg.urlentry.autharray = 0;
        }
        break;
          
    case SLP_FUNCT_ATTRRPLY:
        if(message->body.attrrply.autharray)
        {
            free(message->body.attrrply.autharray);
            message->body.attrrply.autharray = 0;
        }
        break;
        
    
    case SLP_FUNCT_DAADVERT:
        if(message->body.daadvert.autharray)
        {
            free(message->body.daadvert.autharray);
            message->body.daadvert.autharray = 0;
        }
        break; 

    case SLP_FUNCT_ATTRRQST:
    case SLP_FUNCT_SRVACK:
    case SLP_FUNCT_SRVRQST:
    case SLP_FUNCT_SRVTYPERQST:
    case SLP_FUNCT_SRVTYPERPLY:
    default:
        /* don't do anything */
        break;
    }

}

/*=========================================================================*/
SLPMessage SLPMessageAlloc()
/* Allocates memory for a SLP message descriptor                           */
/*                                                                         */
/* Returns   - A newly allocated SLPMessage pointer of NULL on ENOMEM      */
/*=========================================================================*/
{
    SLPMessage result = (SLPMessage)malloc(sizeof(struct _SLPMessage));
    if(result)
    {
        memset(result,0,sizeof(struct _SLPMessage));
    }
    
    return result;
}


/*=========================================================================*/
SLPMessage SLPMessageRealloc(SLPMessage msg)
/* Reallocates memory for a SLP message descriptor                         */
/*                                                                         */
/* Returns   - A newly allocated SLPMessage pointer of NULL on ENOMEM      */
/*=========================================================================*/
{
    if(msg == 0)
    {
        msg = SLPMessageAlloc();
        if(msg == 0)
        {
            return 0;
        }
    }
    else
    {
        SLPMessageFreeInternals(msg);
    }

    return msg;
}


/*=========================================================================*/
void SLPMessageFree(SLPMessage message)
/* Frees memory that might have been allocated by the SLPMessage for       */
/* UrlEntryLists or AuthBlockLists.                                        */
/*                                                                         */
/* message  - (IN) the SLPMessage to free                                  */
/*=========================================================================*/
{
    if(message)
    {
        SLPMessageFreeInternals(message);
        free(message);
    }
}


/*=========================================================================*/
int SLPMessageParseBuffer(SLPBuffer buffer, SLPMessage message)
/* Initializes a message descriptor by parsing the specified buffer.       */
/*                                                                         */
/* buffer   - (IN) pointer the SLPBuffer to parse                          */
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

    /* Get ready to parse */
    SLPMessageFreeInternals(message);
    buffer->curpos = buffer->start;

    /* parse the header first */
    
    result = ParseHeader(buffer,&(message->header));
    if(result == 0)
    {
        /* switch on the function id to parse the body */
        switch(message->header.functionid)
        {
        case SLP_FUNCT_SRVRQST:
            result = ParseSrvRqst(buffer,&(message->body.srvrqst));
            break;

        case SLP_FUNCT_SRVRPLY:
            result = ParseSrvRply(buffer,&(message->body.srvrply));
            break;
    
        case SLP_FUNCT_SRVREG:
            result = ParseSrvReg(buffer,&(message->body.srvreg));
            break;
            
        case SLP_FUNCT_SRVDEREG:
            result = ParseSrvDeReg(buffer,&(message->body.srvdereg));
            break;
    
        case SLP_FUNCT_SRVACK:
            result = ParseSrvAck(buffer,&(message->body.srvack));
            break;
    
        case SLP_FUNCT_ATTRRQST:
            result = ParseAttrRqst(buffer,&(message->body.attrrqst));
            break;
    
        case SLP_FUNCT_ATTRRPLY:
            result = ParseAttrRply(buffer,&(message->body.attrrply));
            break;

        case SLP_FUNCT_DAADVERT:
            result = ParseDAAdvert(buffer,&(message->body.daadvert));
            break;
    
        case SLP_FUNCT_SRVTYPERQST:
            result = ParseSrvTypeRqst(buffer,&(message->body.srvtyperqst));
            break;
    
        case SLP_FUNCT_SRVTYPERPLY:
            result = ParseSrvTypeRply(buffer,&(message->body.srvtyperply));
            break;
#if 0    
        case SLP_FUNCT_SAADVERT:
            result = ParseSAAdvert(buffer,&(message->body.saadvert));
            break;
#endif
        default:
            result = SLP_ERROR_MESSAGE_NOT_SUPPORTED;
        }
    }

    return result;
}

#if !defined(i386)
/* Functions used to parse buffers                                         */
/*-------------------------------------------------------------------------*/
unsigned short AsUINT16(const char *charptr)
/*-------------------------------------------------------------------------*/
{
    unsigned char *ucp = (unsigned char *) charptr;
    return (ucp[0] << 8) | ucp[1];
}

/*-------------------------------------------------------------------------*/
unsigned int AsUINT24(const char *charptr)
/*-------------------------------------------------------------------------*/
{
    unsigned char *ucp = (unsigned char *) charptr;
    return (ucp[0] << 16) | (ucp[1] << 8) |  ucp[2];
}

/*-------------------------------------------------------------------------*/
unsigned int AsUINT32(const char *charptr)
/*-------------------------------------------------------------------------*/
{
    unsigned char *ucp = (unsigned char *) charptr;
    return (ucp[0] << 24) | (ucp[1] << 16) | (ucp[2] << 8) | ucp[3]; 
}
/*=========================================================================*/

/*=========================================================================*/
/* Functions used to set buffers                                           */
/*-------------------------------------------------------------------------*/
void ToUINT16(char *charptr, unsigned int val)
/*-------------------------------------------------------------------------*/
{
    charptr[0] = (val >> 8) & 0xff;
    charptr[1] = val & 0xff;
}

/*-------------------------------------------------------------------------*/
void ToUINT24(char *charptr, unsigned int val)
/*-------------------------------------------------------------------------*/
{
    charptr[0] = (val >> 16) & 0xff;
    charptr[1] = (val >> 8) & 0xff;
    charptr[2] = val & 0xff;
}

/*-------------------------------------------------------------------------*/
void ToUINT32(char *charptr, unsigned int val)
/*-------------------------------------------------------------------------*/
{
    charptr[0] = (val >> 24) & 0xff;
    charptr[1] = (val >> 16) & 0xff;
    charptr[2] = (val >> 8) & 0xff;
    charptr[3] = val & 0xff;
}
#endif
