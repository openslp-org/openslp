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

/** Header file for portable threading primitives.
 *
 * @file       slp_thread.h
 * @author     John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeThread
 */

#ifndef SLP_THREAD_H_INCLUDED
#define SLP_THREAD_H_INCLUDED

/*!@defgroup CommonCodeThread Threading
 * @ingroup CommonCodePlatform
 * @{
 */

/** @name Threading Primitives. 
 * 
 * The thread model is simple, providing only the ability to create and start
 * a thread, and the ability to wait for that thread to terminate, and return
 * its error code.
 */
/*@{*/
/** A waitable thread handle type. */
typedef void * SLPThreadHandle;

/** Thread start function signature. */
typedef void * (*SLPThreadStartProc)(void *);

SLPThreadHandle SLPThreadCreate(SLPThreadStartProc startproc, void * arg);
void * SLPThreadWait(SLPThreadHandle th);
/*@}*/

/** @name Mutex Primitives. 
 * 
 * These mutex primitives are simple, light-weight, non-recursive
 * mutual exclusion locks. They're based on pthreads for POSIX platforms
 * and Win32 CRITICAL_SECTIONS on Windows platforms.
 */
/*@{*/
/** A cross-platform mutex handle abstraction. */
typedef void * SLPMutexHandle;

SLPMutexHandle SLPMutexCreate(void);
bool SLPMutexTryAcquire(SLPMutexHandle mh);
void SLPMutexAcquire(SLPMutexHandle mh);
void SLPMutexRelease(SLPMutexHandle mh);
void SLPMutexDestroy(SLPMutexHandle mh);
/*@}*/

/*! @} */

#endif   /* SLP_THREAD_H_INCLUDED */

/*=========================================================================*/
