
/****************************************************************************
* Project:  OpenSLP unit tests
* File:     slpperf.c
* Abstract: Performance test #1 for SLP
****************************************************************************/

#include "slpperf.h"
#include <sys/utsname.h>

/*-------------------------------------------------------------------------*/
typedef struct _TestService
/*-------------------------------------------------------------------------*/
{
    SLPListItem     listitem;
    char*           serviceurl;
    char*           servicetype;
    char*           attributes;
}TestService_T;


/*-------------------------------------------------------------------------*/
TestService_T* CreateTestService(const char* serviceurl,
                                 const char* servicetype,
                                 const char* attributes)
/*-------------------------------------------------------------------------*/
{
    TestService_T* result;
    int serviceurlsize;
    int servicetypesize;
    int attributessize;
    
    serviceurlsize = strlen(serviceurl) + 1;
    servicetypesize =  strlen(servicetype) + 1;
    attributessize = strlen(attributes) + 1;
    
    result = (TestService_T*) malloc(sizeof(TestService_T) + serviceurlsize + servicetypesize + attributessize);
    if(result)
    {
        result->serviceurl = (char*) (result + 1);
        memcpy(result->serviceurl,serviceurl,serviceurlsize);
        result->servicetype = result->serviceurl + serviceurlsize;
        memcpy(result->servicetype,servicetype,servicetypesize);
        result->attributes = result->servicetype + servicetypesize;
        memcpy(result->attributes,attributes,attributessize); 
    }

    return result;
}


/*-------------------------------------------------------------------------*/
TestService_T* FindRandomTestService(SLPList* service_list)
/*-------------------------------------------------------------------------*/
{
    int i;
    int end =  (rand() % service_list->count + 1);
    TestService_T* result = (TestService_T*) service_list->head;
    
    for(i = 1; i < end; i++)
    {
        result = (TestService_T*)result->listitem.next;
    }

    return result;
}

/*-------------------------------------------------------------------------*/
TestService_T* CreateRandomTestService(int id)
/*-------------------------------------------------------------------------*/
{
    char ids[128];
    char serviceurl[256];
    char servicetype[256];
    char attributes[256];
    struct utsname myname;
    
    uname(&myname);  
    sprintf(ids,"slpperf%i",id);
    sprintf(serviceurl,"service:%s.x://%s:12345",ids,myname.nodename);
    sprintf(servicetype,"service:%s.x",ids);
    sprintf(attributes,"(attr1=%s-val),(attr2=%s-val),(attr3=%s-val)", ids,ids,ids);
      

    return  CreateTestService(serviceurl,servicetype,attributes);
}


/*-------------------------------------------------------------------------*/
SLPBoolean SlpPerfTest1_srvtypecallback(SLPHandle hslp,
                                        const char* srvtypes,
                                        SLPError errcode,
                                        void *cookie)
/*-------------------------------------------------------------------------*/
{
    int* found = (int*) cookie;
    
    if(errcode == SLP_OK)
    {
        *found = (*found) + 1;
        printf("Found srvtypes: %s\n",srvtypes);
    }
    
    return SLP_TRUE;   
}


/*-------------------------------------------------------------------------*/
int SlpPerfTest1_slpfindsrvtypes(SLPHandle hslp, 
                                 SLPList* service_list,
                                 double* ave_slpfindattrs,
                                 int* count_slpfindattrs)
/*-------------------------------------------------------------------------*/
{
    SLPError errorcode;
    int      found;

    errorcode = SLPFindSrvTypes(hslp,
                                "*",
                                "",
                                SlpPerfTest1_srvtypecallback,
                                &found);

    if(errorcode != SLP_OK)
    {
        printf("SLPFindSrvTypes(hslp,*, , callback %i) returned %i \n",
               found,
               errorcode);
        printf("This should not happen!\n");
    
        return -1;
    }

    if(found <= 0)
    {
        printf("Did not find any types on SLPFindSrvTypes(). This is bad\n");
        return -1;
    }

    return 0;
}

