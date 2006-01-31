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

/** Header file for debug memory allocator.
 *
 * @file       slp_xmalloc.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#ifndef SLP_XMALLOC_H_INCLUDED
#define SLP_XMALLOC_H_INCLUDED

/*!@defgroup CommonCodeXMalloc Debug Memory */

/*!@addtogroup CommonCodeXMalloc
 * @ingroup CommonCode
 * @{
 */

#include <stddef.h>

#ifdef DEBUG

#include "slp_linkedlist.h"

#define SLPXMALLOC_MAX_WHERE_LEN    256
#define SLPXMALLOC_MAX_BUF_LOG_LEN  32

typedef struct xallocation
{
   SLPListItem listitem;
   char where[SLPXMALLOC_MAX_WHERE_LEN];
   void * buf;
   size_t size;
} xallocation_t;

void * _xmalloc(const char * file, int line, size_t size);
void * _xcalloc(const char * file, int line, int numblks, size_t size);
void * _xrealloc(const char * file, int line, void * ptr, size_t size);
char * _xstrdup(const char * file, int line, const char * str);
void * _xmemdup(const char * file, int line, const void * ptr, size_t size);
void   _xfree(const char * file, int line, void * ptr);

int xmalloc_init(const char * filename, size_t freemem);
int xmalloc_report(void);
void xmalloc_deinit(void);

#define xmalloc(s)      _xmalloc(__FILE__,__LINE__,(s))
#define xcalloc(n,s)    _xcalloc(__FILE__,__LINE__,(n),(s))
#define xrealloc(p,s)   _xrealloc(__FILE__,__LINE__,(p),(s))
#define xfree(p)        _xfree(__FILE__,__LINE__,(p))
#define xstrdup(p)      _xstrdup(__FILE__,__LINE__,(p))
#define xmemdup(p,s)    _xmemdup(__FILE__,__LINE__,(p),(s))

#else    /* ?DEBUG */

#include <stdlib.h>
#include <string.h>

void * memdup(const void * ptr, size_t srclen);

#ifdef _WIN32
# define strdup   _strdup
#endif

#define xmalloc   malloc
#define xcalloc   calloc
#define xrealloc  realloc
#define xfree     free
#define xstrdup   strdup
#define xmemdup   memdup

#endif   /* ?DEBUG */

/*! @} */

#endif   /* SLP_XMALLOC_H_INCLUDED */

/*=========================================================================*/
