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

/** Functions common to both versions of SLP wire protocol.
 *
 * @file       slp_message.c
 * @author     John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#include "slp_message.h"
#include "slp_xmalloc.h"

#ifndef _WIN32
# include <sys/types.h>
# include <netinet/in.h>
#endif

#if defined(ENABLE_SLPv1)
# include <slp_v1message.h>
#endif

/** Extract a 16-bit big-endian buffer value into a native 16-bit word.
 *
 * @param[in,out] cpp - The address of a pointer from which to extract.
 *
 * @return A 16-bit unsigned value in native format; the buffer pointer
 *    is moved ahead by 2 bytes on return.
 */
unsigned short AsUINT16(const char *charptr)
{
    unsigned char *ucp = (unsigned char *) charptr;
    return(ucp[0] << 8) | ucp[1];
}

/** Extract a 24-bit big-endian buffer value into a native 32-bit word.
 *
 * @param[in,out] cpp - The address of a pointer from which to extract.
 *
 * @return A 32-bit unsigned value in native format; the buffer pointer 
 *    is moved ahead by 3 bytes on return.
 */
unsigned int AsUINT24(const char *charptr)
{
    unsigned char *ucp = (unsigned char *) charptr;
    return(ucp[0] << 16) | (ucp[1] << 8) |  ucp[2];
}

/** Extract a 32-bit big-endian buffer value into a native 32-bit word.
 *
 * @param[in,out] cpp - The address of a pointer from which to extract.
 *
 * @return A 32-bit unsigned value in native format; the buffer pointer
 *    is moved ahead by 4 bytes on return.
 */
unsigned int AsUINT32(const char *charptr)
{
    unsigned char *ucp = (unsigned char *) charptr;
    return(ucp[0] << 24) | (ucp[1] << 16) | (ucp[2] << 8) | ucp[3]; 
}

/** Extract a string buffer address into a character pointer.
 *
 * Note that this routine doesn't actually copy the string. It only casts
 * the buffer pointer to a character pointer and moves the value at @p cpp 
 * ahead by @p len bytes.
 *
 * @param[in,out] cpp - The address of a pointer from which to extract.
 * @param[in] len - The length of the string to extract.
 *
 * @return A pointer to the first character at the address pointed to by 
 *    @p cppstring pointer; the buffer pointer is moved ahead by @p len bytes
 *    on return. If @p len is zero, returns NULL.
 */
char * GetStrPtr(const uint8_t ** cpp, size_t len)
{
   char * sp = len? (char *)*cpp: 0;
   *cpp += len;
   return sp;
}

/** Insert a 16-bit native word into a buffer in big-endian format.
 *
 * @param[in,out] cpp - The address of a pointer where @p val is written.
 * @param[in]     val - A 16-bit native value to be inserted into @p cpp.
 *
 * @note The buffer address is moved ahead by 2 bytes on return.
 */
void ToUINT16(char *charptr, unsigned int val)
{
    charptr[0] = (val >> 8) & 0xff;
    charptr[1] = val & 0xff;
}

/** Insert a 24-bit native word into a buffer in big-endian format.
 *
 * @param[in,out] cpp - The address of a pointer where @p val is written.
 * @param[in]     val - A 24-bit native value to be inserted into @p cpp.
 *
 * @note The buffer address is moved ahead by 3 bytes on return.
 */
void ToUINT24(char *charptr, unsigned int val)
{
    charptr[0] = (val >> 16) & 0xff;
    charptr[1] = (val >> 8) & 0xff;
    charptr[2] = val & 0xff;
}

/** Insert a 32-bit native word into a buffer in big-endian format.
 *
 * @param[in,out] cpp - The address of a pointer where @p val is written.
 * @param[in]     val - A 32-bit native value to be inserted into @p cpp.
 *
 * @note The buffer address is moved ahead by 4 bytes on return.
 */
void ToUINT32(char *charptr, unsigned int val)
{
    charptr[0] = (val >> 24) & 0xff;
    charptr[1] = (val >> 16) & 0xff;
    charptr[2] = (val >> 8) & 0xff;
    charptr[3] = val & 0xff;
}

