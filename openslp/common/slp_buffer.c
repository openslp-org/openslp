/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_buffer.c                                               */
/*                                                                         */
/* Abstract:    Header file that defines structures and constants that are */
/*              are used to control memory allocation in OpenSLP           */
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


#include <slp_buffer.h> 

#ifdef DEBUG
int G_Debug_SLPBufferAllocCount = 0;
int G_Debug_SLPBufferFreeCount = 0;
#endif




/*=========================================================================*/
void* memdup(const void* src, int srclen)
/* Generic memdup analogous to strdup()                                    */
/*=========================================================================*/
{
    char* result;
    result = (unsigned char*)malloc(srclen);
    if(result)
    {
        memcpy(result,src,srclen);
    }

    return result;
}

/*=========================================================================*/
SLPBuffer SLPBufferAlloc(size_t size)                                         
/* Must be called to initially allocate a SLPBuffer                        */
/*                                                                         */
/* size     - (IN) number of bytes to reallocate                           */ 
/*                                                                         */
/* returns  - newly allocated SLPBuffer or NULL on ENOMEM. An extra byte   */
/*            is allocated to null terminating strings. This extra byte    */
/*            is not counted in the buffer size                            */
/*=========================================================================*/
{
    SLPBuffer result;

    /* allocate an extra byte for null terminating strings */
    result = (SLPBuffer)malloc(sizeof(struct _SLPBuffer) + size + 1);
    if(result)
    {
        result->allocated = size;
        result->start = (unsigned char*)(result + 1);
        result->curpos = result->start;
        result->end = result->start + size; 

#if(defined DEBUG)
        memset(result->start,0xff,size + 1);
        G_Debug_SLPBufferAllocCount ++;
#endif
    }

    return result;
}

/*=========================================================================*/
SLPBuffer SLPBufferRealloc(SLPBuffer buf, size_t size)
/* Must be called to initially allocate a SLPBuffer                        */
/*                                                                         */
/* size     - (IN) number of bytes to allocate                             */
/*                                                                         */
/* returns  - newly (re)allocated SLPBuffer or NULL on ENOMEM. An extra    */
/*            byte is allocated to null terminating strings. This extra    */
/*            byte is not counted in the buffer size                       */
/*=========================================================================*/
{
    SLPBuffer result;
    if(buf)
    {
        if(buf->allocated >= size)
        {
            result = buf;
        }
        else
        {
            /* allocate an extra byte for null terminating strings */
            result = (SLPBuffer)realloc(buf, sizeof(struct _SLPBuffer) +
                                        size + 1);
            result->allocated = size;
        }

        if(result)
        {
            result->start = (unsigned char*)(result + 1);
            result->curpos = result->start;
            result->end = result->start + size;

#if(defined DEBUG)
            memset(result->start,0xff,size + 1);
#endif
        }
    }
    else
    {
        result = SLPBufferAlloc(size);
    }

    return result;
}


/*=========================================================================*/
SLPBuffer SLPBufferDup(SLPBuffer buf)
/* Returns a duplicate buffer.  Duplicate buffer must be freed by a call   */
/* to SLPBufferFree()                                                      */
/*                                                                         */
/* size     - (IN) number of bytes to allocate                             */
/*                                                                         */
/* returns  - a newly allocated SLPBuffer or NULL on ENOMEM                */
/*=========================================================================*/
{
    SLPBuffer dup;

    dup = SLPBufferAlloc(buf->end - buf->start);
    if(dup)
    {
        memcpy(dup->start,buf->start,buf->end - buf->start);       
    }

    return dup;
}


/*=========================================================================*/
void SLPBufferFree(SLPBuffer buf)
/* Free a previously allocated SLPBuffer                                   */
/*                                                                         */
/* msg      - (IN) the SLPBuffer to free                                   */
/*                                                                         */
/* returns  - none                                                         */
/*=========================================================================*/
{
    if(buf)
    {
        free(buf);
#ifdef DEBUG
        G_Debug_SLPBufferFreeCount ++;
#endif
    }
}
