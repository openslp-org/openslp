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
 * @ingroup    CommonCodeXMalloc
 */

#ifndef SLP_XMALLOC_H_INCLUDED
#define SLP_XMALLOC_H_INCLUDED

/*!@defgroup CommonCodeXMalloc Memory
 * @ingroup CommonCodeDebug
 * @{
 */

#include "slp_types.h" 

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

void * slp_xmalloc(const char * file, int line, size_t size);
void * slp_xcalloc(const char * file, int line, int numblks, size_t size);
void * slp_xrealloc(const char * file, int line, void * ptr, size_t size);
char * slp_xstrdup(const char * file, int line, const char * str);
void * slp_xmemdup(const char * file, int line, const void * ptr, size_t size);
void   slp_xfree(const char * file, int line, void * ptr);

int slp_xmalloc_init(const char * filename, size_t freemem);
int slp_xmalloc_report(void);
void slp_xmalloc_deinit(void);

#define xmalloc(s)      slp_xmalloc(__FILE__,__LINE__,(s))
#define xcalloc(n,s)    slp_xcalloc(__FILE__,__LINE__,(n),(s))
#define xrealloc(p,s)   slp_xrealloc(__FILE__,__LINE__,(p),(s))
#define xfree(p)        slp_xfree(__FILE__,__LINE__,(p))
#define xstrdup(p)      slp_xstrdup(__FILE__,__LINE__,(p))
#define xmemdup(p,s)    slp_xmemdup(__FILE__,__LINE__,(p),(s))

#define xmalloc_init    slp_xmalloc_init
#define xmalloc_report  slp_xmalloc_report
#define xmalloc_deinit  slp_xmalloc_deinit

#else    /* ?DEBUG */

void * slp_xmemdup(const void * ptr, size_t srclen);

#define xmalloc   malloc
#define xcalloc   calloc
#define xrealloc  realloc
#define xfree     free
#define xstrdup   strdup
#define xmemdup   slp_xmemdup

#endif   /* ?DEBUG */

/*! @} */

#endif   /* SLP_XMALLOC_H_INCLUDED */

/*=========================================================================*/
