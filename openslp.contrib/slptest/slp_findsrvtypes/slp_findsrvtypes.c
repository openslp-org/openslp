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


#define NUM_BASIC_FIND_STRS 3
const char* basic_find_string[NUM_BASIC_FIND_STRS] =  {"service:service1", 
                                                       "service:service2",
                                                       "service:service3"};

#define NUM_SCOPE_STRS      2
const char* scope_string[NUM_SCOPE_STRS] = {"service:scope1",
                                            "service:scope2"};

#define NUM_NA_STRS         2
const char* na_string[NUM_NA_STRS] =   {"service:na1.TESTNA",
                                        "service:na2.TESTNA"};

#define NUM_SP_STRS         2
const char* sp_string[NUM_SP_STRS] =  {"service:helloñÑàèâûñÑhello",
                                       "service:@#$%&^!Èl Câpitàn¿?Ç¿¡&%$"};



//No parameters can be null because they aren't checked.
int doTest(SLPHandle hslp, 
           const char * namingAuth, 
           const char* scopelist,  
           const char* testid, 
           void* cookie) 
{
    SLPResultCookie* myCookie = (SLPResultCookie*)cookie;
    int result;
    result = SLPFindSrvTypes(hslp,
                    namingAuth,
                    scopelist,
                    theSrvtypesCallback,
                    cookie);
    if ( (myCookie->retval == myCookie->expectedRetval) ) {
        printf("SLPFindSrvTypes\t(%*s): %10s\n", 
               TEST_TYPE_PADDING, testid, "PASSED");
    }
    else {
        printf("SLPFindSrvTypes\t(%*s): %12s\n", 
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
    
    if (SLPOpen("en", SLP_FALSE, &hslp) == SLP_OK) {
        //Do a basic check to see if findslpsrvs is working at all
        setupResultCookie(&resultCookie, 
                          NUM_BASIC_FIND_STRS, 
                          basic_find_string,
                          SLP_OK); 
        result = doTest(hslp, 
                        "*",
                        "", 
                        "basic find pass one (default)", // <-- Test id string 
                        &resultCookie);

        //Do a basic check to see if findslpsrvs is working at all
        setupResultCookie(&resultCookie, 
                          NUM_SCOPE_STRS, 
                          scope_string,
                          SLP_OK); 
        result = doTest(hslp, 
                        "*",
                        "", 
                        "basic find pass two (testscope)", // <-- Test id string 
                        &resultCookie);

        //Do a basic check to see if findslpsrvs is working at all
        setupResultCookie(&resultCookie, 
                          NUM_NA_STRS, 
                          na_string,
                          SLP_OK); 
        result = doTest(hslp, 
                        "*",
                        "", 
                        "basic find pass three (naming authorities)", // <-- Test id string 
                        &resultCookie);

        //Do a basic check to see if findslpsrvs is working at all
        setupResultCookie(&resultCookie, 
                          NUM_SP_STRS, 
                          sp_string,
                          SLP_OK); 
        result = doTest(hslp, 
                        "*",
                        "", 
                        "basic find pass four (spanish scope)", // <-- Test id string 
                        &resultCookie);


        //Now see if you can find services in a particular scope
        // memset(&resultCookie, 0, sizeof(resultCookie));
        setupResultCookie(&resultCookie, NUM_SCOPE_STRS, scope_string, SLP_OK);
        result = doTest(hslp, 
                        "*", 
                        "testscope", 
                        "find in scope", // <-- Test id string 
                        &resultCookie);

        //Now see if you can find services by a particular Naming Authority
        setupResultCookie(&resultCookie, NUM_NA_STRS, na_string, SLP_OK);
        result = doTest(hslp, 
                        "TESTNA", 
                        "testscope", 
                        "find by NA (testscope scope)", // <-- Test id string 
                        &resultCookie);

        //Now find a service with odd characters in the name
        setupResultCookie(&resultCookie, NUM_SP_STRS, sp_string, SLP_OK);
        result = doTest(hslp, 
                        "*", 
                        "spanish", 
                        "find service with spanish chars in spanish scope", // <-- Test id string 
                        &resultCookie);

        //Now see if you can find services by a particular Naming Authority
        setupResultCookie(&resultCookie, NUM_NA_STRS, na_string, SLP_OK);
        result = doTest(hslp, 
                        "TESTNA", 
                        "spanish", 
                        "find by NA(spanish scope)", // <-- Test id string 
                        &resultCookie);

        //Now see if you can find services by a particular Naming Authority
        setupResultCookie(&resultCookie, NUM_NA_STRS, na_string, SLP_OK);
        result = doTest(hslp, 
                        "TESTNA", 
                        "", 
                        "find by NA(configured scopes)", // <-- Test id string
                        &resultCookie);



        SLPClose(hslp);
    }
    else {
        printf("CRITICAL ERROR: SLPOpen failed!\n");
    }

    return result;
}
