/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_regfile.c                                             */
/*                                                                         */
/* Abstract:    Reads service registrations from a file                    */
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
#include "slpd_regfile.h"
#include "slpd_property.h"
#include "slpd_log.h"
#ifdef ENABLE_SECURITY
#include "slpd_spi.h"
#endif

/*=========================================================================*/
/* common code includes                                                    */
/*=========================================================================*/
#include "../common/slp_xmalloc.h"
#ifdef ENABLE_SECURITY
#include "../common/slp_auth.h"
#endif



/*-------------------------------------------------------------------------*/
char* TrimWhitespace(char* str)
/*-------------------------------------------------------------------------*/
{
    char* end;

    end=str+strlen(str)-1;

    while(*str && *str <= 0x20)
    {
        str++;
    }

    while(end >= str)
    {
        if(*end > 0x20)
        {
            break;
        }

        *end = 0;

        end--;
    }

    return str;
}

/*-------------------------------------------------------------------------*/
char* RegFileReadLine(FILE* fd, char* line, int linesize)
/*-------------------------------------------------------------------------*/
{
    while(1)
    {
        if(fgets(line,linesize,fd) == 0)
        {
            return 0;
        }
        
        while(*line && 
              *line <= 0x20 &&
              *line != 0x0d &&
              *line != 0x0a) line++;
        
        if(*line == 0x0d || 
           *line == 0x0a)
        {
            break;    
        }

        if(*line != 0 && *line != '#' && *line != ';')
        {
            break;
        }
    }

    return line;
}

/*=========================================================================*/
int SLPDRegFileReadSrvReg(FILE* fd,
                          SLPMessage* msg,
                          SLPBuffer* buf)
