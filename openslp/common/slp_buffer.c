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

#include <slp_buffer.h> 

/*=========================================================================*/
void* memdup(const void* src, int srclen)
/* Generic memdup analogous to strdup()                                    */
/*=========================================================================*/
{
    char* result;
    result = (char*)malloc(srclen);
    if(result)
    {
        memcpy(result,src,srclen);
    }

    return result;
}

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

   result = (SLPBuffer)malloc(sizeof(struct _SLPBuffer) + size);
   if(result)
   {
       result->allocated = size;
       result->start = ((char*)result) + sizeof(struct _SLPBuffer);
       result->curpos = result->start;
       result->end = result->start + size; 
       
       #if(defined DEBUG)
       memset(result->start,0xff,size);
       #endif
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
   SLPBuffer result;
   if(buf)
   {
       if(buf->allocated >= size)
       {
           result = buf;
       }
       else
       {
           result = (SLPBuffer)realloc(buf,sizeof(struct _SLPBuffer) + size);
           result->allocated = size;
       }
       
       if(result)
       {
    	   result->start = ((char*)result) + sizeof(struct _SLPBuffer);
    	   result->curpos = result->start;
    	   result->end = result->start + size;
    	   
    	   #if(defined DEBUG)
    	   memset(result->start,0xff,size);
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
    }
}


/*=========================================================================*/
SLPBuffer SLPBufferListRemove(SLPBuffer* list, SLPBuffer buf)
/* Removes and frees the specified SLPBuffer from a SLPBuffer list         */
/*                                                                         */
/* list (IN/OUT) pointer to the list                                       */
/*                                                                         */
/* buf  (IN) buffer to remove                                              */
/*                                                                         */
/* Returns the previous item in the list (may be NULL)                     */
/*=========================================================================*/
{
    SLPBuffer del = buf;
    buf = (SLPBuffer)buf->listitem.previous;
    ListUnlink((PListItem*)list,(PListItem)del);
    if(buf == 0)
    {
        buf = *list;
    }                   
    SLPBufferFree(del);
    return buf;
}


/*=========================================================================*/
SLPBuffer SLPBufferListAdd(SLPBuffer* list, SLPBuffer buf)
/* Add the specified SLPBuffer from a SLPBuffer list                       */
/*                                                                         */
/* list (IN/OUT) pointer to the list                                       */
/*                                                                         */
/* buf  (IN) buffer to add                                                 */
/*                                                                         */
/* Returns the added item in the list.                                     */
/*=========================================================================*/
{
    ListLink((PListItem*)list,(PListItem)buf);
    return buf;
}
