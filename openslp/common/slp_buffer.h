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
/* Copyright (c) 1995, 1999  Caldera Systems, Inc.                         */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify it */
/* under the terms of the GNU Lesser General Public License as published   */
/* by the Free Software Foundation; either version 2.1 of the License, or  */
/* (at your option) any later version.                                     */
/*                                                                         */
/*     This program is distributed in the hope that it will be useful,     */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       */
/*     GNU Lesser General Public License for more details.                 */
/*                                                                         */
/*     You should have received a copy of the GNU Lesser General Public    */
/*     License along with this program; see the file COPYING.  If not,     */
/*     please obtain a copy from http://www.gnu.org/copyleft/lesser.html   */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*     Please submit patches to http://www.openslp.org                     */
/*                                                                         */
/***************************************************************************/

#if(!defined SLP_BUFFER_H_INCLUDED)
#define SLP_BUFFER_H_INCLUDED

#include <malloc.h>

#include <slp_linkedlist.h>

/*=========================================================================*/
typedef struct _SLPBuffer                                                  
/*=========================================================================*/
{
    SLPListItem listitem;
    /* SLPListItem so that SLPBuffers can be linked into a list*/
    
    /* the allocated size of this buffer (the actual malloc'd size is
       one byte more than this to null terminate C strings) */
    size_t  allocated;

    char*   start;  
    /* ALWAYS points to the start of the malloc() buffer  */

    char*   curpos;
    /* "slider" pointer.  Range is ALWAYS (start < curpos < end) */

    char*   end;
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