/* A really big and nasty function that reads an service registration from */
/* from a file. Don't look at this too hard or you'll be sick.  This is by */
/* the most horrible code in OpenSLP.  Please volunteer to rewrite it!     */
/*                                                                         */
/*  "THANK GOODNESS this function is only called at startup" -- Matt       */
/*                                                                         */
/*                                                                         */
/* fd       (IN) file to read from                                         */
/*                                                                         */
/* msg      (OUT) message describing the SrvReg in buf                     */
/*                                                                         */
/* buf      (OUT) buffer containing the SrvReg                             */
/*                                                                         */
/* Returns:  zero on success. > 0 on error.  < 0 if EOF                    */
/*                                                                         */
/* Note:    Eventually the caller needs to call SLPBufferFree() and        */
/*          SLPMessageFree() to free memory                                */
/*=========================================================================*/
{
    char*   slider1;
    char*   slider2;
    char    line[4096];
    
    struct  sockaddr_in     peer;
    int     result          = 0;
    int     bufsize         = 0;
    int     langtaglen      = 0;
    char*   langtag         = 0;
    int     scopelistlen    = 0;
    char*   scopelist       = 0;
    int     urllen          = 0;
    char*   url             = 0;
    int     lifetime        = 0;
    int     srvtypelen      = 0;
    char*   srvtype         = 0;
    int     attrlistlen     = 0;
    char*   attrlist        = 0;
#ifdef ENABLE_SECURITY
    unsigned char*  urlauth         = 0;
    int             urlauthlen      = 0;
    unsigned char*  attrauth        = 0;
    int             attrauthlen     = 0;
#endif
    
    
    /*-------------------------------------------*/
    /* give the out params an initial NULL value */
    /*-------------------------------------------*/
    *buf = 0;
    *msg = 0;

    /*----------------------------------------------------------*/
    /* read the next non-white non-comment line from the stream */
    /*----------------------------------------------------------*/
    do
    {
        slider1 = RegFileReadLine(fd,line,4096);
        if(slider1 == 0)
        {
            /* Breath a sigh of relief.  We get out before really  */
            /* horrid code                                         */
            return -1;
        }
    }while(*slider1 == 0x0d ||  *slider1 == 0x0a);

    
    /*---------------------*/
    /* Parse the url-props */
    /*---------------------*/
    slider2 = strchr(slider1,',');
    if(slider2)
    {
        /* srvurl */
        *slider2 = 0; /* squash comma to null terminate srvurl */
        url = xstrdup(TrimWhitespace(slider1));
        if(url == 0)
        {
            result = SLP_ERROR_INTERNAL_ERROR;
            goto CLEANUP;
        }
        urllen = strlen(url);

        /* derive srvtype from srvurl */
        srvtype = strstr(slider1,"://");
        if(srvtype == 0)
        {
            result = SLP_ERROR_INVALID_REGISTRATION;
            goto CLEANUP;   
        }
        *srvtype = 0;
        srvtype=xstrdup(TrimWhitespace(slider1));
        if(srvtype == 0)
        {
            result = SLP_ERROR_INTERNAL_ERROR;
            goto CLEANUP;
        }
        srvtypelen = strlen(srvtype);
        slider1 = slider2 + 1;

        /*lang*/
        slider2 = strchr(slider1,',');
        if(slider2)
        {
            *slider2 = 0; /* squash comma to null terminate lang */
            langtag = xstrdup(TrimWhitespace(slider1)); 
            if(langtag == 0)
            {
                result = SLP_ERROR_INVALID_REGISTRATION;
                goto CLEANUP;   
            }
            langtaglen = strlen(langtag);     
            slider1 = slider2 + 1;                                  
        }
        else
        {
            result = SLP_ERROR_INVALID_REGISTRATION;
            goto CLEANUP;   
        }

        /* ltime */
        slider2 = strchr(slider1,',');
        if(slider2)
        {
            *slider2 = 0; /* squash comma to null terminate ltime */
            lifetime = atoi(slider1);
            slider1 = slider2 + 1;
        }
        else
        {
            lifetime = atoi(slider1);
            slider1 = slider2;
        }
        if(lifetime < 1 || lifetime > SLP_LIFETIME_MAXIMUM)
        {
            result = SLP_ERROR_INVALID_REGISTRATION;
            goto CLEANUP;   
        }

        /* get the srvtype if one was not derived by the srvurl*/
        if(srvtype == 0)
        {
            srvtype = xstrdup(TrimWhitespace(slider1));
            if(srvtype == 0)
            {
                result = SLP_ERROR_INTERNAL_ERROR;
                goto CLEANUP;
            }
            srvtypelen = strlen(srvtype);
            if(srvtypelen == 0)
            {
                result = SLP_ERROR_INVALID_REGISTRATION;
                goto CLEANUP;   
            }
        }
    }
    else
    {
        result = SLP_ERROR_INVALID_REGISTRATION;
        goto CLEANUP;   
    }

    /*-------------------------------------------------*/
    /* Read all the attributes including the scopelist */
    /*-------------------------------------------------*/
    *line=0;
    while(1)
    {
        slider1 = RegFileReadLine(fd,line,4096);
        if(slider1 == 0)
        {
            /* Breath a sigh of relief.  We're done */
            result = -1;
            break;
        }
        if(*slider1 == 0x0d || *slider == 0x0a)
        {
            break;
        }

        /* Check to see if it is the scopes line */
        /* FIXME We can collapse the scope stuff into the value getting and 
         * just make it a special case (do strcmp on the tag as opposed to the 
         * line) of attribute getting. 
         */
        if(strncasecmp(slider1,"scopes",6) == 0)
        {
            /* found scopes line */
            slider2 = strchr(slider1,'=');
            if(slider2)
            {
                slider2++;
                if(*slider2)
                {
                    /* just in case some idiot puts multiple scopes lines */
                    if(scopelist)
                    {
                        result = SLP_ERROR_SCOPE_NOT_SUPPORTED;
                        goto CLEANUP;
                    }
                    scopelist=xstrdup(TrimWhitespace(slider2));
                    if(scopelist == 0)
                    {
                        result = SLP_ERROR_INTERNAL_ERROR;
                        goto CLEANUP;
                    }
                    scopelistlen = strlen(scopelist);
                }
            }
        }
        else
        {
            /* line contains an attribute (slow but it works)*/
            /* TODO Fix this so we do not have to realloc memory each time! */
            TrimWhitespace(slider1); 
            
            if(attrlist == 0)
            {
                attrlistlen += strlen(slider1) + 2;
                attrlist = xmalloc(attrlistlen + 1);
                *attrlist = 0;
            }
            else
            {
                attrlistlen += strlen(slider1) + 3;
                attrlist = xrealloc(attrlist,
                                   attrlistlen + 1);
                strcat(attrlist,",");
            }

            if(attrlist == 0)
            {
                result = SLP_ERROR_INTERNAL_ERROR;
                goto CLEANUP;
            }
            strcat(attrlist,"(");
            strcat(attrlist,slider1);
            strcat(attrlist,")");
        }
    }

    /* Set the scope set in properties if not is set */
    if(scopelist == 0)
    {
        scopelist=xstrdup(G_SlpdProperty.useScopes);
        if(scopelist == 0)
        {
            result = SLP_ERROR_INTERNAL_ERROR;
            goto CLEANUP;
        }
        scopelistlen = G_SlpdProperty.useScopesLen;
    }

 
#ifdef ENABLE_SECURITY
    /*--------------------------------*/
    /* Generate authentication blocks */
    /*--------------------------------*/
    if(G_SlpdProperty.securityEnabled)
    {
        
        SLPAuthSignUrl(G_SlpdSpiHandle,
                       0,
                       0,
                       urllen,
                       url,
                       &urlauthlen,
                       &urlauth);
    
        SLPAuthSignString(G_SlpdSpiHandle,
                          0,
                          0,
                          attrlistlen,
                          attrlist,
                          &attrauthlen,
                          &attrauth);
    }
#endif


    /*----------------------------------------*/
    /* Allocate buffer for the SrvReg Message */
    /*----------------------------------------*/
    bufsize = 14 + langtaglen;  /* 14 bytes for header    */
    bufsize += urllen + 6;      /*  1 byte for reserved   */
                                /*  2 bytes for lifetime  */
                                /*  2 bytes for urllen    */
                                /*  1 byte for authcount  */
    bufsize += srvtypelen + 2;  /*  2 bytes for len field */
    bufsize += scopelistlen + 2;/*  2 bytes for len field */
    bufsize += attrlistlen + 2; /*  2 bytes for len field */
    bufsize += 1;               /*  1 byte for authcount  */
    #ifdef ENABLE_SECURITY
    bufsize += urlauthlen;
    bufsize += attrauthlen;
    #endif  
    *buf = SLPBufferAlloc(bufsize);
    if(*buf == 0)
    {
        result = SLP_ERROR_INTERNAL_ERROR;
        goto CLEANUP;
    }
    
    /*------------------------------*/
    /* Now build the SrvReg Message */
    /*------------------------------*/
    /*version*/
    *((*buf)->start)       = 2;
    /*function id*/
    *((*buf)->start + 1)   = SLP_FUNCT_SRVREG;
    /*length*/
    ToUINT24((*buf)->start + 2, bufsize);
    /*flags*/
    ToUINT16((*buf)->start + 5, 0);
    /*ext offset*/
    ToUINT24((*buf)->start + 7,0);
    /*xid*/
    ToUINT16((*buf)->start + 10, 0);
    /*lang tag len*/
    ToUINT16((*buf)->start + 12,langtaglen);
    /*lang tag*/
    memcpy((*buf)->start + 14, langtag, langtaglen);
    (*buf)->curpos = (*buf)->start + langtaglen + 14 ;
    /* url-entry reserved */
    *(*buf)->curpos= 0;        
    (*buf)->curpos = (*buf)->curpos + 1;
    /* url-entry lifetime */
    ToUINT16((*buf)->curpos,lifetime);
    (*buf)->curpos = (*buf)->curpos + 2;
    /* url-entry urllen */
    ToUINT16((*buf)->curpos,urllen);
    (*buf)->curpos = (*buf)->curpos + 2;
    /* url-entry url */
    memcpy((*buf)->curpos,url,urllen);
    (*buf)->curpos = (*buf)->curpos + urllen;
    /* url-entry authblock */
#ifdef ENABLE_SECURITY
    if(urlauth)
    {
        /* authcount */
        *(*buf)->curpos = 1;
        (*buf)->curpos = (*buf)->curpos + 1;
        /* authblock */
        memcpy((*buf)->curpos,urlauth,urlauthlen);
        (*buf)->curpos = (*buf)->curpos + urlauthlen;
    }
    else
#endif
    {
        /* authcount */
        *(*buf)->curpos = 0;
        (*buf)->curpos += 1;
    } 
    /* service type */
    ToUINT16((*buf)->curpos,srvtypelen);
    (*buf)->curpos = (*buf)->curpos + 2;
    memcpy((*buf)->curpos,srvtype,srvtypelen);
    (*buf)->curpos = (*buf)->curpos + srvtypelen;
    /* scope list */
    ToUINT16((*buf)->curpos,scopelistlen);
    (*buf)->curpos = (*buf)->curpos + 2;
    memcpy((*buf)->curpos,scopelist,scopelistlen);
    (*buf)->curpos = (*buf)->curpos + scopelistlen;
    /* attr list */
    ToUINT16((*buf)->curpos,attrlistlen);
    (*buf)->curpos = (*buf)->curpos + 2;
    memcpy((*buf)->curpos,attrlist,attrlistlen);
    (*buf)->curpos = (*buf)->curpos + attrlistlen;
    /* attribute auth block */
#ifdef ENABLE_SECURITY
    if(attrauth)
    {
        /* authcount */
        *(*buf)->curpos = 1;
        (*buf)->curpos = (*buf)->curpos + 1;
        /* authblock */
        memcpy((*buf)->curpos,attrauth,attrauthlen);
        (*buf)->curpos = (*buf)->curpos + attrauthlen;
    }
    else
#endif
    {
        /* authcount */
        *(*buf)->curpos = 0;
        (*buf)->curpos = (*buf)->curpos + 1;
    }

    /*------------------------------------------------*/
    /* Ok Now comes the really stupid (and lazy part) */
    /*------------------------------------------------*/
    *msg = SLPMessageAlloc();
    if(*msg == 0)
    {
        SLPBufferFree(*buf);
        *buf=0;
        result = SLP_ERROR_INTERNAL_ERROR;
        goto CLEANUP;
    }
    peer.sin_addr.s_addr = htonl(LOOPBACK_ADDRESS);
    result = SLPMessageParseBuffer(&peer,*buf,*msg);
    (*msg)->body.srvreg.source = SLP_REG_SOURCE_STATIC;
    
    
CLEANUP:
    
    /*----------------------------------*/
    /* Check for errors and free memory */
    /*----------------------------------*/
    switch(result)
    {
    case SLP_ERROR_INTERNAL_ERROR:
        SLPDLog("\nERROR: Out of memory one reg file line:\n   %s\n",line);
        break;
    case SLP_ERROR_INVALID_REGISTRATION:
        SLPDLog("\nERROR: Invalid reg file format near:\n   %s\n",line);
        break;
    case SLP_ERROR_SCOPE_NOT_SUPPORTED:
        SLPDLog("\nERROR: Duplicate scopes for same registration near:\n   %s\n",line);
        break;
    default:
        break;
    }
        
    if(langtag) xfree(langtag);
    if(scopelist) xfree(scopelist);
    if(url) xfree(url);
    if(srvtype) xfree(srvtype);
    if(attrlist)xfree(attrlist);
#ifdef ENABLE_SECURITY
    if(urlauth) xfree(urlauth);
    if(attrauth) xfree(attrauth);
#endif

    return result;
}
