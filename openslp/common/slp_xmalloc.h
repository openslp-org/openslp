/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_xmalloc.h                                              */
/*                                                                         */
/* Abstract:    Debug memory allocator                                     */
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

#ifndef SLP_XMALLOC_H_INCLUDED
#define SLP_XMALLOC_H_INCLUDED

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#ifdef DEBUG

#include "slp_linkedlist.h"

#define SLPXMALLOC_MAX_WHERE_LEN    256
#define SLPXMALLOC_MAX_BUF_LOG_LEN  32

/*=========================================================================*/
extern SLPList G_xmalloc_list;
/*=========================================================================*/

/*=========================================================================*/
typedef struct xallocation
/*=========================================================================*/
{
    SLPListItem listitem;
    char        where[SLPXMALLOC_MAX_WHERE_LEN];
    void*       buf;
    size_t      size;
}xallocation_t;


/*=========================================================================*/
void* _xmalloc(const char* file,
               int line,
               size_t size);
/*=========================================================================*/


/*=========================================================================*/
void* _xrealloc(const char* file,
                int line,
                void* buf, 
                size_t size);
/*=========================================================================*/


/*=========================================================================*/
void _xfree(const char* file,
            int line,
            void* buf);
/*=========================================================================*/


/*=========================================================================*/
char* _xstrdup(const char* file,
               int line,
               const char* str);
/*=========================================================================*/

                     
/*=========================================================================*/
int xmalloc_init(const char* filename, size_t freemem);
/*=========================================================================*/


/*=========================================================================*/
int xmalloc_report();
/*=========================================================================*/


/*=========================================================================*/
void xmalloc_deinit();
/*=========================================================================*/


#define xmalloc(x) _xmalloc(__FILE__,__LINE__,(x))
#define xrealloc(x,y) _xrealloc(__FILE__,__LINE__,(x),(y))
#define xfree(x) _xfree(__FILE__,__LINE__,(x))
#define xstrdup(x) _xstrdup(__FILE__,__LINE__,(x))

#else

#define xmalloc(x) malloc((x))
#define xrealloc(x,y) realloc((x),(y))
#define xfree(x) free((x))
#define xstrdup(x) strdup((x))
#endif


#endif