/*-------------------------------------------------------------------------*/
SLPBoolean SlpPerfTest1_slpfindattrscallback( SLPHandle hslp,
                                              const char* attrlist, 
                                              SLPError errcode, 
                                              void* cookie ) 
/*-------------------------------------------------------------------------*/
{
    int* found = (int*) cookie;
    if(errcode == SLP_OK)
    {
        *found = *found + 1;
        printf("Found Attrs: %s\n",attrlist);
    }
    return SLP_TRUE;
}


/*-------------------------------------------------------------------------*/
int SlpPerfTest1_slpfindattrs (SLPHandle hslp, 
                               SLPList* service_list,
                               double* ave_slpfindattrs,
                               int* count_slpfindattrs)
/*-------------------------------------------------------------------------*/
{
    int             found;
    struct timezone tz;
    struct timeval  start;
    struct timeval  end;
    TestService_T*  srv;
    SLPError        errorcode;

    srv = FindRandomTestService(service_list);

       /* mark start time */
    gettimeofday(&start,&tz);
    
    /* call SLP API */
    found = 0;
   
    errorcode = SLPFindAttrs( hslp, 
                              srv->serviceurl,
                              0,
                              0,
                              SlpPerfTest1_slpfindattrscallback, 
                              &found);
    if(errorcode != SLP_OK)
    {
        printf("SLPFindAttrs(hslp,%s,0,0,callback,%i) returned %i\n", srv->serviceurl,found,errorcode);
        printf("This should not happen!\n");
    
        return -1;
    }

    if(found <= 0)
    {
        printf("Did not find any attributes on SLPFindAttrs(). This is bad\n");
        return -1;
    }
                     
    return 0;
}

/*-------------------------------------------------------------------------*/
SLPBoolean SlpPerfTest1_slpsrvurlcallback( SLPHandle hslp, 
                                          const char* srvurl, 
                                          unsigned short lifetime, 
                                          SLPError errcode, 
                                          void* cookie )
/*-------------------------------------------------------------------------*/
{
    /* don't do anything for now */
    int* found = (int*) cookie;
    if(errcode == SLP_OK)
    {
        *found = *found + 1;
        printf("Found Srv: %s\n",srvurl);
    }
    return SLP_TRUE;
}


/*-------------------------------------------------------------------------*/
int SlpPerfTest1_slpfindsrvs (SLPHandle hslp, 
                              SLPList* service_list,
                              double* ave_slpfindattrs,
                              int* count_slpfindattrs)
/*-------------------------------------------------------------------------*/
{
    int             found;
    struct timezone tz;
    struct timeval  start;
    struct timeval  end;
    TestService_T*  srv;
    SLPError        errorcode;

    srv = FindRandomTestService(service_list);

       /* mark start time */
    gettimeofday(&start,&tz);
    
    /* call SLP API */
    found = 0;
    errorcode = SLPFindSrvs(hslp, 
                            srv->servicetype, 
                            0,
                            0,
                            SlpPerfTest1_slpsrvurlcallback,
                            &found);
    if(errorcode != SLP_OK)
    {
        printf("SLPFindSrvs(hslp,%s,0,0,callback,%i) returned %i\n",
	       srv->servicetype,
	       found,
	       errorcode);
    
        printf("This should not happen!\n");
    
        return -1;
    }

    if(found <= 0)
    {
        printf("Did not find any services on SLPFindSrvs(). This is bad\n");
        return -1;
    }
                     
    return 0;
}

/*-------------------------------------------------------------------------*/
void SlpPerfTest1_deregreport(SLPHandle hslp, SLPError errcode, void* cookie) 
/*-------------------------------------------------------------------------*/
{
    TestService_T*  srv = (TestService_T*)cookie;
    if(errcode == SLP_OK)
    {
        printf("Deregistered: %s\n",srv->serviceurl);
    }
}



/*-------------------------------------------------------------------------*/
int SlpPerfTest1_slpdereg (SLPHandle hslp, 
                           SLPList* service_list,
                           double* ave_slpdereg,
                           int* count_slpdereg)
