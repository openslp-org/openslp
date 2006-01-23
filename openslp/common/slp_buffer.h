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

/** Header file that defines SLP message buffer management routines.
 *
 * Includes structures, constants and functions that used to handle memory 
 * allocation for SLP message buffers.
 *
 * @file       slp_buffer.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#ifndef SLP_BUFFER_H_INCLUDED
#define SLP_BUFFER_H_INCLUDED

#include <stdlib.h>
#include <memory.h>

#include "slp_linkedlist.h"
#include "slp_types.h"

/*!@defgroup CommonCodeBuffers Message Buffers */

/*!@addtogroup CommonCodeBuffers
 * @ingroup CommonCode
 * @{
 */

/** Buffer object holds SLP messages.
 */
typedef struct _SLPBuffer
{
   SLPListItem listitem;   /*!< @brief Allows SLPBuffers to be linked. */
   size_t allocated;       /*!< @brief Allocated size of buffer. */
   uint8_t * start;        /*!< @brief Points to start of space. */
   uint8_t * curpos;       /*!< @brief @p start < @c @p curpos < @p end */
   uint8_t * end;          /*!< @brief Points to buffer limit. */
} * SLPBuffer;

SLPBuffer SLPBufferAlloc(size_t size);

SLPBuffer SLPBufferRealloc(SLPBuffer buf, size_t size);

SLPBuffer SLPBufferDup(SLPBuffer buf);

void SLPBufferFree(SLPBuffer buf);

SLPBuffer SLPBufferListRemove(SLPBuffer * list, SLPBuffer buf);

SLPBuffer SLPBufferListAdd(SLPBuffer * list, SLPBuffer buf);

/*! @} */

#endif /* SLP_BUFFER_H_INCLUDED */

/*=========================================================================*/
