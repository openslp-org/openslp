/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP automated test suite                               */
/*                                                                         */
/* File:        slp_findsrvtypes.c                                         */
/*                                                                         */
/* Abstract:    Tests SLPFindSrvTypes API                                  */
/*                                                                         */
/* Requires:    OpenSLP installation                                       */
/*                                                                         */
/* Author(s):   Cody Batt   <cbatt@caldera.com>                            */
/*                                                                         */
/* Copyright (c) 1995, 2001  Caldera Systems, Inc.                         */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify it */
/* under the terms of the GNU Lesser General Public License as published   */
/* by the Free Software Foundation; either version 2.1 of the License, or  */
/* (at your option) any later version.                                     */
/*                                                                         */
/*     This program is distributed in the hope that it will be useful,     */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       */
/*     GNU Lesser General Public License for more details.                 */
/*                                                                         */
/*     You should have received a copy of the GNU Lesser General Public    */
/*     License along with this program; see the file COPYING.  If not,     */
/*     please obtain a copy from http://www.gnu.org/copyleft/lesser.html   */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*     Please submit patches to maintainer of http://www.openslp.org       */
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
