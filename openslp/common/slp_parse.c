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

/** Service URL parsing routines.
 *
 * @file       slp_parse.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#include "slp_parse.h"
#include "slp_xmalloc.h"

#include <errno.h>

/** Parse a service URL.
 *
 * Parses a service URL into its constituent parts.
 *
 * @param[in] srvurllen - The size of @p srvurl in bytes.
 * @param[in] srvurl - A pointer to the URL to be parsed.
 * @param[out] parsedurl - The address of storage for the parsed
 *    URL components.
 *
 * @return Zero on success, or an errno value on failure.
 *
 * @remarks The returned pointer must be freed by caller with a call 
 *    to xfree.
 */
int SLPParseSrvUrl(int srvurllen, const char * srvurl,
      SLPParsedSrvUrl ** parsedurl)
{
   char * empty;   /* always points to an empty string */
   char * buf;     /* points to location in the SLPSrvUrl buffer */
   char * slider1; /* points to parse locations in srvurl */
   char * slider2; /* points to parse locations srvurl */
   char * end;     /* points at the end of srvurl */
   int isIpv6Host = 0;

   /* Allocate memory and set up sliders */
   *parsedurl = (SLPParsedSrvUrl*)xmalloc(srvurllen + sizeof(SLPParsedSrvUrl) + 5);

   /* +5 ensures space for 5 null terminations */
   if (*parsedurl == 0)
      return ENOMEM;

   memset(*parsedurl, 0, srvurllen + sizeof(SLPParsedSrvUrl) + 5);
   buf = ((char *)*parsedurl) + sizeof(SLPParsedSrvUrl);
   slider1 = slider2 = (char *)srvurl;
   end = slider1 + srvurllen;

   /* Set empty */
   empty = buf;
   buf++;

   /* parse out the service type */
   slider2 = (char *)strstr(slider2, ":/");
   if (slider2 == 0)
   {
      xfree(*parsedurl);
      *parsedurl = 0;
      return EINVAL;
   }
   memcpy(buf, slider1, slider2 - slider1);
   (*parsedurl)->srvtype = buf;
   buf += (slider2 - slider1) + 1;

   /* parse out the host */
   slider2 = slider1 = slider2 + 3; /* + 3 skips the "://" */

   /* check for IPV6 url with [] surrounding the host */
   if (slider2 < end && *slider2 == '[')
   {
      /* we have an ipv6 host address */
      slider2 = strchr(slider2, ']');
      if (slider2)
      {
         /* get past the ending ] */
         slider2++;
         isIpv6Host = 1;
      }
      else
      {
         /* get to the next good character */
         slider2 = slider1;
         while (slider2 < end && *slider2 != '/' && *slider2 != ':') 
            slider2++;
      }
   }
   else  /* ipv4 address */
      while (slider2 < end && *slider2 != '/' && *slider2 != ':') 
         slider2++;

   if (slider2-slider1 < 1)
      (*parsedurl)->host = empty; /* no host (this is okay per RFC2609) */
   else
   {
      if (isIpv6Host)
      {
         /* different condition here - must just get stuff inside beginning [ and ending ] */
         slider1++;
         memcpy(buf,slider1,slider2-slider1-1);
         (*parsedurl)->host = buf;
         buf += slider2 - slider1;
      }
      else
      {
         memcpy(buf,slider1,slider2-slider1);
         (*parsedurl)->host = buf;
         buf += (slider2 - slider1) + 1;
      }
   }

   /* parse out the port */
   if (*slider2 == ':')
   {
      slider2 ++; /* + 1 skips the ":" */
      slider1 = slider2; /* set up slider1 at the start of the port str */
      while (*slider2 && (*slider2 != '/' && *slider2 != ';')) 
         slider2++;

      if (slider2 - slider1 < 1)
         (*parsedurl)->port = 80; /* FIXME: no port, default to 80? */
      else
      {
         memcpy(buf,slider1,slider2-slider1);
         (*parsedurl)->port = atoi(buf);
         buf += (slider2-slider1) + 1;
      }
   }

   /* parse out the remainder of the url */
   if (slider2 < end)
   {
      slider1 = slider2;
      memcpy(buf,slider1,end-slider1);
      (*parsedurl)->remainder = buf;
      buf += (end - slider2) + 1;
   }
   else
      (*parsedurl)->remainder = empty; /* no remainder portion */

   /* set the net family to always be an empty string for IP */
   (*parsedurl)->family = empty;

   return 0;
}

/*===========================================================================
 *  TESTING CODE enabled by removing #define comment and compiling with the 
 *  following command line:
 *
 *  $ gcc -g -DSLP_PARSE_TEST -DDEBUG slp_parse.c slp_xmalloc.c 
 *        slp_linkedlist.c
 */
//#define SLP_PARSE_TEST
#ifdef SLP_PARSE_TEST 

#define TESTSRVTYPE1    "service:printer.x"
#define TESTHOST1       "192.168.100.2"
#define TESTHOST2       "[1111:2222:3333::4444]"
#define TESTHOST2_PARSED "1111:2222:3333::4444"
#define TESTHOST3       "[1111:2222:3333::4444"
#define TESTPORT1       "4563"
#define TESTREMAINDER1  "/hello/good/world"

