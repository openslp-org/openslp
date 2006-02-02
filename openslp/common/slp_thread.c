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

/** Thread synchronization.
 *
 * Implementation of threading primitives.
 *
 * @file       slp_thread.c
 * @author     John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#include "slp_types.h"
#include "slp_thread.h"

/** Create a new thread of execution.
 *
 * While it could be argued that creating a new thread for each asynch
 * request is inefficient, it's equally true that network latency and the
 * most often practiced uses of the SLP API don't mandate the overhead of
 * more heavy-weight constructs such as thread-pools to solve this problem.
 *
 * @param[in] startproc - The function to run on a new thread.
 * @param[in] arg - Context data to pass to @p startproc.
 *
 * @return A thread handle that may be used later by another thread to 
 * synchronize with the new thread's termination point. In pthread 
 * parlance, this attribute is called "joinable".
 *
 * @note The function address passed for @p startproc must match the 
 * following signature: void * startproc(void *), where the return value
 * and the parameter must both be integer values the size of a pointer on 
 * the target platform.
 */
ThreadHandle ThreadCreate(ThreadStartProc startproc, void * arg)
{
#ifdef _WIN32
   return (ThreadHandle)CreateThread(0, 0, 
         (LPTHREAD_START_ROUTINE)startproc, arg, 0, 0);
#else
   pthread_t th;
   return (ThreadHandle)(pthread_create(&th, 0, startproc, arg)? 0: th);
#endif
}

/** Wait for a thread to terminate.
 *
 * Causes the caller to wait in an efficient wait-state until the thread 
 * associated with the handle @p th terminates.
 *
 * @param[in] th - The thread handle to wait on.
 *
 * @return The thread's exit code.
 */
void * ThreadWait(ThreadHandle th)
{
   void * result = 0;
#ifdef _WIN32
   DWORD dwres;
   WaitForSingleObject((HANDLE)th, INFINITE);
   if (GetExitCodeThread((HANDLE)th, &dwres))
      result = (void *)(intptr_t)dwres;
   CloseHandle((HANDLE)th);
#else
   pthread_join((pthread_t)th, &result);
#endif
   return result;
}

/*=========================================================================*/