/** Free internal buffers in an SLP message.
 *
 * @param[in] message - The message to be freed.
 */
void SLPMessageFreeInternals(SLPMessage message)
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
                    xfree(message->body.srvrply.urlarray[i].autharray);
                    message->body.srvrply.urlarray[i].autharray = 0;
                }
            }

            xfree(message->body.srvrply.urlarray);
            message->body.srvrply.urlarray = 0;
        }
        break;

    case SLP_FUNCT_SRVREG:
        if(message->body.srvreg.urlentry.autharray)
        {
            xfree(message->body.srvreg.urlentry.autharray);
            message->body.srvreg.urlentry.autharray = 0;
        }
        if(message->body.srvreg.autharray)
        {
            xfree(message->body.srvreg.autharray);
            message->body.srvreg.autharray = 0;
        }
        break;


    case SLP_FUNCT_SRVDEREG:
        if(message->body.srvdereg.urlentry.autharray)
        {
            xfree(message->body.srvdereg.urlentry.autharray);
            message->body.srvdereg.urlentry.autharray = 0;
        }
        break;

    case SLP_FUNCT_ATTRRPLY:
        if(message->body.attrrply.autharray)
        {
            xfree(message->body.attrrply.autharray);
            message->body.attrrply.autharray = 0;
        }
        break;


    case SLP_FUNCT_DAADVERT:
        if(message->body.daadvert.autharray)
        {
            xfree(message->body.daadvert.autharray);
            message->body.daadvert.autharray = 0;
        }
        break; 

    case SLP_FUNCT_SAADVERT:
        if(message->body.saadvert.autharray)
        {
            xfree(message->body.saadvert.autharray);
            message->body.saadvert.autharray = 0;
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

/** Allocate memory for an SLP message descriptor.
 *
 * @return A new SLP message object, or NULL if out of memory.
 */
SLPMessage SLPMessageAlloc()
{
    SLPMessage result = (SLPMessage)xmalloc(sizeof(struct _SLPMessage));
    if(result)
    {
        memset(result,0,sizeof(struct _SLPMessage));
    }

    return result;
}

/** Reallocate memory for an SLP message descriptor.
 *
 * @param[in] msg - The message descriptor to be reallocated.
 *
 * @return A resized version of @p msg, or NULL if out of memory.
 */
SLPMessage SLPMessageRealloc(SLPMessage msg)
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

/** Frees memory associated with an SLP message descriptor.
 *
 * @param[in] message - The SLP message descriptor to be freed.
 */
void SLPMessageFree(SLPMessage message)
{
    if(message)
    {
        SLPMessageFreeInternals(message);
        xfree(message);
    }
}

/** Switch on version field to parse v1 or v2 header.
 *
 * @param[in] buffer - The buffer to be parsed.
 * @param[out] header - The address of a message header to be filled.
 *
 * @return Zero on success, or SLP_ERROR_VER_NOT_SUPPORTED.
 */
int SLPMessageParseHeader(SLPBuffer buffer, SLPHeader* header)
{
    header->version     = *(buffer->curpos);
    header->functionid  = *(buffer->curpos + 1);
   
    if(header->version != 2)
    {
        return SLP_ERROR_VER_NOT_SUPPORTED;
    }
    header->length      = AsUINT24(buffer->curpos + 2);
    header->flags       = AsUINT16(buffer->curpos + 5);
    header->encoding    = 0; /* not used for SLPv2 */
    header->extoffset   = AsUINT24(buffer->curpos + 7);
    header->xid         = AsUINT16(buffer->curpos + 10);
    header->langtaglen  = AsUINT16(buffer->curpos + 12);
    header->langtag     = buffer->curpos + 14;

    /* check for invalid function id */
    if(header->functionid > SLP_FUNCT_SAADVERT)
    {
        return SLP_ERROR_PARSE_ERROR;
    }

    /* check for invalid length 18 bytes is the smallest v2 message*/
    if(header->length != buffer->end - buffer->start ||
       header->length < 18)
    {
        return SLP_ERROR_PARSE_ERROR;
    }

    /* check for invalid flags */
    if(header->flags & 0x1fff)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    buffer->curpos = buffer->curpos + header->langtaglen + 14;

    /* check for invalid langtaglen */
    if((void*)(header->langtag + header->langtaglen) > (void*)buffer->end)
    {
        return SLP_ERROR_PARSE_ERROR;
    }

    /* check for invalid ext offset */
    if(buffer->start + header->extoffset > buffer->end)
    {
        return SLP_ERROR_PARSE_ERROR;
    }

    return 0;
}

/** Switch on version field to parse v1 or v2 message.
 *
 * This routine provides a common location between SLP v1 and v2
 * messages to perform operations that are common to both message
 * types. These include copying remote and local bindings into the
 * message buffer.
 *
 * @param[in] peeraddr - Remote address binding to store in @p message.
 * @param[in] localaddr - Local address binding to store in @p message.
 * @param[in] buffer - The buffer to be parsed.
 * @param[in] message - The message into which @p buffer should be parsed.
 *
 * @return Zero on success, SLP_ERROR_PARSE_ERROR, or
 *    SLP_ERROR_INTERNAL_ERROR if out of memory.
 *
 * @remarks On success, pointers in the SLPMessage reference memory in
 *    the parsed SLPBuffer. If SLPBufferFree is called then the pointers
 *    in @p message will be invalidated.
 */
int SLPMessageParseBuffer(struct sockaddr_storage *peerinfo,
                          struct sockaddr_storage *localaddr,
                          SLPBuffer buffer, 
                          SLPMessage message)
{
    int result;

    /* Copy in the address info */
    if (peerinfo != NULL)
        memcpy(&message->peer,peerinfo,sizeof(message->peer));
    if (localaddr != NULL)
        memcpy(&message->localaddr, localaddr, sizeof(message->localaddr));

    /* Get ready to parse */
    SLPMessageFreeInternals(message);
    buffer->curpos = buffer->start;

    /* parse the header first */
    result = SLPMessageParseHeader(buffer,&(message->header));
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

        case SLP_FUNCT_SAADVERT:
            result = ParseSAAdvert(buffer,&(message->body.saadvert));
            break;
        default:
            result = SLP_ERROR_MESSAGE_NOT_SUPPORTED;
        }
    }

    if(result == 0 && message->header.extoffset)
    {
        result = ParseExtension(buffer,message);
    }

    return result;
}

