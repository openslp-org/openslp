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

#include <slp_linkedlist.h>

/*=========================================================================*/
typedef struct _SLPBuffer                                                  
/*=========================================================================*/
{
    ListItem listitem;
    /* ListItem so that SLPBuffers can be linked into a list*/
    
    char*   start;  
    /* ALWAYS points to the start of the malloc() buffer  */

    char*   curpos;
    /* "slider" pointer.  Range is ALWAYS (start < curpos < end) */

    char*   end;
    /* ALWAYS set to point to the byte after the last meaningful byte */
    /* Data beyond this index may not be valid */
}*SLPBuffer;   
                                  

/*=========================================================================*/
SLPBuffer SLPBufferAlloc(int size);
/* Must be called to initially allocate a SLPBuffer                        */           
/*                                                                         */
/* size     - (IN) number of bytes to allocate                             */
/*                                                                         */
/* returns  - a newly allocated SLPBuffer or NULL on ENOMEM                */
/*=========================================================================*/


/*=========================================================================*/
SLPBuffer SLPBufferRealloc(SLPBuffer buf, int size);
/* Must be called to initially allocate a SLPBuffer                        */ 
/*                                                                         */
/* size     - (IN) number of bytes to allocate                             */
/*                                                                         */
/* returns  - a newly allocated SLPBuffer or NULL on ENOMEM                */
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
void* memdup(const void* src, int srclen);
/*=========================================================================*/


#endif
