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

/** Functions for managing SLP message buffers.
 *
 * This file provides a higher level abstraction over malloc and free that
 * deals specifically with message buffer management.
 *
 * @file       slp_buffer.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeBuffers
 */

#include "slp_buffer.h" 
#include "slp_xmalloc.h"

/** Allocates an SLP message buffer.
 *
 * This routines must be called to initially allocate a SLPBuffer.
 *
 * @param[in] size The size of the buffer to be allocated.
 *
 * @return A newly allocated SLPBuffer, or NULL on memory allocation
 *    failure.
 *
 * @remarks An extra byte is allocated to store the terminating null for
 *    strings stored in the buffer. This extra byte is not counted in the 
 *    buffer size.
 */
SLPBuffer SLPBufferAlloc(size_t size)
{
   SLPBuffer result;

   /* Allocate one extra byte for null terminating strings that 
    * occupy the last field of the buffer.
    */
   result = xmalloc(sizeof(struct _SLPBuffer) + size + 1);
   if (result)
   {
      result->allocated = size;
      result->start = (uint8_t *) (result + 1);
      result->curpos = result->start;
      result->end = result->start + size;

#ifdef DEBUG
      memset(result->start, 0x4d, size + 1);
#endif
   } 
   return result;
}

/** Resizes an SLP message buffer.
 *
 * This routine should be called to resize an existing memory buffer
 * that is too small.
 *
 * @param[in] buf    The buffer to be resized.
 * @param[in] size   The new size to reallocate for @p buf.
 *
 * @return A pointer to the newly allocated, or resized buffer is 
 *    returned, or NULL on memory allocation failure.
 *
 * @sa See the @b remarks section for SLPBufferAlloc.
 */
SLPBuffer SLPBufferRealloc(SLPBuffer buf, size_t size)
{
   SLPBuffer result;
   if (buf)
   {
      if (buf->allocated >= size)
         result = buf;
      else
      {
         /* Allocate one extra byte for null terminating strings that 
          * occupy the last field of the buffer.
          */
         result = xrealloc(buf, sizeof(struct _SLPBuffer) + size + 1);
         if (result)
             result->allocated = size;
      }
      if (result)
      {
         result->start = (uint8_t *)(result + 1);
         result->curpos = result->start;
         result->end = result->start + size;

#ifdef DEBUG
         memset(result->start, 0x4d, size + 1);
#endif 
      }
   }
   else
      result = SLPBufferAlloc(size);
   return result;
}

/** Returns a duplicate buffer.  
 *
 * Duplicate buffers must be freed by a call to SLPBufferFree.
 *
 * @param[in] buf The @c SLPBuffer to be duplicated.
 *
 * @return A newly allocated SLPBuffer that is a byte-for-byte copy
 *    of @p buf, or NULL on memory allocation failure.
 */
SLPBuffer SLPBufferDup(SLPBuffer buf)
{
   SLPBuffer dup = 0;
   if (buf && (dup = SLPBufferAlloc(buf->end - buf->start)) != 0)
      memcpy(dup->start, buf->start, buf->end - buf->start + 1);
   return dup;
}

/** Free an SLPBuffer.
 *
 * This routine releases the memory for a previously allocated 
 * @c SLPBuffer object.
 *
 * @param[in] buf The SLPBuffer to be freed.
 */
void SLPBufferFree(SLPBuffer buf)
{
   xfree(buf);
}

/*=========================================================================*/
