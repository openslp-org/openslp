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

/** Reads service registration file.
 *
 * @file       slpd_regfile.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#include "slpd_regfile.h"
#include "slpd_property.h"
#include "slpd_log.h"

#if defined(ENABLE_SLPv2_SECURITY)
# include "slpd_spi.h"
#endif

#include "slp_xmalloc.h"
#include "slp_compare.h"

#if defined(ENABLE_SLPv2_SECURITY)
# include "slp_auth.h"
#endif

/** Trim leading and trailing whitespace from a string.
 *
 * @param[in,out] str - The address of the string to be trimmed.
 *
 * @return A pointer to the first non-whitespace character in @p str.
 *
 * @internal
 */
static char * TrimWhitespace(char * str)
{
   char * end;
   
   end = str+strlen(str)-1;
   while (*str && *str <= 0x20)
      str++;

   while (end >= str)
   {
      if (*end > 0x20)
         break;
      *end = 0;
      end--;
   }
   return str;
}

/** Read a line from a file into a buffer.
 *
 * @param[in] fd - The file to read from.
 * @param[out] line - The address of storage for the read line.
 * @param[in] linesize - The size of the buffer pointed to by @p line.
 *
 * @return A pointer to @p line for convenience.
 *
 * @internal
 */
static char * RegFileReadLine(FILE * fd, char * line, int linesize)
{
   while (1)
   {
      if (fgets(line,linesize,fd) == 0)
         return 0;

      while(*line && *line <= 0x20 && *line != 0x0d && *line != 0x0a) 
         line++;

      if (*line == 0x0d || *line == 0x0a)
         break;    

      if (*line != 0 && *line != '#' && *line != ';')
         break;
   }
   return line;
}

/** Read service registrations from a text file.
 *
 * A really big and nasty function that reads service registrations from
 * from a file. Don't look at this too hard or you'll be sick. This is by
 * far the most horrible code in OpenSLP. Please volunteer to rewrite it!
 *
 * "THANK GOODNESS this function is only called at startup" -- Matt
 *
 * @param[in] fd - The file to read from.
 * @param[out] msg - A message describing the SrvReg in buf.
 * @param[out] buf - The buffer used to hold @p message data.
 *
 * @return Zero on success. A value greater than zero on error. A value
 *    less than zero on EOF.
 *
 * @note Eventually the caller needs to call SLPBufferFree and
 *    SLPMessageFree to free memory.
 */
