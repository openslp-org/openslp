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
 * @author     John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeParse
 */

#include "slp_types.h"
#include "slp_parse.h"
#include "slp_xmalloc.h"

/** Parse a service URL.
 *
 * RFC 2614 (Service Location API) indicates this routine is destructive to 
 * @p srvurl. Not so with this version: A copy of @p srvurl is made at the 
 * end of the allocated return buffer, and fields in the structure refer
 * to zero terminated portions of this copy. However, it should be noted that
 * in order to implement this code as specified in the RFC, the s_pcSrvPart 
 * field must begin with the character following the delimiting '/', because
 * it would have been necessary to terminate the s_pcHost field by writing a 
 * 0 into this position, in such an implementation. 
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
 *
 * @note Not returning the leading '/' character in the s_pcSrvPart field
 *    will break backward compatibility with existing OpenSLP 1.x code. This 
 *    may cause problems in clients that expected to be able to simply paste 
 *    all of the parts together without supplying any extra delimiters.
 */
int SLPParseSrvUrl(size_t srvurllen, const char * srvurl,
      SLPParsedSrvUrl ** parsedurl)
{
   char * slider1;         /* sliders are buffer walking variables */
   char * slider2;
   char * endptr;          /* points to end of url copy buffer */

   /* define net family strings */
#  define SLP_NF_IP        ""
#  define SLP_NF_IPv6      "v6"
#  define SLP_NF_AT        "at"
#  define SLP_NF_IPX       "ipx"
#  define SLP_NF_MAX_SIZE  sizeof(SLP_NF_IPX)   /* longest NF */

   /* allocate space for structure + srvurl + longest net family tag */
   *parsedurl = (SLPParsedSrvUrl *)xmalloc(sizeof(SLPParsedSrvUrl) 
         + SLP_NF_MAX_SIZE    /* long enough for longest net family */
         + srvurllen + 1);    /* +1 for null-terminator */
   if (*parsedurl == 0)
      return ENOMEM;

   /* point to family tag buffer, copy url to buffer space */
   (*parsedurl)->family = (char *)*parsedurl + sizeof(SLPParsedSrvUrl);
   slider1 = slider2 = (*parsedurl)->family + SLP_NF_MAX_SIZE;
   memcpy(slider1, srvurl, srvurllen);

   /* find end and terminate url copy */
   endptr = slider1 + srvurllen;
   *endptr = 0;

   /* parse out the service type */
   (*parsedurl)->srvtype = slider1;
   slider2 = strstr(slider1, "://");
   if (slider2 == 0)
   {
      xfree(*parsedurl);
      *parsedurl = 0;
      return EINVAL;       /* ill-formatted URL - missing "://" */
   }
   *slider2 = 0;           /* terminate service type at ':' */
   slider1 = slider2 + 3;  /* skip delimiters "://" */

   /* parse out the host */
   /* check for IPv6 numeric address with [] surrounding the host */
   (*parsedurl)->host = slider1;
   if (*slider1 == '[' && (slider2 = strchr(slider1, ']')) != 0)
   {
      (*parsedurl)->host++;   /* skip '[' */
      *slider2++ = 0;
      slider1 = slider2;
      strcpy((*parsedurl)->family, SLP_NF_IPv6);
   }
   else                       /* assume IP for now */
      *(*parsedurl)->family = 0;

   /** @todo Parse ipx and appletalk addresses also (format?). */

   /* parse out remainder */
   slider2 = strchr(slider1, '/');
   if (slider2)
   {
      *slider2++ = 0;      /* terminate port or host */
      (*parsedurl)->remainder = slider2;
   }
   else
      (*parsedurl)->remainder = endptr;

   /* parse out port */
   slider2 = strchr(slider1, ':');
   if (slider2)
   {
      *slider2++ = 0;      /* terminate host */
      (*parsedurl)->port = atoi(slider2);
   }
   else
      (*parsedurl)->port = 0;

   return 0;
}

/*===========================================================================
 *  TESTING CODE : compile with the following command lines:
 *
 *  $ gcc -g -DSLP_PARSE_TEST -DDEBUG slp_parse.c slp_xmalloc.c 
 *       slp_linkedlist.c
 *
 *  C:\> cl -DSLP_PARSE_TEST -DDEBUG slp_parse.c slp_xmalloc.c 
 *       slp_linkedlist.c
 */
#ifdef SLP_PARSE_TEST 

#define TESTSRVTYPE1 "service:printer.x"
#define TESTHOST1    "192.168.100.2"
#define TESTHOST2    "1111:2222:3333::4444"
#define TESTHOST3    "[dnsname.with.leading.bracket"
#define TESTPORT1    "4563"
#define TESTPATH1    "hello/world"

/* In the following test strings, any delimiters which may be overwritten by
   zero terminators, are quoted literals. Critical return values are defined 
   above, and used as macros below. */

