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
 * Implementation of threading and mutex primitives.
 *
 * @file       slp_thread.c
 * @author     John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeThread
 */

#include "slp_types.h"
#include "slp_thread.h"
#include "slp_xmalloc.h"

/** Create a new thread of execution.
 *
 * While it could be argued that creating a new thread for each async
 * request is inefficient, it's equally true that network latency and the
 * most often practiced uses of the SLP API don't mandate the overhead of
 * more heavy-weight constructs (e.g., thread pools) to solve this problem.
 *
 * @param[in] startproc - The function to run on a new thread.
 * @param[in] arg - Context data to pass to @p startproc.
 *
 * @return A thread handle that may be used later by another thread to 
 * synchronize with the new thread's termination point. In pthread 
 * parlance, this attribute is called "joinable".
 *
 * @remarks The function address passed for @p startproc must match the 
 * following signature: void * startproc(void *), where the return value
 * and the parameter must both be integer values the size of a pointer on 
 * the target platform.
 */
SLPThreadHandle SLPThreadCreate(SLPThreadStartProc startproc, void * arg)
{
#ifdef _WIN32
   return (SLPThreadHandle)CreateThread(0, 0, 
         (LPTHREAD_START_ROUTINE)startproc, arg, 0, 0);
#else
   pthread_t th;
   return (SLPThreadHandle)(pthread_create(&th, 0, startproc, arg)? 0: th);
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
void * SLPThreadWait(SLPThreadHandle th)
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

/** Create a new mutex lock.
 *
 * Create and return a handle to a new recursive mutex lock. Note that 
 * Windows CRITICAL_SECTION objects are recursive by default, whereas
 * pthread mutexes have to be configured that way with an attribute.
 * 
 * @return The new mutex's handle, or zero on failure (generally memory 
 * allocation failure causes mutex creation failure).
 */
SLPMutexHandle SLPMutexCreate(void)
{
#ifdef _WIN32
   CRITICAL_SECTION * mutex = (CRITICAL_SECTION *)xmalloc(sizeof(*mutex));
   if (mutex != 0)
      InitializeCriticalSection(mutex);
#else
   pthread_mutex_t * mutex = 0;
   pthread_mutexattr_t attr;
   if (pthread_mutexattr_init(&attr) == 0)
   {
      (void)pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
      mutex = (pthread_mutex_t *)xmalloc(sizeof(*mutex));
      if (mutex != 0 && pthread_mutex_init(mutex, &attr) != 0)
      {
         xfree(mutex);
         mutex = 0;
      }
      (void)pthread_mutexattr_destroy(&attr);
   }
#endif
   return (SLPMutexHandle)mutex;
}

/** Acquire a lock on a mutex.
 *
 * @param[in] mh - The mutex handle to be acquired.
 */
void SLPMutexAcquire(SLPMutexHandle mh)
{
#ifdef _WIN32
   EnterCriticalSection((CRITICAL_SECTION *)mh);
#else
   (void)pthread_mutex_lock((pthread_mutex_t *)mh);
#endif
}

/** Try to acquire a lock on a mutex.
 *
 * @param[in] mh - The mutex handle on which to attempt acquisition.
 */
bool SLPMutexTryAcquire(SLPMutexHandle mh)
{
#ifdef _WIN32
   return TryEnterCriticalSection((CRITICAL_SECTION *)mh) != FALSE;
#else
   return pthread_mutex_trylock((pthread_mutex_t *)mh) == 0;
#endif
}

/** Release a lock on a mutex.
 *
 * @param[in] mh - The mutex handle to be released.
 */
void SLPMutexRelease(SLPMutexHandle mh)
{
#ifdef _WIN32
   LeaveCriticalSection((CRITICAL_SECTION *)mh);
#else
   (void)pthread_mutex_unlock((pthread_mutex_t *)mh);
#endif
}

/** Destroy a mutex lock.
 * 
 * @param[in] mh - The mutex handle to be destroyed.
 */
void SLPMutexDestroy(SLPMutexHandle mh)
{
#ifdef _WIN32
   DeleteCriticalSection((CRITICAL_SECTION *)mh);
#else
   (void)pthread_mutex_destroy((pthread_mutex_t *)mh);
#endif
   free(mh);
}

/*=========================================================================*/
