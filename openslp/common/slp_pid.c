/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_pid.c                                                  */
/*                                                                         */
/* Abstract:    Common code to obtain process identifier information       */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*     Please submit patches to http://www.openslp.org                     */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/* Copyright (C) 2000 Caldera Systems, Inc                                 */
/* All rights reserved.                                                    */
/*                                                                         */
/* Redistribution and use in source and binary forms, with or without      */
/* modification, are permitted provided that the following conditions are  */
/* met:                                                                    */ 
/*                                                                         */
/*      Redistributions of source code must retain the above copyright     */
/*      notice, this list of conditions and the following disclaimer.      */
/*                                                                         */
/*      Redistributions in binary form must reproduce the above copyright  */
/*      notice, this list of conditions and the following disclaimer in    */
/*      the documentation and/or other materials provided with the         */
/*      distribution.                                                      */
/*                                                                         */
/*      Neither the name of Caldera Systems nor the names of its           */
/*      contributors may be used to endorse or promote products derived    */
/*      from this software without specific prior written permission.      */
/*                                                                         */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS     */
/* `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT      */
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   */
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CALDERA      */
/* SYSTEMS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, */
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON       */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    */
/*                                                                         */
/***************************************************************************/

#include "slp_pid.h"

#ifdef __WIN32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#endif

                   

/*=========================================================================*/
uint32_t SLPPidGet()
/* Description:
 *    Get a process 32 bit integer identifier for the current process
 *    loopback interface
 *
 * Parameters:
 *
 * Returns:
 *     32 bit integer identifier for the current process
 *=========================================================================*/
{
    #ifdef __WIN32__
    return GetCurrentProcessId();
    #else
    return getpid();
    #endif
}


/*=========================================================================*/
int SLPPidExists(uint32_t pid)
/* Description:
 *    (quickly) determine whether or not the process with the specified
 *    identifier exists (is alive)
 *
 * Parameters:
 *    pid (IN) 32 bit integer identifier for the process to check for
 *
 * Returns:
 *    Boolean value.  Zero if process does not exist, non-zero if it does
 *=========================================================================*/
{
#ifndef __WIN32__
    if(kill(pid,0))
    {
        if(errno == ESRCH)
        {
            /* pid does not exist */
            return 0;
        }
    }

#else
	HANDLE h;
	if (!(h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, (DWORD)pid)))
	{
		/* pid does not exist - or we have so few rights we shouldn't be running! */
		return 0;
	}
	CloseHandle(h);
#endif
	return 1;
}
