/****************************************************************************
* Project:  OpenSLP unit tests
* File:     slpperf.c
* Abstract: Performance test for SLP
****************************************************************************/

#if(!defined SLPPERF_H_INCLUDED)
#define SLPPERF_H_INCLUDED

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h> 
#include <errno.h>
#include <slp.h>

#include "slp_linkedlist.h"

/*=========================================================================*/
double ElapsedTime(struct timeval* start, struct timeval* end);
/* returns the elapsed time in seconds between two timevals
/*=========================================================================*/


/*=========================================================================*/
int SlpPerfTest1(int min_services,
                 int max_services,
                 int iterations,
                 int delay);
/* Performance test number 1.  Randomly registers up to "max_services"     *
 * services with no less than "min_services"                               *
 * Test will be performed for "iterations" iterations. Calls to SLP APIs   *
 * are delayed by "delay" seconds.                                         *
 *=========================================================================*/


#endif