/*-------------------------------------------------------------------------*/
{
    struct timezone tz;
    struct timeval  start;
    struct timeval  end;
    TestService_T*  srv;
    SLPError        errorcode;
    
    srv = FindRandomTestService(service_list);

       /* mark start time */
    gettimeofday(&start,&tz);
    
    /* call SLP API */
    errorcode = SLPDereg(hslp,
                         srv->serviceurl,
                         SlpPerfTest1_deregreport,
                         srv);
    if(errorcode != SLP_OK)
    {
        printf("SLPDereg(hslp,%s,callback,0) returned %i\n",
               srv->serviceurl,
               errorcode);
        
        printf("This should not happen!\n");
        
        return -1;
    }

    /* mark end time */
    gettimeofday(&end,&tz);
    
    /* unlink the registered service from the list and free it*/
    free(SLPListUnlink(service_list,(SLPListItem*)srv));

    /* recalculate average time */
    *ave_slpdereg = (*ave_slpdereg) * (*count_slpdereg) + ElapsedTime(&start,&end);
    *count_slpdereg = *count_slpdereg + 1;
    *ave_slpdereg = *ave_slpdereg / *count_slpdereg;

    return 0;
}

/*-------------------------------------------------------------------------*/
int SlpPerfTest1_slpderegall (SLPHandle hslp, 
                           SLPList* service_list,
                           double* ave_slpdereg,
                           int* count_slpdereg)
/*-------------------------------------------------------------------------*/
{
       
    SLPError        errorcode;
    TestService_T*  srv;

    srv = (TestService_T*)service_list->head;
    while(srv)
    {
        errorcode = SLPDereg(hslp,
                             srv->serviceurl,
                             SlpPerfTest1_deregreport,
                             srv);
        srv = (TestService_T*)srv->listitem.next;
    }                             
	
    return errorcode;
}


/*-------------------------------------------------------------------------*/
void SlpPerfTest1_regreport(SLPHandle hslp, SLPError errcode, void* cookie) 
/*-------------------------------------------------------------------------*/
{
    TestService_T*  srv = (TestService_T*)cookie;
    if(errcode == SLP_OK)
    {
        printf("Registered: %s\n",srv->serviceurl);
    }
}

/*-------------------------------------------------------------------------*/
int SlpPerfTest1_slpreg (SLPHandle hslp,
                         SLPList* service_list,
                         double* ave_slpreg,
                         int* count_slpreg)
/*-------------------------------------------------------------------------*/
{
    struct timezone tz;
    struct timeval  start;
    struct timeval  end;
    TestService_T*  srv;
    SLPError        errorcode;
        
    srv = CreateRandomTestService(*count_slpreg);
    if(srv == 0)
    {
        return ENOMEM;
    }

    
    /* mark start time */
    gettimeofday(&start,&tz);
    
    /* call SLP API */
    errorcode = SLPReg(hslp,
                       srv->serviceurl,
                       SLP_LIFETIME_MAXIMUM - 1,
                       srv->servicetype,
                       srv->attributes,
                       SLP_TRUE,
                       SlpPerfTest1_regreport,
                       srv);
    if(errorcode != SLP_OK)
    {
        printf("SLPReg(hslp,%s,SLP_LIFETIME_MAX,%s,%s,SLP_FALSE,callback,0) returned %i\n",
               srv->serviceurl,
               srv->servicetype,
               srv->attributes,
               errorcode);
        
        printf("This should not happen!\n");
        
        return -1;
    }
    /* mark end time */
    gettimeofday(&end,&tz);
    
    /* link the registered service into the list */
    SLPListLinkHead(service_list,(SLPListItem*)srv);

    /* recalculate average time */
    *ave_slpreg = (*ave_slpreg) * (*count_slpreg) + ElapsedTime(&start,&end);
    *count_slpreg = *count_slpreg + 1;
    *ave_slpreg = *ave_slpreg / *count_slpreg;

    return 0;
}


/*=========================================================================*/
int SlpPerfTest1(int min_services,
                 int max_services,
                 int iterations,
                 int delay)
