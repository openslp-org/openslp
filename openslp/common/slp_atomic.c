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

/** Atomic Operations.
 *
 * Implementation of atomic operations. 
 *
 * @file       slp_atomic.c
 * @author     John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeAtomic
 */

#include "slp_types.h"
#include "slp_atomic.h"

#if defined(_WIN32)
# define USE_WIN32_ATOMICS
#elif defined(__GNUC__) && defined(__i386__)
# define USE_GCC_I386_ATOMICS

__attribute__((always_inline)) 
static inline int32_t atomic_inc32(volatile int32_t * p)
{
   int32_t rv;
   __asm__ __volatile__(
      "movl $1, %%eax\n\t"
      "lock\n\t"
      "xaddl %%eax, (%%ecx)\n\t"
      "incl %%eax"
      : "=a" (rv) 
      : "c" (p)
   );
   return rv;
}

__attribute__((always_inline)) 
static inline int32_t atomic_dec32(volatile int32_t * p) 
{
   int32_t rv;
   __asm__ __volatile__(
      "movl $-1, %%eax\n\t"
      "lock\n\t"
      "xaddl %%eax, (%%ecx)\n\t"
      "decl %%eax"
      : "=a" (rv)
      : "c" (p)
   );   // result left in eax
   return rv;
}

__attribute__((always_inline))
static inline int32_t atomic_xchg32(volatile int32_t * p, int32_t i) 
{
   int32_t rv;
   __asm__ __volatile__(
      "xchgl %%eax, (%%ecx)"
      : "=a" (rv)
      : "c" (p), "a" (i)
   );   // result left in eax, no buslock required for xchgl
   return rv;
}

#elif defined(__GNUC__) && defined(__x86_64__)
# define USE_GCC_X86_64_ATOMICS

__attribute__((always_inline)) 
static inline int64_t atomic_inc64(volatile int64_t * p)
{
   int64_t rv;
   __asm__ __volatile__(
      "movq $1, %%rax\n\t"
      "lock; xaddq %%rax, (%%rcx)\n\t"
      "incq %%rax"
      : "=a" (rv) 
      : "c" (p)
   );
   return rv;
}

__attribute__((always_inline)) 
static inline int64_t atomic_dec64(volatile int64_t * p) 
{
   int64_t rv;
   __asm__ __volatile__(
      "movq $-1, %%rax\n\t"
      "lock; xaddq %%rax, (%%rcx)\n\t"
      "decq %%rax"
      : "=a" (rv)
      : "c" (p)
   );
   return rv;
}

__attribute__((always_inline))
static inline int64_t atomic_xchg64(volatile int64_t * p, int64_t i) 
{
   int64_t rv;
   __asm__ __volatile__(
      "xchgq %%rax, (%%rcx)"
      : "=a" (rv)
      : "c" (p), "a" (i)
   );
   return rv;
}

#elif defined(_AIX)
# define USE_AIX_ATOMICS
# include <sys/atomic_op.h>
#elif defined(__DECC) || defined(__DECCXX)
# define USE_ALPHA_ATOMICS
# include <machine/builtins.h>
#elif defined(sun) && (defined(__sparc) || defined(__sparc__) || defined(__sparcv8plus) || defined(__sparcv9))
# define USE_SPARC_ATOMICS

int32_t sparc_atomic_add_32(volatile int32_t * p, int32_t i);
int32_t sparc_atomic_xchg_32(volatile int32_t * p, int32_t i);

void sparc_asm_code(void)
{
   asm( ".align 8");
   asm( ".local sparc_atomic_add_32");
   asm( ".type sparc_atomic_add_32, #function");
   asm( "sparc_atomic_add_32:");
   asm( "    membar #LoadLoad | #LoadStore | #StoreStore | #StoreLoad");
   asm( "    ld [%o0], %l0");
   asm( "    add %l0, %o1, %l2");
   asm( "    cas [%o0], %l0, %l2");
   asm( "    cmp %l0, %l2");
   asm( "    bne sparc_atomic_add_32");
   asm( "    nop");
   asm( "    add %l2, %o1, %o0");
   asm( "    membar #LoadLoad | #LoadStore | #StoreStore | #StoreLoad");
   asm( "retl");
   asm( "nop");
   
   asm( ".align 8");
   asm( ".local sparc_atomic_xchg_32");
   asm( ".type sparc_atomic_xchg_32, #function");
   asm( "sparc_atomic_xchg_32:");
   asm( "    membar #LoadLoad | #LoadStore | #StoreStore | #StoreLoad");
   asm( "    ld [%o0], %l0");
   asm( "    mov %o1, %l1");
   asm( "    cas [%o0], %l0, %l1");
   asm( "    cmp %l0, %l1");
   asm( "    bne sparc_atomic_xchg_32");
   asm( "    nop");
   asm( "    mov %l0, %o0");
   asm( "    membar #LoadLoad | #LoadStore | #StoreStore | #StoreLoad");
   asm( "retl");
   asm( "nop");
}

