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