/* Performance test number 1.  Randomly registers up to "max_services"     *
 * services with no less than "min_services"                               *
 * Test will be performed for "iterations" iterations. Calls to SLP APIs   *
 * are delayed by "delay" seconds.                                         *
 *=========================================================================*/
{
    int             i;
    SLPError        result;
    double          ave_slpreg              = 0;
    int             count_slpreg            = 0;
    double          ave_slpfindsrvs         = 0;
    int             count_slpfindsrvs       = 0;
    double          ave_slpfindattrs        = 0;
    int             count_slpfindattrs      = 0;
    double          ave_slpfindsrvtypes     = 0;
    int             count_slpfindsrvtypes   = 0;
    double          ave_slpdereg            = 0;
    int             count_slpdereg          = 0;
    SLPHandle       hslp = 0;
    SLPList         service_list        = {0,0,0};

    /*-----------------*/
    /* Open SLP handle */
    /*-----------------*/
    result = SLPOpen("en",SLP_FALSE,&hslp);
    if(result)
    {
        printf("SLPOpen() failed %i \n",result);
        goto RESULTS;
    }

    /*--------------------------------------------------------*/
    /* make sure the minimum number of services is registered */
    /*--------------------------------------------------------*/
    printf("slpperf: Test #1 \n");
    printf("----------------------------------------------------------\n\n");
    printf("Setting up for test. Registering %i minimum services)... \n",
           min_services);
    for (i=0;i<min_services;i++)
    {
        result = SlpPerfTest1_slpreg(hslp,
                                     &service_list,
                                     &ave_slpreg,
                                     &count_slpreg);
        if (result) goto RESULTS;
        sleep(delay);   
    }
    
    /*-------------------------------*/
    /* now start the test iterations */
    /*-------------------------------*/
    printf("Performing %i test iterations...\n",iterations);
    for (i=0;i<iterations;i++)
    {
        switch(rand() % 10)
        {
        /* 10% chance */
        case 0:
            if(service_list.count < max_services)
            {
                result = SlpPerfTest1_slpreg(hslp,
                                             &service_list,
                                             &ave_slpreg,
                                             &count_slpreg);
            }
            else
            {
                /* call SlpPerfTest1_slpdereg() */
                result = SlpPerfTest1_slpdereg(hslp,
                                               &service_list,
                                               &ave_slpdereg,
                                               &count_slpdereg);
            }
            break;
        
        /* 10% chance */
        case 1:
            if(service_list.count > min_services)
            {
                 result = SlpPerfTest1_slpdereg(hslp,
                                               &service_list,
                                               &ave_slpdereg,
                                               &count_slpdereg);
            }
            else
            {
                result = SlpPerfTest1_slpreg(hslp,
                                             &service_list,
                                             &ave_slpreg,
                                             &count_slpreg);            }
            break;
            
        /* 50% chance */
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:            
            /* call SlpPerfTest1_slpfindsrv() */
            result = SlpPerfTest1_slpfindsrvs(hslp,
                                              &service_list,
                                              &ave_slpfindsrvs,
                                              &count_slpfindsrvs);
            break;

        /* 20% chance*/
        case 7:
        case 8:
            /* call SlpPerfTest1_slpfindattr() */
             result = SlpPerfTest1_slpfindattrs(hslp,
                                                &service_list,
                                                &ave_slpfindattrs,
                                                &count_slpfindattrs);
            break;

        /* 10% chance */
        case 9:
            result = SlpPerfTest1_slpfindsrvtypes(hslp,
                                                  &service_list,
                                                  &ave_slpfindsrvtypes,
                                                  &count_slpfindsrvtypes);
            break;
        }
        
        if(result)
        {
            break;
        }

        sleep(delay);
    }

    printf("----------------------------------------------------------\n");
    printf("Cleaning up registered services... \n");
    result = SlpPerfTest1_slpderegall(hslp,
                                      &service_list,
                                      &ave_slpdereg,
                                      &count_slpdereg);


    /*----------------------*/
    /* close the SLP handle */
    /*----------------------*/
    SLPClose(hslp);

RESULTS:
    /*---------------------*/
    /* Display the results */
    /*---------------------*/

    return result;
}