#elif defined(__APPLE__)
# define USE_APPLE_ATOMICS
#else

/** The default guarantee of atomicity. A single global mutex that wraps
 * access to all atomic variables. This is a very poor substitute for true
 * atomics, but the only substitute that guarantees atomicity in the face
 * of no native primtives. Please feel free to implement and add atomics
 * for your platform if you don't find them here.
 */
static pthread_mutex_t g_atomic_mutex = PTHREAD_MUTEX_INITIALIZER;

#endif

/** Increment an integer and return the NEW value in an MP-safe manner.
 *
 * @param[in,out] pn - The address of an integer to atomically increment.
 *
 * @return The result of the atomic increment. 
 *
 * @remarks This routine increments the value at the address specified
 * by @p pn, and returns the @e new value atomically. It is not sufficient
 * to atomically increment, and @e then read the value. The increment and
 * read must occur within one atomic transaction in order to comply with 
 * the required semantics of this routine.
 *
 * @todo Fix the default atomic increment case so it is really atomic.
 */
intptr_t SLPAtomicInc(intptr_t * pn)
{
#if defined(USE_WIN32_ATOMICS)
   return (intptr_t)InterlockedIncrement((LPLONG)pn);
#elif defined(USE_GCC_I386_ATOMICS)
   return (intptr_t)atomic_inc32((int32_t *)pn);
#elif defined(USE_GCC_X86_64_ATOMICS)
   return (intptr_t)atomic_inc64((int64_t *)pn);
#elif defined(USE_AIX_ATOMICS)
   return (intptr_t)fetch_and_add((atomic_p)pn, 1) + 1;
#elif defined(USE_ALPHA_ATOMICS)
   /* Note: __ATOMIC_INCREMENT_QUAD returns the __previous__ value. */
   return __ATOMIC_INCREMENT_QUAD(pn) + 1;
#elif defined(USE_SPARC_ATOMICS)
   return sparc_atomic_add_32(pn, 1);
#elif defined(USE_APPLE_ATOMICS)
   return (intptr_t)OSAtomicIncrement32((int32_t *)pn);
#else
   intptr_t tn;
   pthread_mutex_lock(&g_atomic_mutex);
   tn = ++(*pn);
   pthread_mutex_unlock(&g_atomic_mutex);
   return tn;
#endif
}

/** Decrement an integer and return the NEW value in an MP-safe manner.
 *
 * @param[in,out] pn - The address of an integer to atomically decrement.
 *
 * @return The result of the atomic decrement. 
 *
 * @remarks This routine decrements the value at the address specified
 * by @p pn, and returns the @e new value atomically. It is not sufficient
 * to atomically decrement, and @e then read the value. The decrement and
 * read must occur within one atomic transaction in order to comply with 
 * the required semantics of this routine.
 *
 * @todo Fix the default atomic decrement case so it is really atomic.
 */
intptr_t SLPAtomicDec(intptr_t * pn)
{
#if defined(USE_WIN32_ATOMICS)
   return (intptr_t)InterlockedDecrement((LPLONG)pn);
#elif defined(USE_GCC_I386_ATOMICS)
   return (intptr_t)atomic_dec32((int32_t *)pn);
#elif defined(USE_GCC_X86_64_ATOMICS)
   return (intptr_t)atomic_dec64((int64_t *)pn);
#elif defined(USE_AIX_ATOMICS)
   return (intptr_t)fetch_and_add((atomic_p)pn, -1) - 1;
#elif defined(USE_ALPHA_ATOMICS)
   /* Note: __ATOMIC_DECREMENT_QUAD returns the __previous__ value. */
   return __ATOMIC_DECREMENT_QUAD(pn) - 1;
#elif defined(USE_SPARC_ATOMICS)
   return sparc_atomic_add_32(pn, -1);
#elif defined(USE_APPLE_ATOMICS)
   return (intptr_t)OSAtomicDecrement32((int32_t *)pn);
#else
   intptr_t tn;
   pthread_mutex_lock(&g_atomic_mutex);
   tn = --(*pn);
   pthread_mutex_unlock(&g_atomic_mutex);
   return tn;
#endif
}

