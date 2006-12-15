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

/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP automated test suite                               */
/*                                                                         */
/* File:        slp_dereg.c                                                */
/*                                                                         */
/* Abstract:    Tests SLPDereg API                                         */
/*                                                                         */
/* Requires:    OpenSLP installation                                       */
/*                                                                         */
/* Author(s):   Cody Batt   <cbatt@caldera.com>                            */
/*                                                                         */
/***************************************************************************/

#include "../slp_testcommon.h"


#define NUM_BASIC_SRVFIND_STRS 1
const char* basic_srvfind_string[NUM_BASIC_SRVFIND_STRS] =  {"service:service1"};

#define NUM_SCOPE_STRS      2
const char* scope_string[NUM_SCOPE_STRS] = { "service:scope1://192.168.100.4",
                                           "service:scope1://193.168.100.4"};

#define NUM_NA_STRS         2
const char* na_string[NUM_NA_STRS] = { "service:na1.TESTNA",
                                       "service:na2.TESTNA"};

#define NUM_PR_STRS         1
const char* pr_string[NUM_PR_STRS]              = { "service:slp_test_predicate"};
const char* pr_xy_string[NUM_PR_STRS]           = {"service:slp_test_predicate://10.10.10.1"};
const char* pr_foobar_string[NUM_PR_STRS]       = {"service:slp_test_predicate://10.10.10.2"};
const char* pr_qspeed_string[NUM_PR_STRS]       = {"service:slp_test_predicate://10.10.10.3"};
const char* pr_wild_string[NUM_PR_STRS]         = {"service:slp_test_predicate://10.10.10.4"};
const char* pr_presence_string[NUM_PR_STRS]     = {"service:slp_test_predicate://10.10.10.5"};

//No parameters can be null because they aren't checked.
int doTest(SLPHandle hslp, 
           const char* srvtype, 
           const char* scopelist,
           const char* LDAPfilter,
           const char* testid, 
           void* cookie) 
{
    SLPResultCookie* myCookie = (SLPResultCookie*)cookie;
    int result;
    result = SLPFindSrvs(hslp,
                    srvtype,
                    scopelist,
                    LDAPfilter,
                    theSrvsCallback,
                    cookie);
    if ( myCookie->retval == myCookie->expectedRetval ) {
        printf("SLPFindSrvs\t(%*s): %10s\n", 
               TEST_TYPE_PADDING, testid, "PASSED");
    }
    else {
        printf("SLPFindSrvs\t(%*s): %12s\n", 
               TEST_TYPE_PADDING, testid, "**FAILED**");
        printf("\tReturn Value:\t\t%s\n\tExpected:\t\t%s",
               errcodeToStr(myCookie->retval),
               errcodeToStr(myCookie->expectedRetval)); 
        printf("\n\tMessage:\t\t%s\n",myCookie->msg);
    }

    return result;
}

/*=========================================================================*/
int main(int argc, char* argv[]) 

