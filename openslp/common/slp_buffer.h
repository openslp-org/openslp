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
/* Author(s):   Matthew Peterson                                           */
/*                                                                         */
/***************************************************************************/
#if(!defined SLP_BUFFER_H_INCLUDED)
#define SLP_BUFFER_H_INCLUDED

/*=========================================================================*/
typedef struct _SLPBuffer                                                  
/*=========================================================================*/
{
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
void SLPBufferFree(SLPBuffer buf);
/* Free a previously allocated SLPBuffer                                   */
/*                                                                         */
/* msg      - (IN) the SLPBuffer to free                                   */
/*                                                                         */
/* returns  - none                                                         */
/*=========================================================================*/

#endif
