#ifndef __SLP_TESTCOMMON_H
#define __SLP_TESTCOMMON_H

#include <slp.h>
#include <libslp.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define MAX_RESULT_MSG_SIZE  2048

#define SLP_TEST_MISMATCH       -1000
#define SLP_TEST_NORESULTS      -1001
#define SLP_TEST_INVALIDARGS    -1002
#define SLP_TEST_UNINITIALIZED  -1003

const int TEST_TYPE_PADDING  =  58;

//The result cookie is used to pass around
//the expected test results and the actual
//results of the test.
typedef struct  _SLPResultCookie {
    char          msg[MAX_RESULT_MSG_SIZE];
    int           expectedRetval;
    int           retval;
    char          resultString[MAX_RESULT_MSG_SIZE];
    int           numTestStrings;
    const char**  testStrings;
} SLPResultCookie,*PSLPResultCookie;

int NetworkConnectToSlpd(struct sockaddr_in* peeraddr);

/*=========================================================================*/
char* errcodeToStr(int errcode)
/*=========================================================================*/
{
    char* retval;

    switch (errcode) {
    case 1:
        retval="SLP_LAST_CALL";
        break;
    case 0:
        retval="SLP_OK";
        break;    
    case -1:
        retval="SLP_LANGUAGE_NOT_SUPPORTED";
        break;
    case -2:
        retval="SLP_PARSE_ERROR";
        break;
    case -3:
        retval="SLP_INVALID_REGISTRATION";
        break;
    case -4:
        retval="SLP_SCOPE_NOT_SUPPORTED";
        break;
    case -6:
        retval="SLP_AUTHENTICATION_ABSENT";
        break;
    case -7:
        retval="SLP_AUTHENTICATION_FAILED";
        break;
    case -13:
        retval="SLP_INVALID_UPDATE";
        break;
    case -15:
        retval="SLP_REFRESH_REJECTED";
        break;    
    case -17:
        retval="SLP_NOT_IMPLEMENTED";
        break;
    case -18:
        retval="SLP_BUFFER_OVERFLOW";
        break;
    case -19:
        retval="SLP_NETWORK_TIMED_OUT";
        break;
    case -20:
        retval="SLP_NETWORK_INIT_FAILED";
        break;
    case -21:
        retval="SLP_MEMORY_ALLOC_FAILED";
        break;
    case -22:
        retval="SLP_PARAMETER_BAD";
        break;
    case -23:
        retval="SLP_NETWORK_ERROR";
        break;
    case -24:
        retval="SLP_INTERNAL_SYSTEM_ERROR";
        break;
    case -25:
        retval="SLP_HANDLE_IN_USE";
        break;
    case -26:
        retval="SLP_TYPE_ERROR";
        break;
    case -1000:
        retval="SLP_TEST_MISMATCH";
        break;
    case -1001:
        retval="SLP_TEST_NORESULTS";
        break;
    case -1002:
        retval="SLP_TEST_INVALIDARGS";
        break;
    case -1003:
        retval="SLP_TEST_UNINITIALIZED";
        break;
    default:
        snprintf(retval,80,"Unknown SLP error Code %d",errcode);
        break;
    }

    return retval;
}


/*=========================================================================*/
SLPBoolean theAttrsCallback( SLPHandle hslp, 
                        const char* attrlist,  
                        SLPError errcode, 
                        void* cookie ) 
/*=========================================================================*/
{
    int i = 0;
    SLPResultCookie* myCookie = (SLPResultCookie*)cookie;
    if(errcode == SLP_OK)
    {
        //Test all of the expected strings against srvtypes string
        //if one is found set it to NULL
        for ( i = 0; i < myCookie->numTestStrings; i++) {  
            if ( *attrlist ) {
                if (myCookie->testStrings[i] != NULL) {
                    if ( strstr(attrlist, myCookie->testStrings[i]) != NULL) {
                        myCookie->testStrings[i] = NULL; //If the string is found, set it to NULL
                    }
                }//end if (myCookie->testStrings[i] != NULL)
            }// end if (*srvtypes)
        }
        strncpy(myCookie->resultString, attrlist, MAX_RESULT_MSG_SIZE);
    }
    else {
        if (errcode != SLP_LAST_CALL) {
            myCookie->retval  = errcode;
            snprintf(myCookie->msg, 
                 MAX_RESULT_MSG_SIZE, 
                 "General library error.\n");    
        }
        else {
            //LAST_CALL was returned need to set return values
            memset(&myCookie->msg, 0, MAX_RESULT_MSG_SIZE);
            myCookie->retval = SLP_OK;
            for ( i = 0; i < myCookie->numTestStrings; i++) {
                //If one of the expected strings is not null, then it
                //was not found in the result string. That's bad.
                if ( myCookie->testStrings[i] != NULL) {
                    myCookie->retval  = SLP_TEST_MISMATCH;
                    snprintf(myCookie->msg,
                             MAX_RESULT_MSG_SIZE,
                             "MISMATCH!\n\tExpected string:\t%s\n\tResultString:\t\t%s\n",
                             myCookie->testStrings[i], 
                             myCookie->resultString);
                    return SLP_FALSE; //Don't call me again. Won't get any more data anyway.
                }//end if ( myCookie->testStrings[i] != NULL)
            } //end for ( i = 0; i < myCookie->numTestStrings; i++)
        } //end else
    } //end else
    return SLP_TRUE; //Call me again if needed
}



/*=========================================================================*/
void theRegCallback(SLPHandle hslp, 
                  SLPError errcode, 
                  void* cookie)