/*=========================================================================*/
{
    SLPError    result = SLP_OK;
    SLPHandle   hslp;
    SLPResultCookie resultCookie;
    char hostname[MAX_RESULT_MSG_SIZE];
    void* hostPtr;
    if (SLPOpen("en", SLP_FALSE, &hslp) == SLP_OK) {
        //Do a basic check to see if findslpsrvs is working at all
        setupResultCookie(&resultCookie, 
                          NUM_BASIC_SRVFIND_STRS, 
                          basic_srvfind_string,
                          SLP_OK);
        result = doTest(hslp, 
                        "service:service1", 
                        0, 
                        0 , 
                        "basic find service (service:service1)", 
                        &resultCookie);

        //Do a basic check to see if findslpsrvs is working at all
        setupResultCookie(&resultCookie, 
                          NUM_BASIC_SRVFIND_STRS, 
                          basic_srvfind_string,
                          SLP_OK);
        result = doTest(hslp, 
                        "service1", 
                        0, 
                        0 , 
                        "basic find service (service1)", 
                        &resultCookie);

        //Find services in a scope
        setupResultCookie(&resultCookie, NUM_SCOPE_STRS, scope_string,
                          SLP_OK);
        result = doTest(hslp, 
                        "service:scope1", 
                        "testscope", 
                        0, 
                        "find in scope", 
                        &resultCookie);

        //Try to find a non-existent service -- should return OK
        setupResultCookie(&resultCookie, NUM_SCOPE_STRS, scope_string,
                          SLP_OK);
        result = doTest(hslp, 
                        "service:slp_test_scopeydopey", 
                        "someunknownandneverusedscope", 
                        0, 
                        "non-existent service", 
                        &resultCookie);

        //Try to find a service by ldap filter (x=3)
        setupResultCookie(&resultCookie,
                          NUM_PR_STRS, 
                          pr_xy_string,
                          SLP_OK);
        result = doTest(hslp,
                        "service:slp_test_predicate",
                        0,
                        "(x=3)",
                        "find by predicate (x=3)",
                        &resultCookie);

        //Try to find a service by ldap filter (!(y=0))
        setupResultCookie(&resultCookie,
                          NUM_PR_STRS, 
                          pr_xy_string,
                          SLP_OK);
        result = doTest(hslp,
                        "service:slp_test_predicate",
                        0,
                        "(!(y=0))",
                        "find by predicate (!(y=0))",
                        &resultCookie);

        //Try to find a service by ldap filter (presence=*)
        setupResultCookie(&resultCookie,
                          NUM_PR_STRS, 
                          pr_presence_string,
                          SLP_OK);
        result = doTest(hslp,
                        "service:slp_test_predicate",
                        0,
                        "(presence=*)",
                        "find by predicate (presence=*)",
                        &resultCookie);

        //Try to find a service by ldap filter (foo=BAR)
        setupResultCookie(&resultCookie,
                          NUM_PR_STRS, 
                          pr_foobar_string,
                          SLP_OK);
        result = doTest(hslp,
                        "service:slp_test_predicate",
                        0,
                        "(foo=BAR)",
                        "find by predicate (foo=BAR)",
                        &resultCookie);

        //Try to find a service by ldap filter (foo=bar)
        setupResultCookie(&resultCookie,
                          NUM_PR_STRS, 
                          pr_foobar_string,
                          SLP_OK);
        result = doTest(hslp,
                        "service:slp_test_predicate",
                        0,
                        "(foo=bar)",
                        "find by predicate (foo=bar)",
                        &resultCookie);
        
        //Try to find a service by ldap filter (bar=FOO)
        setupResultCookie(&resultCookie,
                          NUM_PR_STRS, 
                          pr_foobar_string,
                          SLP_OK);
        result = doTest(hslp,
                        "service:slp_test_predicate",
                        0,
                        "(bar=FOO)",
                        "find by predicate (bar=FOO)",
                        &resultCookie);
        
        //Try to find a service by ldap filter (|(bar=FOO)(bar=27))
        setupResultCookie(&resultCookie,
                          NUM_PR_STRS, 
                          pr_foobar_string,
                          SLP_OK);
        result = doTest(hslp,
                        "service:slp_test_predicate",
                        0,
                        "(|(bar=FOO)(bar=27))",
                        "find by predicate (|(bar=FOO)(bar=27))",
                        &resultCookie);

        //Try to find a service by ldap filter (wild=23*)
        setupResultCookie(&resultCookie,
                          NUM_PR_STRS, 
                          pr_wild_string,
                          SLP_OK);
        result = doTest(hslp,
                        "service:slp_test_predicate",
                        0,
                        "(wild=23*)",
                        "find by predicate (wild=23*)",
                        &resultCookie);

        //Try to find a service by ldap filter (nonwild=23*)
        setupResultCookie(&resultCookie,
                          NUM_PR_STRS, 
                          pr_string,
                          SLP_TEST_MISMATCH);
        result = doTest(hslp,
                        "service:slp_test_predicate",
                        0,
                        "(nonwild=23*)",
                        "find by predicate (nonwild=23*)",
                        &resultCookie);

        //Try to find a service by ldap filter (&(q<=3)(speed>=100))
        setupResultCookie(&resultCookie,
                          NUM_PR_STRS, 
                          pr_qspeed_string,
                          SLP_OK);
        result = doTest(hslp,
                        "service:slp_test_predicate",
                        0,
                        "(&(q<=3)(speed>=100))",
                        "find by predicate (&(q<=3)(speed>=100))",
                        &resultCookie);

        //Try to find a service by ldap filter (nopresence=*)
        setupResultCookie(&resultCookie,
                          NUM_PR_STRS, 
                          pr_string,
                          SLP_TEST_MISMATCH);
        result = doTest(hslp,
                        "service:slp_test_predicate",
                        0,
                        "(nopresence=*)",
                        "find by predicate (nopresence=*)",
                        &resultCookie);

        gethostname(hostname, MAX_RESULT_MSG_SIZE);
        hostPtr = &hostname;
        if ( slpdRunningLocal() == SLP_TRUE ) {
            if ( SLPGetProperty("net.slp.isDA") &&
                 strcmp(SLPGetProperty("net.slp.isDA"), "true") == 0) 
            {
                /* Running as a DA, put in DA specific tests here
                */
                printf("SLPD configured as DA on %s. Running DA tests.\n", hostname);
                setupResultCookie(&resultCookie, 1, (const char**)&hostPtr, SLP_OK);
                result = doTest(hslp, 
                            "service:directory-agent", 
                            0, 
                            0, 
                            "find unique service service:directory-agent", 
                            &resultCookie);
            }
            else 
            {
                /* Running as an SA, put in SA specific tests here
                */
                printf("SLPD configured as SA on %s. Running SA tests.\n", hostname);
                setupResultCookie(&resultCookie, 1, (const char**)&hostPtr, SLP_OK);
                result = doTest(hslp, 
                        "service:service-agent", 
                        0, 
                        0, 
                        "find unique service service:service-agent", 
                        &resultCookie);
            }
        }
        else {
            /* Running as a UA, put in UA specific tests here
            */
            printf("SLPD is not running locally. Running UA tests.\n");

        }
        
        
        SLPClose(hslp);
        
    }
    else {
        printf("CRITICAL ERROR: SLPOpen failed!\n");
    }

    return result;
}