/** Exchange two values and return the previous value in an MP-safe manner.
 *
 * Exhanges the value at an address with a specified value and returns the
 * previous contents of the address atomically.
 *
 * @param[in,out] pn - The address of an integer to atomically decrement.
 * @param[in] n - The value to exchange with the contents of @p pn.
 *
 * @return The value of @p pn @e before the exchange.
 *
 * @remarks This routine exchanges @p n with the value at @p pn and returns
 * the @e previous value at @pn, or the value before the exchange. It is not 
 * sufficient read and store the value at @pn, and then atomically exchange
 * with @p n, and return the stored value. The read and exchange must occur 
 * within one atomic transaction in order to comply with the required 
 * semantics of this routine.
 *
 * @todo Fix the default atomic decrement case so it is really atomic.
 */
intptr_t SLPAtomicXchg(intptr_t * pn, intptr_t n)
{
#if defined(USE_WIN32_ATOMICS)
   return (intptr_t)InterlockedExchange((LPLONG)pn, (LONG)n); 
#elif defined(USE_GCC_I386_ATOMICS)
   return (intptr_t)atomic_xchg32((int32_t *)pn, (int32_t)n);
#elif defined(USE_GCC_X86_64_ATOMICS)
   return (intptr_t)atomic_xchg64((int64_t *)pn, (int64_t)n);
#elif defined(USE_AIX_ATOMICS)
   int value;
   do value = *pn; 
   while (!compare_and_swap((atomic_p)pn, &value, (int)n));
   return value;
#elif defined(USE_ALPHA_ATOMICS)
   return __ATOMIC_EXCH_QUAD(pn, n);
#elif defined(USE_SPARC_ATOMICS)
   return sparc_atomic_xchg_32(pn, n);
#elif defined(USE_APPLE_ATOMICS)
   int32_t value;
   do value = (int32_t)*pn; 
   while (!OSAtomicCompareAndSwap32(o, n, (int32_t *)pn));
   return value;
#else
   intptr_t tn;
   pthread_mutex_lock(&g_atomic_mutex);
   tn = *pn;
   *pn = n;
   pthread_mutex_unlock(&g_atomic_mutex);
   return tn;
#endif
}

/** Acquire a user space spinlock.
 *
 * The lock is ensured by iteratively attempting atomic exchange of 1
 * with the contents of a memory location. We can be sure that when the 
 * return value is zero, that WE were the setter, and not some other thread.
 *
 * @param[in,out] pn - The address of a value to be used as the lock point.
 *
 * @remarks The value at the address in @p pn should be preset to zero
 * as zero indicates the "unlocked" state in this type of lock.
 */
void SLPSpinLockAcquire(intptr_t * pn)
{
   while (SLPAtomicXchg(pn, 1) != 0)
      sleep(0);
}

/** TRY to acquire a user-space spinlock.
 *
 * We can be sure that if the return value is zero, that WE were the 
 * setter, and not some other thread.
 *
 * @param[in,out] pn - The address of a value to be used as the lock point.
 *
 * @return A boolean value; SLP_TRUE if the lock was acquired; SLP_FALSE
 * if the lock is already owned by another thread.
 *
 * @remarks The value at the address in @p pn should be preset to zero
 * as zero indicates the "unlocked" state in this type of lock.
 */
bool SLPSpinLockTryAcquire(intptr_t * pn)
{
   return SLPAtomicXchg(pn, 1)? true: false;
}

/** Release a user-space spin lock.
 *
 * @param[out] pn - The address of the lock control word.
 *
 * @remarks The lock is released by setting the value at @p pn to zero.
 * Simple aligned writes to a single-word addresses are inherently atomic 
 * on decent processor architectures.
 */
void SLPSpinLockRelease(intptr_t * pn)
{
   *pn = 0;
}

/*=========================================================================*/