/*=========================================================================*/
{
    SLPResultCookie* myCookie = (SLPResultCookie*)cookie;
    if (errcode == SLP_OK) {
        myCookie->retval = SLP_OK;
    }
    else {
        myCookie->retval = errcode;
        snprintf(myCookie->msg, MAX_RESULT_MSG_SIZE,
                 "Error registering service. Errorcode %s",
                 errcodeToStr(errcode));
    }
}

/*=========================================================================*/
SLPBoolean theSrvsCallback( SLPHandle hslp, 
                        const char* srvurl, 
                        unsigned short lifetime, 
                        SLPError errcode, 
                        void* cookie ) 
/*=========================================================================*/
{
    int i = 0;
    SLPResultCookie* myCookie = (SLPResultCookie*)cookie;
    if(errcode == SLP_OK)
    {
        //Test all of the expected strings against srvtypes string
        //if one is found set it to NULL
        for ( i = 0; i < myCookie->numTestStrings; i++) {  
            if ( *srvurl ) {
                strncat(myCookie->resultString, srvurl, MAX_RESULT_MSG_SIZE);
                strncat(myCookie->resultString, ",", MAX_RESULT_MSG_SIZE);
                if (myCookie->testStrings[i] != NULL) {
                    if ( strstr(srvurl, myCookie->testStrings[i]) != NULL) {
                        myCookie->testStrings[i] = NULL; //If the string is found, set it to NULL
                    }
                }//end if (myCookie->testStrings[i] != NULL)
            }// end if (*srvtypes)
        }
    }
    else {
        if (errcode != SLP_LAST_CALL) {
            myCookie->retval  = errcode;
            snprintf(myCookie->msg, 
                 MAX_RESULT_MSG_SIZE, 
                 "General library error.\n");    
        }
        else {
            //LAST_CALL was returned need to set return values
            memset(&myCookie->msg, 0, MAX_RESULT_MSG_SIZE);
            myCookie->retval = SLP_OK;
            for ( i = 0; i < myCookie->numTestStrings; i++) {
                //If one of the expected strings is not null, then it
                //was not found in the result string. That's bad.
                if ( myCookie->testStrings[i] != NULL) {
                    myCookie->retval  = SLP_TEST_MISMATCH;
                    snprintf(myCookie->msg,
                             MAX_RESULT_MSG_SIZE,
                             "MISMATCH!\n\tExpected string:\t%s\n\tResultString:\t\t%s\n",
                             myCookie->testStrings[i], 
                             myCookie->resultString);
                    return SLP_FALSE; //Don't call me again. Won't get any more data anyway.
                }//end if ( myCookie->testStrings[i] != NULL)
            } //end for ( i = 0; i < myCookie->numTestStrings; i++)
        } //end else
    } //end else
    return SLP_TRUE; //Call me again if needed
}



/*=========================================================================*/
SLPBoolean theSrvtypesCallback( SLPHandle hslp, 
                        const char* srvtypes, 
                        SLPError errcode, 
                        void* cookie ) 
/*=========================================================================*/
{
    int i = 0;
    SLPResultCookie* myCookie = (SLPResultCookie*)cookie;
    //printf("Service Types Returned: %s\n", srvtypes);
    if(errcode == SLP_OK)
    {
        //Test all of the expected strings against srvtypes string
        //if one is found set it to NULL
        for ( i = 0; i < myCookie->numTestStrings; i++) {  
            if ( *srvtypes ) {
                strncpy(myCookie->resultString, srvtypes, MAX_RESULT_MSG_SIZE);
                if (myCookie->testStrings[i] != NULL) {
                    if ( strstr(srvtypes, myCookie->testStrings[i]) != NULL) {
                        myCookie->testStrings[i] = NULL; //If the string is found, set it to NULL
                    }
                }//end if (myCookie->testStrings[i] != NULL)
            }// end if (*srvtypes)
        }
    }
    else {
        if (errcode != SLP_LAST_CALL) {
            myCookie->retval  = errcode;
            snprintf(myCookie->msg, 
                 MAX_RESULT_MSG_SIZE, 
                 "General library error.\n");    
        }
        else {
            //LAST_CALL was returned need to set return values
            memset(&myCookie->msg, 0, MAX_RESULT_MSG_SIZE);
            myCookie->retval = SLP_OK;
            for ( i = 0; i < myCookie->numTestStrings; i++) {
                //If one of the expected strings is not null, then it
                //was not found in the result string. That's bad.
                if ( myCookie->testStrings[i] != NULL) {
                    myCookie->retval  = SLP_TEST_MISMATCH;
                    snprintf(myCookie->msg,
                             MAX_RESULT_MSG_SIZE,
                             "MISMATCH!\n\tExpected string:\t%s\n\tResultString:\t\t%s\n",
                             myCookie->testStrings[i], 
                             myCookie->resultString);
                    return SLP_FALSE; //Don't call me again. Won't get any more data anyway.
                }//end if ( myCookie->testStrings[i] != NULL)
            } //end for ( i = 0; i < myCookie->numTestStrings; i++)
        } //end else
    } //end else
    return SLP_TRUE; //Call me again if needed
}

void setupResultCookie(PSLPResultCookie rc, int numTestStrings, const char** testStrings, int expectedRetval) {
    rc->numTestStrings = numTestStrings;
    rc->testStrings = testStrings;
    rc->retval = -1003; //Set up an inital fail state
    rc->expectedRetval = expectedRetval;
    memset(rc->msg, 0, MAX_RESULT_MSG_SIZE );
    strcpy(rc->msg, "Not set. [Callback function was not invoked.]");
    memset(rc->resultString, 0, MAX_RESULT_MSG_SIZE);
}

SLPBoolean slpdRunningLocal() {
    struct sockaddr_in ina;
    int result = -1;
    result = NetworkConnectToSlpd(&ina);
    if (result == -1) {
        return SLP_FALSE;
    }
    return SLP_TRUE;
}

#endif
