/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_parse.c                                                */
/*                                                                         */
/* Abstract:    Common string parsing functionality                        */
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

#include "slp_parse.h"
#include "slp_xmalloc.h"

#include <errno.h>

/*=========================================================================*/
int SLPParseSrvUrl(int srvurllen,
                   const char* srvurl,
                   SLPParsedSrvUrl** parsedurl)
/*                                            
 * Description:
 *    Parses a service URL into its parts                             
 *
 * Parameters:
 *    srvurllen (IN) size of srvurl in bytes
 *    srvurl    (IN) pointer to service URL to parse
 *    parsedurl (OUT) pointer to SLPParsedSrvUrl pointer that will be
 *                    set to xmalloc()ed parsed url parts.  Returned
 *                    pointer must be freed by caller with call to xfree() 
 *                                                                         
 * Returns: zero on success, errno on failure..                            
 *=========================================================================*/
{
    char*   empty;   /* always points to an empty string */
    char*   buf;     /* points to location in the SLPSrvUrl buffer */
    char*   slider1; /* points to parse locations in srvurl */
    char*   slider2; /* points to parse locations srvurl */
    char*   end;     /* points at the end of srvurl */
    
    /* Allocate memory and set up sliders */
    *parsedurl = (SLPParsedSrvUrl*)xmalloc(srvurllen + sizeof(SLPParsedSrvUrl) + 5);
    /* +5 ensures space for 5 null terminations */
    if(*parsedurl == 0)
    {
        return ENOMEM;
    }
    memset(*parsedurl,0,srvurllen + sizeof(SLPParsedSrvUrl) + 5);    
    buf = ((char*)*parsedurl) + sizeof(SLPParsedSrvUrl);
    slider1 = slider2 = (char*)srvurl;
    end = slider1 + srvurllen;

    /* Set empty */
    empty = buf;
    buf++;
     
    /* parse out the service type */
    slider2 = (char*)strstr(slider2,":/");
    if(slider2 == 0)
    {
        xfree(*parsedurl);
        *parsedurl = 0;
        return EINVAL;
    }    
    memcpy(buf,slider1,slider2-slider1);
    (*parsedurl)->srvtype = buf;
    buf += (slider2 - slider1) + 1;

    /* parse out the host */
    slider2 = slider1 = slider2 + 3; /* + 3 skips the "://" */
    while(slider2 < end && *slider2 != '/' && *slider2 != ':') slider2++;
    if(slider2-slider1 < 1)
    {
        /* no host part (this is okay according to RFC2609) */
        (*parsedurl)->host = empty;
    }
    else
    {
        memcpy(buf,slider1,slider2-slider1);
        (*parsedurl)->host = buf;
        buf += (slider2 - slider1) + 1;
    }

    /* parse out the port */
    if(*slider2 == ':')
    {
        slider3 = slider2 = slider3 + 1; /* + 1 skips the ":" */
        while(*slider3 && (*slider3 != '/' && *slider3 != ';')) slider3++;
        memcpy(buf,slider1,slider2-slider1);
        (*parsedurl)->port = atoi(buf);
        buf += (slider2-slider1) + 1;
    }

    /* parse out the remainder of the url */
    if(slider2 < end)
    {
        slider1 = slider2;
        memcpy(buf,slider1,end-slider1);
        (*parsedurl)->remainder = buf;
        buf += (end - slider2) + 1;
    }
    else
    {
        /* no remainder portion */
        (*parsedurl)->remainder = empty;
    }

    /* set  the net family to always be an empty string for IP */
    (*parsedurl)->family = empty;

    return 0;
}


/*===========================================================================
 * TESTING CODE enabled by removing #define comment and compiling with the 
 * following command line:
 *
 * $ gcc -g -DSLP_PARSE_TEST -DDEBUG slp_parse.c slp_xmalloc.c slp_linkedlist.c
 *==========================================================================*/
#ifdef SLP_PARSE_TEST 

#define TESTSRVTYPE1    "service:printer.x"
#define TESTHOST1       "192.168.100.2"
#define TESTPORT1       "4563"
#define TESTREMAINDER1  "/hello/good/world"