int ParseAuthBlock(SLPBuffer buffer, SLPAuthBlock* authblock)
{
    /* make sure that min size is met */
    if(buffer->end - buffer->curpos < 10)
    {
        return SLP_ERROR_PARSE_ERROR;
    }

    authblock->opaque = buffer->curpos;
    
    authblock->bsd          = AsUINT16(buffer->curpos);
    authblock->length       = AsUINT16(buffer->curpos + 2);
    
    if(authblock->length > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }

    authblock->timestamp    = AsUINT32(buffer->curpos + 4);
    authblock->spistrlen    = AsUINT16(buffer->curpos + 8);
    authblock->spistr       = buffer->curpos + 10;

    if(authblock->spistrlen > buffer->end - buffer->curpos + 10)
    {
        return SLP_ERROR_PARSE_ERROR;
    }

    authblock->authstruct   = buffer->curpos + authblock->spistrlen + 10;
    
    authblock->opaquelen = authblock->length;

    buffer->curpos = buffer->curpos + authblock->length;

    return 0;
}

int ParseUrlEntry(SLPBuffer buffer, SLPUrlEntry* urlentry)
{
    int             result;
    int             i;

    /* make sure that min size is met */
    if(buffer->end - buffer->curpos < 6)
    {
        return SLP_ERROR_PARSE_ERROR;
    }

    urlentry->opaque = buffer->curpos;

    /* parse out reserved */
    urlentry->reserved = *(buffer->curpos);
    buffer->curpos = buffer->curpos + 1;

    /* parse out lifetime */
    urlentry->lifetime = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;

    /* parse out url */
    urlentry->urllen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(urlentry->urllen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }

    urlentry->url = buffer->curpos;
    buffer->curpos = buffer->curpos + urlentry->urllen;

    /* parse out auth block count */
    urlentry->authcount = *(buffer->curpos);
    buffer->curpos = buffer->curpos + 1;

    /* parse out the auth block (if any) */
    if(urlentry->authcount)
    {
        urlentry->autharray = (SLPAuthBlock*)xmalloc(sizeof(SLPAuthBlock) * urlentry->authcount);
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

    urlentry->opaquelen = (char*)buffer->curpos - urlentry->opaque;

    return 0;
}

int ParseSrvRqst(SLPBuffer buffer, SLPSrvRqst* srvrqst)
{
    /* make sure that min size is met */
    if(buffer->end - buffer->curpos < 10)
    {
        return SLP_ERROR_PARSE_ERROR;
    }

    /* parse the prlist */
    srvrqst->prlistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(srvrqst->prlistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    srvrqst->prlist = buffer->curpos;
    buffer->curpos = buffer->curpos + srvrqst->prlistlen;


    /* parse the service type */
    srvrqst->srvtypelen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(srvrqst->srvtypelen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    srvrqst->srvtype = buffer->curpos;
    buffer->curpos = buffer->curpos + srvrqst->srvtypelen;    


    /* parse the scope list */
    srvrqst->scopelistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(srvrqst->scopelistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    srvrqst->scopelist = buffer->curpos;
    buffer->curpos = buffer->curpos + srvrqst->scopelistlen;    


    /* parse the predicate string */
    srvrqst->predicatever = 2;  /* SLPv2 predicate (LDAPv3) */
    srvrqst->predicatelen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(srvrqst->predicatelen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    srvrqst->predicate = buffer->curpos;
    buffer->curpos = buffer->curpos + srvrqst->predicatelen;


    /* parse the slpspi string */
    srvrqst->spistrlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(srvrqst->spistrlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    srvrqst->spistr = buffer->curpos;
    buffer->curpos = buffer->curpos + srvrqst->spistrlen;

    return 0;
}

int ParseSrvRply(SLPBuffer buffer, SLPSrvRply* srvrply)
{
    int             result;
    int             i;

    /* make sure that min size is met */
    if(buffer->end - buffer->curpos < 4)
    {
        return SLP_ERROR_PARSE_ERROR;
    }

    /* parse out the error code */
    srvrply->errorcode = AsUINT16(buffer->curpos);
    if(srvrply->errorcode)
    {
        /* We better not trust the rest of the packet */
        memset( srvrply, 0, sizeof(SLPSrvRply)); 
        srvrply->errorcode = AsUINT16(buffer->curpos);
        return 0;
    }
    buffer->curpos = buffer->curpos + 2;

    /* parse out the url entry count */
    srvrply->urlcount = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;

    /* parse out the url entries (if any) */
    if(srvrply->urlcount)
    {
        srvrply->urlarray = (SLPUrlEntry*)xmalloc(sizeof(SLPUrlEntry) * srvrply->urlcount);
        if(srvrply->urlarray == 0)
        {
            return SLP_ERROR_INTERNAL_ERROR;
        }
        memset(srvrply->urlarray,0,sizeof(SLPUrlEntry) * srvrply->urlcount);

        for(i=0;i<srvrply->urlcount;i++)
        {
            result = ParseUrlEntry(buffer,&(srvrply->urlarray[i]));   
            if(result) return result;
        }
    }
    else
    {
        srvrply->urlarray = 0;
    }

    return 0;
}

int ParseSrvReg(SLPBuffer buffer, SLPSrvReg* srvreg)
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
    if(srvreg->srvtypelen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    srvreg->srvtype = buffer->curpos;
    buffer->curpos = buffer->curpos + srvreg->srvtypelen;    


    /* parse the scope list */
    srvreg->scopelistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(srvreg->scopelistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    srvreg->scopelist = buffer->curpos;
    buffer->curpos = buffer->curpos + srvreg->scopelistlen;    


    /* parse the attribute list*/
    srvreg->attrlistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(srvreg->attrlistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    srvreg->attrlist = buffer->curpos;
    buffer->curpos = buffer->curpos + srvreg->attrlistlen;


    /* parse out attribute auth block count */
    srvreg->authcount = *(buffer->curpos);
    buffer->curpos = buffer->curpos + 1;

    /* parse out the auth block (if any) */
    if(srvreg->authcount)
    {
        srvreg->autharray = (SLPAuthBlock*)xmalloc(sizeof(SLPAuthBlock) * srvreg->authcount);
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

int ParseSrvDeReg(SLPBuffer buffer, SLPSrvDeReg* srvdereg)
{
    int            result;

    /* make sure that min size is met */
    if(buffer->end - buffer->curpos < 4)
    {
        return SLP_ERROR_PARSE_ERROR;
    }


    /* parse the scope list */
    srvdereg->scopelistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(srvdereg->scopelistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
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
    if(srvdereg->taglistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    srvdereg->taglist = buffer->curpos;
    buffer->curpos = buffer->curpos + srvdereg->taglistlen;

    return 0;
}

int ParseSrvAck(SLPBuffer buffer, SLPSrvAck* srvack)
{
    srvack->errorcode = AsUINT16(buffer->curpos);
    return 0;
}

int ParseAttrRqst(SLPBuffer buffer, SLPAttrRqst* attrrqst)
{
    /* make sure that min size is met */
    if(buffer->end - buffer->curpos < 10)
    {
        return SLP_ERROR_PARSE_ERROR;
    }

    /* parse the prlist */
    attrrqst->prlistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(attrrqst->prlistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    attrrqst->prlist = buffer->curpos;
    buffer->curpos = buffer->curpos + attrrqst->prlistlen;

    /* parse the url */
    attrrqst->urllen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(attrrqst->urllen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    attrrqst->url = buffer->curpos;
    buffer->curpos = buffer->curpos + attrrqst->urllen;    


    /* parse the scope list */
    attrrqst->scopelistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(attrrqst->scopelistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    attrrqst->scopelist = buffer->curpos;
    buffer->curpos = buffer->curpos + attrrqst->scopelistlen;    


    /* parse the taglist string */
    attrrqst->taglistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(attrrqst->taglistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    attrrqst->taglist = buffer->curpos;
    buffer->curpos = buffer->curpos + attrrqst->taglistlen;


    /* parse the slpspi string */
    attrrqst->spistrlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(attrrqst->spistrlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    attrrqst->spistr = buffer->curpos;
    buffer->curpos = buffer->curpos + attrrqst->spistrlen;

    return 0;
}

int ParseAttrRply(SLPBuffer buffer, SLPAttrRply* attrrply)
{
    int             result;
    int             i;

    /* make sure that min size is met */
    if(buffer->end - buffer->curpos < 4)
    {
        return SLP_ERROR_PARSE_ERROR;
    }

    /* parse out the error code */
    attrrply->errorcode = AsUINT16(buffer->curpos);
    if(attrrply->errorcode)
    {
        /* We better not trust the rest of the packet */
        memset(attrrply,0,sizeof(SLPAttrRply));
        attrrply->errorcode = AsUINT16(buffer->curpos);
        return 0;
    }
    buffer->curpos = buffer->curpos + 2;

    /* parse out the attrlist */
    attrrply->attrlistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(attrrply->attrlistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    attrrply->attrlist = buffer->curpos;
    buffer->curpos = buffer->curpos + attrrply->attrlistlen;

    /* parse out auth block count */
    attrrply->authcount = *(buffer->curpos);
    buffer->curpos = buffer->curpos + 1;

    /* parse out the auth block (if any) */
    if(attrrply->authcount)
    {
        attrrply->autharray = (SLPAuthBlock*)xmalloc(sizeof(SLPAuthBlock) * attrrply->authcount);
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

int ParseDAAdvert(SLPBuffer buffer, SLPDAAdvert* daadvert)
{
    int             result;
    int             i;

    /* make sure that min size is met */
    if(buffer->end - buffer->curpos < 4)
    {
        return SLP_ERROR_PARSE_ERROR;
    }

    /* parse out the error code */
    daadvert->errorcode = AsUINT16(buffer->curpos);
    if(daadvert->errorcode)
    {
        /* We better not trust the rest of the packet */
        memset(daadvert,0,sizeof(SLPDAAdvert));
        daadvert->errorcode = AsUINT16(buffer->curpos);
        return 0;
    }
    buffer->curpos = buffer->curpos + 2;

    /* parse out the bootstamp */
    daadvert->bootstamp = AsUINT32(buffer->curpos);
    buffer->curpos = buffer->curpos + 4;

    /* parse out the url */
    daadvert->urllen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(daadvert->urllen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    daadvert->url = buffer->curpos;
    buffer->curpos = buffer->curpos + daadvert->urllen;

    /* parse the scope list */
    daadvert->scopelistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(daadvert->scopelistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    daadvert->scopelist = buffer->curpos;
    buffer->curpos = buffer->curpos + daadvert->scopelistlen;  

    /* parse the attr list */
    daadvert->attrlistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(daadvert->attrlistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    daadvert->attrlist = buffer->curpos;
    buffer->curpos = buffer->curpos + daadvert->attrlistlen;

    /* parse the SPI list */
    daadvert->spilistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(daadvert->spilistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    daadvert->spilist = buffer->curpos;
    buffer->curpos = buffer->curpos + daadvert->spilistlen;


    /* parse out auth block count */
    daadvert->authcount = *(buffer->curpos);
    buffer->curpos = buffer->curpos + 1;

    /* parse out the auth block (if any) */
    if(daadvert->authcount)
    {
        daadvert->autharray = (SLPAuthBlock*)xmalloc(sizeof(SLPAuthBlock) * daadvert->authcount);
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

int ParseSAAdvert(SLPBuffer buffer, SLPSAAdvert* saadvert)
{
    int             result;
    int             i;

    /* make sure that min size is met */
    if(buffer->end - buffer->curpos < 4)
    {
        return SLP_ERROR_PARSE_ERROR;
    }

    /* parse out the url */
    saadvert->urllen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(saadvert->urllen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    saadvert->url = buffer->curpos;
    buffer->curpos = buffer->curpos + saadvert->urllen;

    /* parse the scope list */
    saadvert->scopelistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(saadvert->scopelistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    saadvert->scopelist = buffer->curpos;
    buffer->curpos = buffer->curpos + saadvert->scopelistlen;  

    /* parse the attr list */
    saadvert->attrlistlen = AsUINT16(buffer->curpos);
    buffer->curpos = buffer->curpos + 2;
    if(saadvert->attrlistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    saadvert->attrlist = buffer->curpos;
    buffer->curpos = buffer->curpos + saadvert->attrlistlen;

    /* parse out auth block count */
    saadvert->authcount = *(buffer->curpos);
    buffer->curpos = buffer->curpos + 1;

    /* parse out the auth block (if any) */
    if(saadvert->authcount)
    {
        saadvert->autharray = (SLPAuthBlock*)xmalloc(sizeof(SLPAuthBlock) * saadvert->authcount);
        if(saadvert->autharray == 0)
        {
            return SLP_ERROR_INTERNAL_ERROR;
        }
        memset(saadvert->autharray,0,sizeof(SLPAuthBlock) * saadvert->authcount);

        for(i=0;i<saadvert->authcount;i++)
        {
            result = ParseAuthBlock(buffer,&(saadvert->autharray[i]));
            if(result) return result;
        }
    }

    return 0;
}

int ParseSrvTypeRqst(SLPBuffer buffer, SLPSrvTypeRqst* srvtyperqst)
{
    /* make sure that min size is met */
    if(buffer->end - buffer->curpos < 6)
    {
        return SLP_ERROR_PARSE_ERROR;
    }

    /* parse the prlist */
    srvtyperqst->prlistlen = AsUINT16(buffer->curpos);
    buffer->curpos += 2;
    if(srvtyperqst->prlistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    srvtyperqst->prlist = srvtyperqst->prlistlen ? buffer->curpos : 0;
    buffer->curpos += srvtyperqst->prlistlen;

    /* parse the naming authority if present */
    srvtyperqst->namingauthlen = AsUINT16(buffer->curpos);
    buffer->curpos += 2;
    if(!srvtyperqst->namingauthlen || srvtyperqst->namingauthlen == 0xffff)
    {
        srvtyperqst->namingauth = 0;
    }
    else
    {
        if(srvtyperqst->namingauthlen > buffer->end - buffer->curpos)
        {
            return SLP_ERROR_PARSE_ERROR;
        }
        srvtyperqst->namingauth = buffer->curpos;
        buffer->curpos += srvtyperqst->namingauthlen;
    }

    /* parse the scope list */
    srvtyperqst->scopelistlen = AsUINT16(buffer->curpos);
    buffer->curpos += 2;
    if(srvtyperqst->scopelistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    srvtyperqst->scopelist = buffer->curpos;
    buffer->curpos += srvtyperqst->scopelistlen;    

    return 0;
}

int ParseSrvTypeRply(SLPBuffer buffer, SLPSrvTypeRply* srvtyperply)
{
    /* make sure that min size is met */
    if(buffer->end - buffer->curpos < 4)
    {
        return SLP_ERROR_PARSE_ERROR;
    }

    /* parse out the error code */
    srvtyperply->errorcode = AsUINT16(buffer->curpos);
    if(srvtyperply->errorcode)
    {
        /* We better not trust the rest of the packet */
        memset(srvtyperply,0,sizeof(SLPSrvTypeRply));
        srvtyperply->errorcode = AsUINT16(buffer->curpos);
        return 0;
    }
    buffer->curpos += 2;

    /* parse out the error srvtype-list length */
    srvtyperply->srvtypelistlen = AsUINT16(buffer->curpos);
    buffer->curpos += 2;

    if(srvtyperply->srvtypelistlen > buffer->end - buffer->curpos)
    {
        return SLP_ERROR_PARSE_ERROR;
    }
    srvtyperply->srvtypelist = buffer->curpos;

    return 0;
}

int ParseExtension(SLPBuffer buffer, SLPMessage message)
{
    int             extid;
    int             nextoffset;
    int             result  = SLP_ERROR_OK;

    nextoffset = message->header.extoffset;
    while(nextoffset)
    {
        buffer->curpos = buffer->start + nextoffset;
        if(buffer->curpos + 5 >= buffer->end)
        {
            /* Extension takes us past the end of the buffer */
            result = SLP_ERROR_PARSE_ERROR;
            goto CLEANUP;
        }
    
        extid = AsUINT16(buffer->curpos);
        buffer->curpos += 2;

        nextoffset = AsUINT24(buffer->curpos);
        buffer->curpos += 3;
        
        switch(extid)
        {
        case SLP_EXTENSION_ID_REG_PID:
            if(message->header.functionid == SLP_FUNCT_SRVREG)
            {
                /* check to see if buffer is large enough to contain the 4 byte pid */
                if(buffer->curpos + 4 > buffer->end)
                {
                    result = SLP_ERROR_PARSE_ERROR;
                    goto CLEANUP;
                }
                
                message->body.srvreg.pid = AsUINT32(buffer->curpos);
                buffer->curpos += 4;
            }
            break;

        default:
            if (extid >= 0x4000 && extid <= 0x7FFF )
            {
                /* This is a required extension.  We better error out */
                result = SLP_ERROR_MESSAGE_NOT_SUPPORTED;
                goto CLEANUP;
            }
            break;
        }
    }

CLEANUP:
    
    return result;
}

/*=========================================================================*/
