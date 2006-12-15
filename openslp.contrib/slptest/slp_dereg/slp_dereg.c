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


#define NUM_BASIC_REG_STRS 3
const char* basic_reg_string[NUM_BASIC_REG_STRS] =   { "service:newService1://100.100.100.1", 
                                                       "service:newService2://200.200.200.2",
                                                       "service:newService3://300.300.300.3"};

#define NUM_SCOPE_STRS      2
const char* scope_string[NUM_SCOPE_STRS] = {"service:scope1",
                                            "service:scope2"};

#define NUM_NA_STRS         2
const char* na_string[NUM_NA_STRS] =   {"service:na1.TESTNA",
                                        "service:na2.TESTNA"};



//No parameters can be null because they aren't checked.
int doTest(SLPHandle hslp, 
           const char *     srvurl, 
           const char*      testid,
           void*            cookie)
{
    SLPResultCookie* myCookie = (SLPResultCookie*)cookie;
    int result;
    result = SLPDereg( hslp, 
                     srvurl, 
                     theRegCallback, 
                     cookie ); 
    
    if ( myCookie->retval == myCookie->retval ) {
        printf("SLPDereg\t(%*s): %10s\n", 
               TEST_TYPE_PADDING, testid, "PASSED");
    }
    else {
        printf("SLPDereg\t(%*s): %12s\n", 
               TEST_TYPE_PADDING, testid, "**FAILED**");
        printf("\tReturn Value:\t\t%s\n\tExpected:\t\t%s\n\tMessage:\t\t%s\n", 
               errcodeToStr(myCookie->retval), 
               errcodeToStr(myCookie->expectedRetval),
               myCookie->msg);
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
    int i;

    if (SLPOpen("en", SLP_FALSE, &hslp) == SLP_OK) {
        //Do a basic check to see if findslpsrvs is working at all
        
        setupResultCookie(&resultCookie, 
                          NUM_BASIC_REG_STRS, 
                          basic_reg_string,
                          SLP_OK); 
        for (i = 0; i < NUM_BASIC_REG_STRS; i++ ) {
            result = doTest(hslp, 
                        basic_reg_string[i],
                        "basic service de-registration",
                        &resultCookie);
        }

        SLPClose(hslp);
    }
    else {
        printf("CRITICAL ERROR: SLPOpen failed!\n");
    }

    return result;
}
