/*=========================================================================
* Project:  OpenSLP unit tests
* File:     slpperf.c
* Abstract: Performance test for SLP
*=========================================================================*/

#include "slpperf.h"

double ElapsedTime(struct timeval* start, struct timeval* stop)
{
    if(stop->tv_usec < start->tv_usec )
    {
        return ((stop->tv_sec - start->tv_sec) - 1) + (.000001 * (1000000 + stop->tv_usec - start->tv_usec) );
    }
    
    return (stop->tv_sec - start->tv_sec) + (.000001 * (stop->tv_usec - start->tv_usec));
}



/*-------------------------------------------------------------------------*/
void test_callback(SLPHandle hslp, SLPError errcode, void* cookie) 
/*-------------------------------------------------------------------------*/
{
}

void test()
{
    int i;
    SLPError errorcode;
    SLPHandle hslp;

    SLPOpen("en",SLP_FALSE,&hslp);
    

    for (i = 1;i<10;i++)
    {
        errorcode = SLPDereg(hslp,
                             "hello://1.2.3.4",
                             test_callback,
                             0);
    }

    SLPClose(hslp);
}

int main(int argc, char* argv[])
{
    int result;

    if(argc < 2)
    {
        goto USAGE;
    }

    switch(atoi(argv[1]))
    {
    case 1:
        if(argc == 6)
        {
            result = SlpPerfTest1(atoi(argv[2]),
                                  atoi(argv[3]),
                                  atoi(argv[4]),
                                  atoi(argv[5]));
        }
        else
        {
            goto USAGE;
        }
        
        break;
    case 2:
         test();
         break;
    default:
        goto USAGE;
        
    }

    return result;

USAGE:
    printf("Usage: slpperf [test number] [test args]\n\n");
    printf("    1 <min services> <max services> <iterations> <delay>\n");

    return 1;
}



                                      
