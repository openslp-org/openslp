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
/* Author(s):   Matthew Peterson                                           */
/*                                                                         */
/***************************************************************************/

#include <malloc.h>

#include <slp_buffer.h> 

/*=========================================================================*/
SLPBuffer SLPBufferAlloc(int size)                                         
/* Must be called to initially allocate a SLPBuffer                        */
/*                                                                         */
/* size     - (IN) number of bytes to reallocate                           */ 
/*                                                                         */
/* returns  - newly allocated SLPBuffer or NULL on ENOMEM                  */
/*=========================================================================*/
{
   SLPBuffer result;

   result = (SLPBuffer)malloc(sizeof(struct _SLPBuffer));
   if(result)
   {
       if(size)
       {   
           result->start = (char*)malloc(size);
           if(result->start)
           {
               #if(defined DEBUG)
               memset(result->start,0xff,size);
               #endif
               
               result->curpos = result->start;
               result->end = result->start + size;
           }
           else
           { 
               free(result);
               result = 0;
           }
       }
   }

   return result;
}

/*=========================================================================*/
SLPBuffer SLPBufferRealloc(SLPBuffer buf, int size)
/* Must be called to initially allocate a SLPBuffer                        */
/*                                                                         */
/* size     - (IN) number of bytes to allocate                             */
/*                                                                         */
/* returns  - a newly allocated SLPBuffer or NULL on ENOMEM                */
/*=========================================================================*/
{
   
   if(buf)
   {
       if(size)
       {
           buf->start = (char*)realloc(buf->start,size);
           if(buf->start)
           {
               #if(defined DEBUG)
               memset(buf->start,0xff,size);
               #endif
               buf->curpos = buf->start;
               buf->end = buf->start + size;
           }
           else
           { 
               free(buf);
               buf = 0;
           }
       }
   }
   else
   {
       buf = SLPBufferAlloc(size);
   }

   return buf;
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
        free(buf->start);
        free(buf);
    }
}