int SLPDRegFileReadSrvReg(FILE * fd, SLPMessage * msg, SLPBuffer * buf)
{
   char * slider1;
   char * slider2;
   char line[4096];

   struct sockaddr_storage peer;
   int result = 0;
   size_t bufsize = 0;
   size_t langtaglen = 0;
   char * langtag = 0;
   size_t scopelistlen = 0;
   char * scopelist = 0;
   size_t urllen = 0;
   char * url = 0;
   int lifetime = 0;
   size_t srvtypelen = 0;
   char * srvtype = 0;
   size_t attrlistlen = 0;
   char * attrlist = 0;
   SLPBuffer tmp;

#ifdef ENABLE_SLPv2_SECURITY
   unsigned char * urlauth = 0;
   size_t urlauthlen = 0;
   unsigned char * attrauth = 0;
   size_t attrauthlen = 0;
#endif

   /* give the out params an initial NULL value */
   *buf = 0;
   *msg = 0;

   /* read the next non-white non-comment line from the stream */
   do
   {
      slider1 = RegFileReadLine(fd, line, 4096);
      if (slider1 == 0)
         return -1; 

   } while (*slider1 == 0x0d ||  *slider1 == 0x0a);

   /* Parse the url-props */
   slider2 = strchr(slider1, ',');
   if (slider2)
   {
      /* srvurl */
      *slider2 = 0; /* squash comma to null terminate srvurl */
      url = xstrdup(TrimWhitespace(slider1));
      if (url == 0)
      {
         result = SLP_ERROR_INTERNAL_ERROR;
         goto CLEANUP;
      }
      urllen = strlen(url);

      /* derive srvtype from srvurl */
      srvtype = strstr(slider1, "://");
      if (srvtype == 0)
      {
         result = SLP_ERROR_INVALID_REGISTRATION;
         goto CLEANUP;   
      }
      *srvtype = 0;
      srvtype=xstrdup(TrimWhitespace(slider1));
      if (srvtype == 0)
      {
         result = SLP_ERROR_INTERNAL_ERROR;
         goto CLEANUP;
      }
      srvtypelen = strlen(srvtype);
      slider1 = slider2 + 1;

      /*lang*/
      slider2 = strchr(slider1, ',');
      if (slider2)
      {
         *slider2 = 0; /* squash comma to null terminate lang */
         langtag = xstrdup(TrimWhitespace(slider1)); 
         if (langtag == 0)
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
      if (slider2)
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
      if (lifetime < 1 || lifetime > SLP_LIFETIME_MAXIMUM)
      {
         result = SLP_ERROR_INVALID_REGISTRATION;
         goto CLEANUP;   
      }

      /* get the srvtype if one was not derived by the srvurl */
      if (srvtype == 0)
      {
         srvtype = xstrdup(TrimWhitespace(slider1));
         if (srvtype == 0)
         {
            result = SLP_ERROR_INTERNAL_ERROR;
            goto CLEANUP;
         }
         srvtypelen = strlen(srvtype);
         if (srvtypelen == 0)
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

   /* read all the attributes including the scopelist */
   *line=0;
   while (1)
   {
      slider1 = RegFileReadLine(fd,line,4096);
      if (slider1 == 0)
      {
         result = -1;
         break;
      }
      if (*slider1 == 0x0d || *slider1 == 0x0a)
         break;

      /* Check to see if it is the scopes line */
      /* FIXME We can collapse the scope stuff into the value getting and 
         just make it a special case (do strcmp on the tag as opposed to the 
         line) of attribute getting. */
      if (strncasecmp(slider1,"scopes", 6) == 0)
      {
         /* found scopes line */
         slider2 = strchr(slider1,'=');
         if (slider2)
         {
            slider2++;
            if (*slider2)
            {
               /* just in case some idiot puts multiple scopes lines */
               if (scopelist)
               {
                  result = SLP_ERROR_SCOPE_NOT_SUPPORTED;
                  goto CLEANUP;
               }

               /* make sure there are no spaces in the scope list */
               if (strchr(slider2, ' '))
               {
                  result = SLP_ERROR_SCOPE_NOT_SUPPORTED;
                  goto CLEANUP;
               }

               scopelist = xstrdup(TrimWhitespace(slider2));
               if (scopelist == 0)
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

         if (attrlist == 0)
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
            strcat(attrlist, ",");
         }

         if (attrlist == 0)
         {
            result = SLP_ERROR_INTERNAL_ERROR;
            goto CLEANUP;
         }

         /* we need special case for keywords (why do we need these)
            they seem like a waste of code.  Why not just use booleans */
         if (strchr(slider1, '='))
         {
            /* normal attribute (with '=') */
            strcat(attrlist, "(");
            strcat(attrlist, slider1);
            strcat(attrlist, ")");
         }
         else
         {
            /* keyword (no '=') */
            attrlistlen -= 2; /* subtract 2 bytes for no '(' or ')' */
            strcat(attrlist, slider1);
         }
      }
   }

   /* Set the scope set in properties if not is set */
   if (scopelist == 0)
   {
      scopelist = xstrdup(G_SlpdProperty.useScopes);
      if (scopelist == 0)
      {
         result = SLP_ERROR_INTERNAL_ERROR;
         goto CLEANUP;
      }
      scopelistlen = G_SlpdProperty.useScopesLen;
   }

#ifdef ENABLE_SLPv2_SECURITY
   /* generate authentication blocks */
   if (G_SlpdProperty.securityEnabled)
   {
      SLPAuthSignUrl(G_SlpdSpiHandle, 0, 0, urllen, url, 
            &urlauthlen, &urlauth);
      SLPAuthSignString(G_SlpdSpiHandle, 0, 0, attrlistlen, attrlist, 
            &attrauthlen, &attrauth);
   }
#endif

   /* allocate buffer for the SrvReg Message */
   bufsize = 14 + langtaglen;    /* 14 bytes for header    */
   bufsize += urllen + 6;        /*  1 byte for reserved   */
                                 /*  2 bytes for lifetime  */
                                 /*  2 bytes for urllen    */
                                 /*  1 byte for authcount  */
   bufsize += srvtypelen + 2;    /*  2 bytes for len field */
   bufsize += scopelistlen + 2;  /*  2 bytes for len field */
   bufsize += attrlistlen + 2;   /*  2 bytes for len field */
   bufsize += 1;                 /*  1 byte for authcount  */

#ifdef ENABLE_SLPv2_SECURITY
   bufsize += urlauthlen;
   bufsize += attrauthlen;
#endif  

   tmp = *buf = SLPBufferAlloc(bufsize);
   if (tmp == 0)
   {
      result = SLP_ERROR_INTERNAL_ERROR;
      goto CLEANUP;
   }

   /* now build the SrvReg Message */

   /* version */
   *tmp->curpos++ = 2;

   /* function id */
   *tmp->curpos++ = SLP_FUNCT_SRVREG;

   /* length */
   PutUINT24(&tmp->curpos, bufsize);

   /* flags */
   PutUINT16(&tmp->curpos, 0);

   /* ext offset */
   PutUINT24(&tmp->curpos, 0);

   /* xid */
   PutUINT16(&tmp->curpos, 0);

   /* lang tag len */
   PutUINT16(&tmp->curpos, langtaglen);

   /* lang tag */
   memcpy(tmp->curpos, langtag, langtaglen);
   tmp->curpos += langtaglen;

   /* url-entry reserved */
   *tmp->curpos++ = 0;

   /* url-entry lifetime */
   PutUINT16(&tmp->curpos, lifetime);

   /* url-entry urllen */
   PutUINT16(&tmp->curpos, urllen);

   /* url-entry url */
   memcpy(tmp->curpos, url, urllen);
   tmp->curpos += urllen;

   /* url-entry authblock */
#ifdef ENABLE_SLPv2_SECURITY
   if (urlauth)
   {
      /* authcount */
      *tmp->curpos++ = 1;

      /* authblock */
      memcpy(tmp->curpos, urlauth, urlauthlen);
      tmp->curpos += urlauthlen;
   }
   else
#endif
      *tmp->curpos++ = 0;

   /* service type */
   PutUINT16(&tmp->curpos, srvtypelen);
   memcpy(tmp->curpos, srvtype, srvtypelen);
   tmp->curpos += srvtypelen;

   /* scope list */
   PutUINT16(&tmp->curpos, scopelistlen);
   memcpy(tmp->curpos, scopelist, scopelistlen);
   tmp->curpos += scopelistlen;

   /* attr list */
   PutUINT16(&tmp->curpos, attrlistlen);
   memcpy(tmp->curpos, attrlist, attrlistlen);
   tmp->curpos += attrlistlen;

   /* attribute auth block */
#ifdef ENABLE_SLPv2_SECURITY
   if (attrauth)
   {
      /* authcount */
      *tmp->curpos++ = 1;

      /* authblock */
      memcpy(tmp->curpos, attrauth, attrauthlen);
      tmp->curpos += attrauthlen;
   }
   else
#endif
      *tmp->curpos++ = 0;

   /* okay, now comes the really stupid (and lazy part) */
   *msg = SLPMessageAlloc();
   if (*msg == 0)
   {
      SLPBufferFree(*buf);
      *buf = 0;
      result = SLP_ERROR_INTERNAL_ERROR;
      goto CLEANUP;
   }

   /* this should be ok even if we are not supporting IPv4, 
    * since it's a static service 
    */
   peer.ss_family = AF_INET;
   ((struct sockaddr_in *)&peer)->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   result = SLPMessageParseBuffer(&peer, &peer, *buf, *msg);
   (*msg)->body.srvreg.source = SLP_REG_SOURCE_STATIC;

CLEANUP:

   /* check for errors and free memory */
   switch(result)
   {
      case SLP_ERROR_INTERNAL_ERROR:
         SLPDLog("\nERROR: Out of memory one reg file line:\n   %s\n", line);
         break;

      case SLP_ERROR_INVALID_REGISTRATION:
         SLPDLog("\nERROR: Invalid reg file format near:\n   %s\n", line);
         break;

      case SLP_ERROR_SCOPE_NOT_SUPPORTED:
         SLPDLog("\nERROR: Duplicate scopes or scope list with "
               "embedded spaces near:\n   %s\n", line);
         break;

      default:
         break;
   }

   xfree(langtag);
   xfree(scopelist);
   xfree(url);
   xfree(srvtype);
   xfree(attrlist);

#ifdef ENABLE_SLPv2_SECURITY
   xfree(urlauth);
   xfree(attrauth);
#endif

   return result;
}

/*=========================================================================*/
