/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_buffer.h                                               */
/*                                                                         */
/* Abstract:    Header file that defines structures and constants and      */
/*              functions that are used to handle memory allocation for    */
/*              slp message buffers.                                       */
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

#if(!defined SLP_BUFFER_H_INCLUDED)
    #define SLP_BUFFER_H_INCLUDED

    #include <stdlib.h>

    #include <slp_linkedlist.h>

    #ifdef DEBUG
extern int G_Debug_SLPBufferAllocCount;
extern int G_Debug_SLPBufferFreeCount;
    #endif




/*=========================================================================*/
typedef struct _SLPBuffer                                                  
/*=========================================================================*/
{
    SLPListItem listitem;
    /* SLPListItem so that SLPBuffers can be linked into a list*/

    /* the allocated size of this buffer (the actual malloc'd size is
       one byte more than this to null terminate C strings) */
    size_t  allocated;

    unsigned char*   start;  
    /* ALWAYS points to the start of the malloc() buffer  */

    unsigned char*   curpos;
    /* "slider" pointer.  Range is ALWAYS (start < curpos < end) */

    unsigned char*   end;
    /* ALWAYS set to point to the byte after the last meaningful byte */
    /* Data beyond this index may not be valid */
}*SLPBuffer;   


/*=========================================================================*/
SLPBuffer SLPBufferAlloc(size_t size); 
/* Must be called to initially allocate a SLPBuffer                        */
/*                                                                         */
/* size     - (IN) number of bytes to allocate                             */
/*                                                                         */
/* returns  - newly allocated SLPBuffer or NULL on ENOMEM. An extra byte   */
/*            is allocated to null terminating strings. This extra byte    */
/*            is not counted in the buffer size                            */
/*=========================================================================*/


/*=========================================================================*/
SLPBuffer SLPBufferRealloc(SLPBuffer buf, size_t size); 
/* Must be called to initially allocate a SLPBuffer                        */
/*                                                                         */
/* size     - (IN) number of bytes to allocate                             */
/*                                                                         */
/* returns  - newly (re)allocated SLPBuffer or NULL on ENOMEM. An extra    */
/*            byte is allocated to null terminating strings. This extra    */
/*            byte is not counted in the buffer size                       */
/*=========================================================================*/

/*=========================================================================*/
SLPBuffer SLPBufferDup(SLPBuffer buf);
/* Returns a duplicate buffer.  Duplicate buffer must be freed by a call   */
/* to SLPBufferFree()                                                      */
/*                                                                         */
/* size     - (IN) number of bytes to allocate                             */
/*                                                                         */
/* returns  - a newly allocated SLPBuffer or NULL on ENOMEM                */
/*=========================================================================*/


/*=========================================================================*/
void SLPBufferFree(SLPBuffer buf);
/* Free a previously allocated SLPBuffer                                   */
/*                                                                         */
/* msg      - (IN) the SLPBuffer to free                                   */
/*                                                                         */
/* returns  - none                                                         */
/*=========================================================================*/


/*=========================================================================*/
SLPBuffer SLPBufferListRemove(SLPBuffer* list, SLPBuffer buf);
/* Removed the specified SLPBuffer from a SLPBuffer list                   */
/*                                                                         */
/* list (IN/OUT) pointer to the list                                       */
/*                                                                         */
/* buf  (IN) buffer to remove                                              */
/*                                                                         */
/* Returns the previous item in the list (may be NULL)                     */
/*=========================================================================*/


/*=========================================================================*/
SLPBuffer SLPBufferListAdd(SLPBuffer* list, SLPBuffer buf);
/* Add the specified SLPBuffer from a SLPBuffer list                       */
/*                                                                         */
/* list (IN/OUT) pointer to the list                                       */
/*                                                                         */
/* buf  (IN) buffer to add                                                 */
/*                                                                         */
/* Returns the added item in the list.                                     */
/*=========================================================================*/


/*=========================================================================*/
void* memdup(const void* src, int srclen);
/*=========================================================================*/


#endif
