/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP automated test suite                               */
/*                                                                         */
/* File:        slp_findattrs.c                                            */
/*                                                                         */
/* Abstract:    Tests SLPFindAttrs API                                     */
/*                                                                         */
/* Requires:    OpenSLP installation                                       */
/*                                                                         */
/* Author(s):   John Bowers   <jbowers@caldera.com>                        */
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


#define NUM_BASIC_FIND_ATTRS_STRS  3
const char* basic_findattrs_string[NUM_BASIC_FIND_ATTRS_STRS] ={"description=First", 
                                                                "onlyfirst=here",
                                                                "colors=blue,green,orange"};
#define NUM_FIND_SPECIFIC_ATTRS 2
const char* find_specific_attrs[NUM_FIND_SPECIFIC_ATTRS] = {"name=John Doe",
                                                            "description=Second"};
#define NUM_FIND_ATTRS_ON_SERVICE_TYPE_STRS 3
const char* find_attrs_on_service_types[NUM_FIND_ATTRS_ON_SERVICE_TYPE_STRS] = {"description=#2 in testscope",
                                                                                "onlyinsecond=here",
                                                                                "lastinsecond=here"};
#define NUM_FIND_SPECIFIC_ATTRS_ON_SERVICE_TYPE_SPAN_SCOPES 2
const char* find_specific_attrs_on_service_types_span_scopes[NUM_FIND_SPECIFIC_ATTRS_ON_SERVICE_TYPE_SPAN_SCOPES] = {"description=Second",
                                                                                                                       "name=John Doe"};
#define NUM_FIND_ATTRS_WITH_WILDCARDS 2 
const char* find_attrs_with_wildcards[NUM_FIND_ATTRS_WITH_WILDCARDS] = {"onlyinsecond=here",
                                                                        "lastinsecond=here"};
#define NUM_SPECIAL_CHAR_VALUES 2
const char* special_char_strings[NUM_SPECIAL_CHAR_VALUES] = {"backslash=\\",
                                    "comma=,"};
#define NUM_BOOL_STRINGS 1
const char* bool_strings[NUM_BOOL_STRINGS]={"boolean=true"};

#define NUM_SPECIAL_ATTRS 1
const char* special_attrs_strings[NUM_SPECIAL_ATTRS] = {"XONLY"};

#define NUM_INT_ATTRS 2
const char* int_attr_strings[NUM_INT_ATTRS] = {"daysinyear=365",
                                "yearsindecade=10"};
#define NUM_ALL_UPPER 1
const char* all_upper_strings[NUM_ALL_UPPER] =  {"firstinsecond=here"};

#define NUM_LARGE_NUM 1
const char* large_num_strings[NUM_LARGE_NUM] =  {"hugenumber=22000000000"};

#define NUM_ALL_LOWER 1
const char* all_lower_strings[NUM_ALL_LOWER] =  {"description=First"};

#define NUM_ALPHA_NUMERIC 2
const char* alpha_numeric_strings[NUM_ALPHA_NUMERIC] = {"phone=435-123-4567",
                                                        "alphanumeric=13AbbyRD"};