#define TESTURL01 TESTSRVTYPE1 "://"  TESTHOST1  ":" TESTPORT1 "/" TESTPATH1
#define TESTURL02 TESTSRVTYPE1 "://"  TESTHOST1  ":" TESTPORT1
#define TESTURL03 TESTSRVTYPE1 "://"  TESTHOST1                "/" TESTPATH1
#define TESTURL04 TESTSRVTYPE1 "://"  TESTHOST1
#define TESTURL05 TESTSRVTYPE1 "://"
#define TESTURL06 TESTSRVTYPE1 "://[" TESTHOST2 "]:" TESTPORT1 "/" TESTPATH1
#define TESTURL07 TESTSRVTYPE1 "://[" TESTHOST2 "]:" TESTPORT1
#define TESTURL08 TESTSRVTYPE1 "://[" TESTHOST2 "]"            "/" TESTPATH1
#define TESTURL09 TESTSRVTYPE1 "://[" TESTHOST2 "]"
#define TESTURL10 TESTSRVTYPE1 "://"  TESTHOST3  ":" TESTPORT1 "/" TESTPATH1
#define TESTURL11 TESTSRVTYPE1 "://"  TESTHOST3  ":" TESTPORT1
#define TESTURL12 TESTSRVTYPE1 "://"  TESTHOST3                "/" TESTPATH1
#define TESTURL13 TESTSRVTYPE1 "://"  TESTHOST3
#define TESTURL14 TESTSRVTYPE1 "://["
#define TESTURL15 "service:badurl"

/** @todo Add test cases for IPX and appletalk host address strings. */

static struct test_ts
{
   const char * url;
   const char * srvtype;
   const char * host;
   const char * port;
   const char * path;
   const char * family;
   int should_fail;
} test_table[] = 
{
   {TESTURL01, TESTSRVTYPE1, TESTHOST1, TESTPORT1, TESTPATH1, SLP_NF_IP,   0},
   {TESTURL02, TESTSRVTYPE1, TESTHOST1, TESTPORT1, "",        SLP_NF_IP,   0},
   {TESTURL03, TESTSRVTYPE1, TESTHOST1, "",        TESTPATH1, SLP_NF_IP,   0},
   {TESTURL04, TESTSRVTYPE1, TESTHOST1, "",        "",        SLP_NF_IP,   0},
   {TESTURL05, TESTSRVTYPE1, "",        "",        "",        SLP_NF_IP,   0},
   {TESTURL06, TESTSRVTYPE1, TESTHOST2, TESTPORT1, TESTPATH1, SLP_NF_IPv6, 0},
   {TESTURL07, TESTSRVTYPE1, TESTHOST2, TESTPORT1, "",        SLP_NF_IPv6, 0},
   {TESTURL08, TESTSRVTYPE1, TESTHOST2, "",        TESTPATH1, SLP_NF_IPv6, 0},
   {TESTURL09, TESTSRVTYPE1, TESTHOST2, "",        "",        SLP_NF_IPv6, 0},
   {TESTURL10, TESTSRVTYPE1, TESTHOST3, TESTPORT1, TESTPATH1, SLP_NF_IP,   0},
   {TESTURL11, TESTSRVTYPE1, TESTHOST3, TESTPORT1, "",        SLP_NF_IP,   0},
   {TESTURL12, TESTSRVTYPE1, TESTHOST3, "",        TESTPATH1, SLP_NF_IP,   0},
   {TESTURL13, TESTSRVTYPE1, TESTHOST3, "",        "",        SLP_NF_IP,   0},
   {TESTURL14, TESTSRVTYPE1, "[",       "",        "",        SLP_NF_IP,   0},
   {TESTURL15, "service:badurl", "",    "",        "",        SLP_NF_IP,   1},
   {0}
};

int main(int argc, char * argv[])
{
   int test, ec = 0;
   struct test_ts * tp;

   for (test = 1, tp = test_table; tp->url; test++, tp++)
   {
      SLPParsedSrvUrl * parsedurl = 0;
      ec++;    /* assume failure, set it right if we don't fail */
      printf("Test [%2d]: \"%s\"\n", test, tp->url);
      if (SLPParseSrvUrl(strlen(tp->url), tp->url, &parsedurl) == 0)
      {
         if (tp->should_fail)
            printf("FAILURE: SLPParseSrvUrl should have failed.\n");
         else if (strcmp(parsedurl->srvtype, tp->srvtype))
            printf(
               "FAILURE: wrong srvtype: \"%s\"\n"
               "             should be: \"%s\"\n", 
               parsedurl->srvtype, tp->srvtype);
         else if (strcmp(parsedurl->host, tp->host))
            printf(
               "FAILURE: wrong host: \"%s\"\n"
               "          should be: \"%s\"\n", 
               parsedurl->host, tp->host);
         else if (parsedurl->port != atoi(tp->port))
            printf(
               "FAILURE: wrong port: \"%d\"\n"
               "          should be: \"%s\"\n", 
               parsedurl->port, tp->port);
         else if (strcmp(parsedurl->remainder, tp->path))
            printf(
               "FAILURE: wrong remainder: \"%s\"\n"
               "               should be: \"%s\"\n", 
               parsedurl->remainder, tp->path);
         else if (strcmp(parsedurl->family, tp->family))
            printf(
               "FAILURE: wrong family: \"%s\"\n"
               "            should be: \"%s\"\n", 
               parsedurl->family, tp->family);
         else
            ec--;
      }
      else if (!tp->should_fail)
         printf("FAILURE: SLPParseSrvUrl failed.\n");
      else
         ec--;
      xfree(parsedurl);
   }
   printf("Error count: %d.\n", ec);
   return ec? -1: 0;
}
#endif

/*=========================================================================*/