#define TESTURL1        TESTSRVTYPE1"://"TESTHOST1":"TESTPORT1 TESTREMAINDER1
#define TESTURL2        TESTSRVTYPE1"://"TESTHOST1":"TESTPORT1
#define TESTURL3        TESTSRVTYPE1"://"TESTHOST1
#define TESTURL4        "service:badurl"
#define TESTURL5        TESTSRVTYPE1"://"TESTHOST2":"TESTPORT1 TESTREMAINDER1
#define TESTURL6        TESTSRVTYPE1"://"TESTHOST3":"TESTPORT1 TESTREMAINDER1

int main(int argc, char* argv[])
{
   SLPParsedSrvUrl* parsedurl;

   printf("Testing srvurl:   %s\n",TESTURL1);
   if (SLPParseSrvUrl(strlen(TESTURL1),
         TESTURL1,
         &parsedurl) != 0)
   {
      printf("FAILURE: SLPParseSrvUrl returned error\n");
      return -1;
   }
   if (strcmp(parsedurl->srvtype,TESTSRVTYPE1))
      printf("FAILURE: wrong srvtype\n");
   else if (strcmp(parsedurl->host,TESTHOST1))
      printf("FAILURE: wrong host\n");
   else if (parsedurl->port != atoi(TESTPORT1))
      printf("FAILURE: wrong port\n");
   else if (strcmp(parsedurl->remainder,TESTREMAINDER1))
      printf("FAILURE: wrong remainder\n");
   else if (*(parsedurl->family))
      printf("FAILURE: wrong family\n");
   else
      printf("Success!\n");

   if (parsedurl) xfree(parsedurl);

   /* TESTURL2 */
   printf("Testing srvurl:   %s\n",TESTURL2);
   if (SLPParseSrvUrl(strlen(TESTURL2),
         TESTURL2,
         &parsedurl) != 0)
   {
      printf("FAILURE: SLPParseSrvUrl returned error\n");
      return -1;
   }
   if (strcmp(parsedurl->srvtype,TESTSRVTYPE1))
      printf("FAILURE: wrong srvtype\n");
   else if (strcmp(parsedurl->host,TESTHOST1))
      printf("FAILURE: wrong host\n");
   else if (parsedurl->port != atoi(TESTPORT1))
      printf("FAILURE: wrong port\n");

   /*
   else if(strcmp(parsedurl->remainder, TESTREMAINDER1))
      printf("FAILURE: wrong remainder\n");
   */

   else if (*(parsedurl->family))
      printf("FAILURE: wrong family\n");
   else
      printf("Success!\n");

   if (parsedurl) xfree(parsedurl);

   /* TESTURL3 */
   printf("Testing srvurl:   %s\n",TESTURL3);
   if (SLPParseSrvUrl(strlen(TESTURL3),
         TESTURL3,
         &parsedurl) != 0)
   {
      printf("FAILURE: SLPParseSrvUrl returned error\n");
      return -1;
   }
   if (strcmp(parsedurl->srvtype,TESTSRVTYPE1))
      printf("FAILURE: wrong srvtype\n");
   else if (strcmp(parsedurl->host,TESTHOST1))
      printf("FAILURE: wrong host\n");

   /*
   else if(parsedurl->port != atoi(TESTPORT1))
      printf("FAILURE: wrong port\n");
   else if(strcmp(parsedurl->remainder,TESTREMAINDER1))
      printf("FAILURE: wrong remainder\n");
   */

   else if (*(parsedurl->family))
      printf("FAILURE: wrong family\n");
   else
      printf("Success!\n");

   if (parsedurl) xfree(parsedurl);

   /* TESTURL4 */
   printf("Testing srvurl:   %s\n",TESTURL4);
   if (SLPParseSrvUrl(strlen(TESTURL4),
         TESTURL4,
         &parsedurl) == 0)
      printf("FAILURE: SLPParseSrvUrl should have returned an error\n");
   else
      printf("Success!\n");

   if (parsedurl) xfree(parsedurl);

   /* TESTURL5 */
   printf("Testing srvurl:   %s\n",TESTURL5);
   if (SLPParseSrvUrl(strlen(TESTURL5),
         TESTURL5,
         &parsedurl) != 0)
   {
      printf("FAILURE: SLPParseSrvUrl returned error\n");
      return -1;
   }
   if (strcmp(parsedurl->srvtype,TESTSRVTYPE1))
      printf("FAILURE: wrong srvtype\n");
   else if (strcmp(parsedurl->host,TESTHOST2_PARSED))  /* host should be different */
      printf("FAILURE: wrong host\n");
   else if (parsedurl->port != atoi(TESTPORT1))
      printf("FAILURE: wrong port\n");
   else if (strcmp(parsedurl->remainder,TESTREMAINDER1))
      printf("FAILURE: wrong remainder\n");
   else if (*(parsedurl->family))
      printf("FAILURE: wrong family\n");
   else
      printf("Success!\n");

   if (parsedurl) xfree(parsedurl);

   /* TESTURL6 - invalid ipv6 host */
   printf("Testing srvurl:   %s\n",TESTURL6);
   if (SLPParseSrvUrl(strlen(TESTURL6),
         TESTURL6,
         &parsedurl) != 0)
   {
      printf("FAILURE: SLPParseSrvUrl returned error\n");
      return -1;
   }
   if (strcmp(parsedurl->srvtype,TESTSRVTYPE1))
      printf("FAILURE: wrong srvtype\n");

   if (parsedurl) xfree(parsedurl);

   return 0;
}
#endif

/*=========================================================================*/