//No parameters can be null because they aren't checked.
int doTest(SLPHandle hslp, 
           const char* srvurlorsrvtype, 
           const char* scopelist,  
           const char* attrids,
           const char* testid, 
           void* cookie) 
{
    SLPResultCookie* myCookie = (SLPResultCookie*)cookie;
    int result;
    result = SLPFindAttrs(hslp, 
                       srvurlorsrvtype, 
                       scopelist, 
                       attrids, 
                       theAttrsCallback, 
                       cookie);

    if ( myCookie->retval == myCookie->expectedRetval ) {
        printf("SLPFindAttrs\t(%*s): %10s\n", TEST_TYPE_PADDING, testid, "PASSED");
    }
    else {
        printf("SLPFindAttrs\t(%*s): %12s\n", TEST_TYPE_PADDING, testid, "**FAILED**");
        printf("\tReturn Value:\t\t%s\n\tExpected:\t\t%s\n\tMessage:\t\t%s\n", 
               errcodeToStr(myCookie->retval), errcodeToStr(myCookie->expectedRetval), myCookie->msg);
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
        
        //Do a basic check to see if findattrs will find all attributes for a specific service url
        setupResultCookie(&resultCookie, 
                          NUM_BASIC_FIND_ATTRS_STRS, 
                          basic_findattrs_string,
                          SLP_OK);
        result = doTest(hslp,"service:testservice:service1://192.168.100.11", "", "", "Find all attributes of a specific service URL", &resultCookie);

        //Now find specific attributes for a specific service url
        setupResultCookie(&resultCookie, 
                          NUM_FIND_SPECIFIC_ATTRS, 
                          find_specific_attrs,
                          SLP_OK);
        result = doTest(hslp, "service:testservice:service2://192.168.100.12", "", "name,description", "Find specific attributes of a specific service URL",  &resultCookie);

        //Now find all attributes for a particular service Type
        //this may need to be changed in the future seeing as how the RFC documents one way that this should work, but
        //it is reported that the RFC will be updated to reflect the way openslp works in the next revision.
        //The way openslp works is that it will return the attributes for the LAST matching service.  See if this is working. 
        setupResultCookie(&resultCookie, 
                          NUM_FIND_ATTRS_ON_SERVICE_TYPE_STRS, 
                          find_attrs_on_service_types,
                          SLP_OK);
        result = doTest(hslp, "service:testservice", "", "", "Find attributes based on a URL type",  &resultCookie);
        
        //NOTE: THis is an apparent misuse of findattrs, so it is no longer tested as of 0.9.1
        //Now find 2 specific attributes for a service type.  Both of these attributes exist, but they are exist on service on 2 different
        //supported scopes.
        //setupResultCookie(&resultCookie, 
        //                  NUM_FIND_SPECIFIC_ATTRS_ON_SERVICE_TYPE_SPAN_SCOPES, 
        //                  find_specific_attrs_on_service_types_span_scopes,
        //                  SLP_OK);
        //result = doTest(hslp, "service:testservice", "", "description,name", "Find attrs existing on different services in same Scope",  &resultCookie);
        
        //NOTE: This is an apparent misuse of findattrs, so it is no longer tested as of 0.9.1
        //Find attributes using wildcards all attributes belong to the same registration, searching a specific registration
        //setupResultCookie(&resultCookie,
        //                  NUM_FIND_ATTRS_WITH_WILDCARDS,
        //                  find_attrs_with_wildcards,
        //                  SLP_OK);
        //result = doTest(hslp, "service:testservice:service2scope2://192.168.100.15", "", "*second", "Find attributes with wildcard-Service URL",  &resultCookie);
        //result = doTest(hslp, "service:testservice", "", "*second", "Find attributes with wildcard-Service Type",  &resultCookie);

        //This test was causing slpd to segfault.
        //Find attribute values which contain special characters
        //setupResultCookie(&resultCookie,
        //                  NUM_SPECIAL_CHAR_VALUES,
        //                  special_char_strings,
        //                  SLP_OK);
        //result = doTest(hslp, "service:scope1://192.168.100.4", "", "comma,backslash", "(Find attributes which contain special character values-Service URL)",  &resultCookie);
        //result = doTest(hslp, "service:scope1", "", "comma,backslash", "(Find attributes which contain special character values-Service Type)",  &resultCookie);

        //Find boolean attributes
        setupResultCookie(&resultCookie,
                          NUM_BOOL_STRINGS,
                          bool_strings,
                          SLP_OK);
        result = doTest(hslp, "service:scope1://192.168.100.4", "", "boolean", "Find boolean attributes-Service URL",  &resultCookie);
        result = doTest(hslp, "service:scope1", "", "boolean", "Find boolean attributes-Service type",  &resultCookie);
        
        //Static registration and finding of Opaque attributes (\FF\0D)
        
        //Find special attributes, specifying one scope
        //setupResultCookie(&resultCookie,
        //                  NUM_SPECIAL_ATTRS,
        //                  special_attrs_strings,
        //                  SLP_OK);
        //result = doTest(hslp, "service:scope3://192.168.100.6", "testscope", "XONLY", "(Find special attributes-Service URL [Scope Specific])",  &resultCookie);
        //result = doTest(hslp, "service:scope3", "testscope", "XONLY", "(Find special attributes-Service type [Scope Specific])",  &resultCookie);
        //result = doTest(hslp, "service:scope3", "default", "XONLY", "(Find existing attribute outside of scope which it exists in [This should Fail])",  &resultCookie);
        
        //Find integer attributes, & explicitly specifying all supported scopes
        setupResultCookie(&resultCookie,
                          NUM_INT_ATTRS,
                          int_attr_strings,
                          SLP_OK);
        result = doTest(hslp, "service:scope2://192.168.100.5", "testscope,DEFAULT", "daysinyear,yearsindecade", "Find int attrs[Specify all Supported Scopes]Service URL",  &resultCookie);
        //NOTE:  This is an apparent misuse of findattrs, so it is no longer tested as of 0.9.1
        //result = doTest(hslp, "service:scope2", "testscope,DEFAULT", "daysinyear,yearsindecade", "Find int attrs[Specify all Supported Scopes]Service Type",  &resultCookie);
        
        //NOTE: This is known to be broken and won't be fixed unless it becomes a problem therefor it won't be tested
        //testing numeric strings which are larger than the maximum integer size
        //setupResultCookie(&resultCookie,
        //                  NUM_LARGE_NUM,
        //                  large_num_strings,
        //                  SLP_OK);
        //result = doTest(hslp, "service:scope2", "", "hugenumber", "Large Numeric String",  &resultCookie);

        //Scope Case insensitivity tests
        printf("Case Insensitivity tests:\n");
        setupResultCookie(&resultCookie,
                          NUM_ALL_UPPER,
                          all_upper_strings,
                          SLP_OK);
        result = doTest(hslp, "service:testservice:service1scope2://192.168.100.14", "TESTSCOPE", "firstinsecond", "All uppercase scopes",  &resultCookie);
        setupResultCookie(&resultCookie,
                          NUM_ALL_LOWER,
                          all_lower_strings,
                          SLP_OK);
        result = doTest(hslp, "service:testservice:service1", "default", "description", "All lowercase scopes",  &resultCookie);
        
        //alpha numeric attribute request
        setupResultCookie(&resultCookie,
                          NUM_ALPHA_NUMERIC,
                          alpha_numeric_strings,
                          SLP_OK);
        result = doTest(hslp, "service:scope2", "", "alphanumeric,phone", "Alphanumeric attribute requests",  &resultCookie);
 
        SLPClose(hslp);
    }
    else {
        printf("CRITICAL ERROR: SLPOpen failed!\n");
    }

    return result;
}