#define TESTURL1        TESTSRVTYPE1"://"TESTHOST1":"TESTPORT1 TESTREMAINDER1
#define TESTURL2        TESTSRVTYPE1"://"TESTHOST1":"TESTPORT1
#define TESTURL3        TESTSRVTYPE1"://"TESTHOST1
#define TESTURL4        "service:badurl"

int main(int argc, char* argv[])
{
    SLPParsedSrvUrl* parsedurl;
    
    printf("Testing srvurl:   %s\n",TESTURL1);
    if(SLPParseSrvUrl(strlen(TESTURL1),
                      TESTURL1,
                      &parsedurl) != 0)
    {
        printf("FAILURE: SLPParseSrvUrl returned error\n");
        return -1;
    }
    if(strcmp(parsedurl->srvtype,TESTSRVTYPE1))
    {
        printf("FAILURE: wrong srvtype\n");
    }
    else if(strcmp(parsedurl->host,TESTHOST1))
    {
        printf("FAILURE: wrong host\n");
    }
    else if(parsedurl->port != atoi(TESTPORT1))
    {
        printf("FAILURE: wrong port\n");
    }
    else if(strcmp(parsedurl->remainder,TESTREMAINDER1))
    {
        printf("FAILURE: wrong remainder\n");
    }
    else if(*(parsedurl->family))
    {
           printf("FAILURE: wrong family\n");
    }
    else
    {
        printf("Success!\n");
    }
    if(parsedurl) xfree(parsedurl);

    /* TESTURL2 */
    printf("Testing srvurl:   %s\n",TESTURL2);
    if(SLPParseSrvUrl(strlen(TESTURL2),
                      TESTURL2,
                      &parsedurl) != 0)
    {
        printf("FAILURE: SLPParseSrvUrl returned error\n");
        return -1;
    }
    if(strcmp(parsedurl->srvtype,TESTSRVTYPE1))
    {
        printf("FAILURE: wrong srvtype\n");
    }
    else if(strcmp(parsedurl->host,TESTHOST1))
    {
        printf("FAILURE: wrong host\n");
    }
    else if(parsedurl->port != atoi(TESTPORT1))
    {
        printf("FAILURE: wrong port\n");
    }
    /*
    else if(strcmp(parsedurl->remainder,TESTREMAINDER1))
    {
        printf("FAILURE: wrong remainder\n");
    }
    */
    else if(*(parsedurl->family))
    {
           printf("FAILURE: wrong family\n");
    }
    else
    {
        printf("Success!\n");
    }
    if(parsedurl) xfree(parsedurl);

    /* TESTURL3 */
    printf("Testing srvurl:   %s\n",TESTURL3);
    if(SLPParseSrvUrl(strlen(TESTURL3),
                      TESTURL3,
                      &parsedurl) != 0)
    {
        printf("FAILURE: SLPParseSrvUrl returned error\n");
        return -1;
    }
    if(strcmp(parsedurl->srvtype,TESTSRVTYPE1))
    {
        printf("FAILURE: wrong srvtype\n");
    }
    else if(strcmp(parsedurl->host,TESTHOST1))
    {
        printf("FAILURE: wrong host\n");
    }
    /*
    else if(parsedurl->port != atoi(TESTPORT1))
    {
        printf("FAILURE: wrong port\n");
    }
    else if(strcmp(parsedurl->remainder,TESTREMAINDER1))
    {
        printf("FAILURE: wrong remainder\n");
    }
    */
    else if(*(parsedurl->family))
    {
           printf("FAILURE: wrong family\n");
    }
    else
    {
        printf("Success!\n");
    }
    if(parsedurl) xfree(parsedurl);
    

    /* TESTURL4 */
    printf("Testing srvurl:   %s\n",TESTURL4);
    if(SLPParseSrvUrl(strlen(TESTURL4),
                      TESTURL4,
                      &parsedurl) == 0)
    {
        printf("FAILURE: SLPParseSrvUrl should have returned an error\n");
        
    }
    else
    {
        printf("Success!\n");
    }
    if(parsedurl) xfree(parsedurl);
    
    return 0;
}
#endif

