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
/* File:        slp_reg.c                                                  */
/*                                                                         */
/* Abstract:    Tests SLPReg API                                           */
/*                                                                         */
/* Requires:    OpenSLP installation                                       */
/*                                                                         */
/* Author(s):   Cody Batt   <cbatt@caldera.com>                            */
/*              John Bowers <jbowers@caldera.com                           */
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

#define NUM_STRING_ATTRS 2
const char* string_attrs_reg[NUM_STRING_ATTRS] = {"(today=holiday)",
                                                  "(tommorrow=3session5537)"};
const char* string_attrs_strings[NUM_STRING_ATTRS]={"service:string:normal.string://192.168.5.26",
                                                    "service:string:alphanumeric.string://192.168.5.25"};

#define NUM_BOOL_ATTRS 1  
const char* bool_attrs_reg[NUM_BOOL_ATTRS]={"(help=false)"};
const char* bool_attrs_strings[NUM_BOOL_ATTRS]={"service:boolean:only://192.168.5.27"};

#define NUM_INT_ATTRS 2
const char* int_attrs_strings[NUM_INT_ATTRS] = {"service:integer:large.int://192.168.5.29",
                                                "service:integer:small.int://192.168.5.28"};
const char* int_attrs_reg[NUM_INT_ATTRS]={"(hugeint=22000000000)",
                                          "(smallint=13)"};
#define NUM_INVALID_REG_ATTRS 1
const char* mix_attrs_reg[NUM_INVALID_REG_ATTRS] = {"(mixed=4,true,sue)"};
const char* mix_attrs_strings[NUM_INVALID_REG_ATTRS] = {"service:mixed:one://192.168.5.24"};

#define NUM_PARSE_ERR_ATTRS 1
const char* parse_err_reg[NUM_PARSE_ERR_ATTRS] = {"(parse=hello\\e error \\m)"};
const char* parse_err_strings[NUM_PARSE_ERR_ATTRS] = {"service:parse.error://192.168.5.23"};

#define NUM_INVALID_LIFETIME 1
const char* invalid_lifetime_strings[NUM_INVALID_LIFETIME] = {"service:invalid:liftime://192.168.5.22"};

#define NUM_INVALID_URL 1
const char* invalid_url_strings[NUM_INVALID_URL] = {"service:invalid:url//192.168.5.21"};

//No parameters can be null because they aren't checked.
int doTest(SLPHandle hslp, 
           const char *     srvurl, 
           unsigned short   lifetime,  
           const char*      srvtype, 
           const char*      attrs,
           SLPBoolean       fresh,
           const char*      testid,
           void*            cookie)
{
    SLPResultCookie* myCookie = (SLPResultCookie*)cookie;
    int result;
    result = SLPReg( hslp, 
                     srvurl, 
                     lifetime, 
                     srvtype, //Ignored if srvurl in form service:foo 
                     attrs, 
                     fresh, 
                     theRegCallback, 
                     cookie );
    if(myCookie->retval != result)
        myCookie->retval=result;

    //In some cases, the callback is never called and the return value is never set
    //this will check for that and set the result to what the test expects to see
    if ( strncmp(myCookie->msg, SLP_TEST_NOCALLBACK_STR, strlen(SLP_TEST_NOCALLBACK_STR)) == 0 ) {
        if (myCookie->expectedRetval == SLP_TEST_NOCALLBACK) {
            myCookie->retval = SLP_TEST_NOCALLBACK; 
        }
    }

    if ( myCookie->retval == myCookie->expectedRetval ) {
        printf("SLPReg   \t(%*s): %10s\n", 
               TEST_TYPE_PADDING, testid, "PASSED");
    }
    else {
        printf("SLPReg   \t(%*s): %12s\n", 
               TEST_TYPE_PADDING, testid, "**FAILED**");
        printf("\tReturn Value:\t\t%s\n\tExpected:\t\t%s",errcodeToStr(myCookie->retval),errcodeToStr(myCookie->expectedRetval)); 
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
                        65000, 
                        "myNewService",
                        "",
                        SLP_TRUE,
                        "basic service registration",
                        &resultCookie);
        }
        //Register services with basic string attributes
        for (i = 0; i < NUM_STRING_ATTRS; i++ ) {
            result = doTest(hslp, 
                        string_attrs_strings[i],
                        100, 
                        0,
                        string_attrs_reg[i],
                        SLP_TRUE,
                        "Register service with string attribute",
                        &resultCookie);
        }
        //Register services with integer attributes
        for (i = 0; i < NUM_INT_ATTRS; i++ ) {
            result = doTest(hslp, 
                        int_attrs_strings[i],
                        100, 
                        0,
                        int_attrs_reg[i],
                        SLP_TRUE,
                        "Register service with integer attribute",
                        &resultCookie);
        }
        //Register services with boolean attributes
        for (i = 0; i < NUM_BOOL_ATTRS; i++ ) {
            result = doTest(hslp, 
                        bool_attrs_strings[i],
                        100, 
                        0,
                        bool_attrs_reg[i],
                        SLP_TRUE,
                        "Register service with boolean attribute",
                        &resultCookie);
        }
        //Register services with Opaque attributes
        
        //Register services with attributes that have boolean values, string values, opaque values and integer values all for the same
        //registraion.  Should return an SLP_INVALID_REGISTRATION
        for (i = 0; i < NUM_INVALID_REG_ATTRS; i++ ) {
            setupResultCookie(&resultCookie,0,0,SLP_TEST_NOCALLBACK);
            result = doTest(hslp, 
                        mix_attrs_strings[i],
                        65535, 
                        0,
                        mix_attrs_reg[i],
                        SLP_TRUE,
                        "Reg service w/ invalid attr(inconsistent value list)",
                        &resultCookie);
        }
        //Try an register a service with attributes which contain escaped characters which are not on the reserved list.  This
        //should return an slp parse error
        //#define NUM_PARSE_ERR_ATTRS 1
        //const char* parse_err_reg[NUM_PARSE_ERR_ATTRS] = {"(parse=hello\\e error \\m)"};
        //const char* parse_err_strings[NUM_PARSE_ERR_ATTRS] = {"service:parse.error://192.168.5.23"};
        for (i = 0; i < NUM_PARSE_ERR_ATTRS; i++ ) {
            setupResultCookie(&resultCookie,0,0,SLP_TEST_NOCALLBACK);
            result = doTest(hslp, 
                        parse_err_strings[i],
                        100, 
                        0,
                        parse_err_reg[i],
                        SLP_TRUE,
                        "Reg service w/ invalid attr(unreserved escaped characters)",
                        &resultCookie);
        }
        //Try registering a service with and invalid liftetime (Zero or negative number)
        for (i = 0; i < NUM_INVALID_LIFETIME; i++ ) {
            setupResultCookie(&resultCookie,0,0,SLP_TEST_NOCALLBACK);
            result = doTest(hslp, 
                        invalid_lifetime_strings[i],
                        0, 
                        0,
                        "",
                        SLP_TRUE,
                        "Register service with invalid lifetime",
                        &resultCookie);
        }
        //Register a service with invalid service URL syntax
        for (i = 0; i < NUM_INVALID_URL; i++ ) {
            setupResultCookie(&resultCookie,0,0,SLP_INVALID_REGISTRATION);
            result = doTest(hslp, 
                        invalid_url_strings[i],
                        100, 
                        0,
                        "",
                        SLP_TRUE,
                        "Register service with invalid Service URL",
                        &resultCookie);
        }
        
        SLPClose(hslp);
    }
    else {
        printf("CRITICAL ERROR: SLPOpen failed!\n");
    }

    return result;
}
