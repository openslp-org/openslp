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

/** Debug memory allocator routines.
 *
 * The routines in this file provide an abstraction layer over the standard
 * library memory allocation routines, malloc and free. This layer of 
 * indirection allows OpenSLP developers to ensure that memory is properly
 * free'd when it should be, and to simulate out of memory conditions in
 * order to verify error paths in test code.
 *
 * @file       slp_xmalloc.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeXMalloc
 */

#include "slp_types.h"
#include "slp_xmalloc.h"

#ifdef DEBUG

static SLPList G_xmalloc_list = {0, 0, 0};
static FILE * G_xmalloc_fh = 0;
static size_t G_xmalloc_freemem = 0;
static size_t G_xmalloc_allocmem = 0;

/** Log a debug memory allocator event.
 *
 * @param[in] x - A pointer to the debug allocator block about which to 
 *    log information.
 *
 * @internal
 */
static void _xmalloc_log(xallocation_t * x)
{
   unsigned int i;
   if (G_xmalloc_fh)
   {   
      fprintf(G_xmalloc_fh,"xmalloced memory:\n");
      fprintf(G_xmalloc_fh,"   x->where = %s\n",x->where);
      fprintf(G_xmalloc_fh,"   x->size  = %i\n",x->size);
      fprintf(G_xmalloc_fh,"   x->buf   = %p  {",x->buf);
      for (i = 0; i < x->size && i < SLPXMALLOC_MAX_BUF_LOG_LEN; i++)
         fprintf(G_xmalloc_fh,"%c",((char*)(x->buf))[i]);
      fprintf(G_xmalloc_fh,"}\n");
   }
}

/** Locates an allocated block of memory in the debug allocator list.
 *
 * @param[in] ptr - The user address of the block for which to search.
 * 
 * @return A pointer to the debug allocator address of the block, if
 *    found, or NULL if not.
 *
 * @internal
 */
static xallocation_t * _xmalloc_find(void * ptr)
{
   xallocation_t * x;
   x = (xallocation_t *)G_xmalloc_list.head;
   while (x)
   {
      if (x->buf == ptr)
         break;
      x = (xallocation_t *)x->listitem.next;
   }
   return x;
}

/** Allocates a block of memory (DEBUG).
 *
 * @param[in] file - The file where @e xfree was called.
 * @param[in] line - The line number where @e xfree was called.
 * @param[in] size - The size of the block to be allocated.
 *
 * @return A pointer to the newly allocated memory block.
 */
void * _xmalloc(const char * file, int line, size_t size)
{
   xallocation_t * x;

   if (size == 0)
      return 0;

   if (G_xmalloc_freemem && G_xmalloc_allocmem + size > G_xmalloc_freemem)
   {
      if (G_xmalloc_fh)
         fprintf(G_xmalloc_fh,"\n*** Simulating out of memory error ***\n\n");
      return 0;
   }

   x = malloc(sizeof(xallocation_t));
   if (x == 0)
   {
      if (G_xmalloc_fh)
         fprintf(G_xmalloc_fh,"\n*** Real out of memory error ***\n\n");
      return 0;
   }

   x->buf = malloc(size);
   if (x->buf == 0)
   {
      if(G_xmalloc_fh)
         fprintf(G_xmalloc_fh,"\n*** Real out of memory error ***\n\n");
      return 0;
   }
   SLPListLinkTail(&G_xmalloc_list, (SLPListItem *)x);

   x->size = size;
   snprintf(x->where, SLPXMALLOC_MAX_WHERE_LEN, "%s:%i", file, line);
   G_xmalloc_allocmem += size;

   if (G_xmalloc_fh)
   {
      fprintf(G_xmalloc_fh, "Called xmalloc() %s:%i ",file,line);
      _xmalloc_log(x);
   }
   return x->buf;
}

/** Allocates and clears a block of memory (DEBUG).
 *
 * Allocates a range of bytes of size @p numblks * @p size, and zeros 
 * the entire memory block.
 *
 * @param[in] file - The file where @e xfree was called.
 * @param[in] line - The line number where @e xfree was called.
 * @param[in] numblks - The number of blocks of size @p size to allocate.
 * @param[in] size - The size of each block to allocate. 
 *
 * @return A pointer to the newly allocated and cleared memory block.
 */
void * _xcalloc(const char * file, int line, int numblks, size_t size)
{
   size_t blksz = numblks * size;
   void * ptr = _xmalloc(file, line, blksz);
   if (ptr)
      memset(ptr, 0, blksz);
   return ptr;
}

/** Resizes a block of memory (DEBUG).
 *
 * @param[in] file - The file where @e xmemdup was called.
 * @param[in] line - The line number where @e xmemdup was called.
 * @param[in] buf - A pointer to a buffer to be resized.
 *
 * @return A pointer to the new block on success, or null on failure.
 *
 * @remarks This routine emulates the standard library routine, 
 *    realloc, including esoteric functionality, such as passing
 *    NULL for @p buf actually allocates a new buffer, passing 0 
 *    for @p size actually allocates a new buffer.
 */
void * _xrealloc(const char * file, int line, void * ptr, size_t size)
{
   xallocation_t * x;

   if (!ptr)
      return _xmalloc(file, line, size);

   if (!size)
   {
      _xfree(file, line, ptr);
      return 0;
   }

   x = _xmalloc_find(ptr);
   if (x != 0)
   {
      void * newptr = ptr;
      if (x->size != size)
      {
         newptr = _xmalloc(file, line, size);
         memcpy(newptr, ptr, x->size);
         _xfree(file, line, x);
      }
      return newptr;
   }

   if (G_xmalloc_fh)
      fprintf(G_xmalloc_fh, "*** xrealloc called on "
            "non-xmalloc'd memory ***\n");

   return 0;
}
                     
/** Duplicates a string buffer (DEBUG).
 *
 * @param[in] file - The file where @e xmemdup was called.
 * @param[in] line - The line number where @e xmemdup was called.
 * @param[in] str - A pointer to the string to be duplicated.
 * 
 * @return A pointer to the duplicated string, or NULL on memory
 *    allocation failure.
 */
char * _xstrdup(const char * file, int line, const char * str)
{
   size_t strsz = strlen(str) + 1;
   char * ptr = _xmalloc(file, line, strsz);
   if (ptr)
      memcpy(ptr, str, strsz);
   return ptr;
}

/** Duplicates a sized memory buffer (DEBUG).
 *
 * @param[in] file - The file where @e xmemdup was called.
 * @param[in] line - The line number where @e xmemdup was called.
 * @param[in] ptr - A pointer to the memory block to be duplicated.
 * @param[in] size - The size of @p ptr in bytes.
 * 
 * @return A pointer to the duplicated memory block, or NULL on memory
 *    allocation failure.
 */
void * _xmemdup(const char * file, int line, const void * ptr, size_t size)
{
   void * cpy = _xmalloc(file, line, size);
   if (cpy)
      memcpy(cpy, ptr, size);
   return cpy;
}

/** Free's a block of memory (DEBUG).
 *
 * @param[in] file - The file where @e xfree was called.
 * @param[in] line - The line number where @e xfree was called.
 * @param[in] ptr - The address of the block to be free'd.
 */
void _xfree(const char * file, int line, void * ptr)
{
   xallocation_t * x;

   x =_xmalloc_find(ptr);
   if (x == 0)
   {
      if (G_xmalloc_fh)
         fprintf(G_xmalloc_fh, "*** xfree called on "
               "non-xmalloc'd memory ***\n");
      return;
   }
   if (G_xmalloc_fh)
   {
      fprintf(G_xmalloc_fh, "Called xfree at %s:%i ", file, line);
      _xmalloc_log(x);
   }

   G_xmalloc_allocmem -= x->size;

   free(x->buf);
   free(SLPListUnlink(&G_xmalloc_list, (SLPListItem *)x));
}

/** Initialize the debug memory allocator.
 *
 * @param[in] filename - The filename of the debug memory log.
 * @param[in] freemem - The amount of simulated remaining memory.
 *
 * @return A boolean true (1) on success, or false (0) the log file 
 *    fails to open.
*/
int xmalloc_init(const char * filename, size_t freemem)
{
   G_xmalloc_fh = fopen(filename, "w");
   if (G_xmalloc_fh == 0)
      return 0;
   G_xmalloc_freemem = freemem;
   return 1;
}

/** Report unfreed memory allocations to the debug memory log.
 *
 * @return 0
 */
int xmalloc_report(void)
{
   xallocation_t * x;

   if (G_xmalloc_fh)
      fprintf(G_xmalloc_fh, "\n*** Start of xmalloc_report ***\n");

   x = (xallocation_t *)G_xmalloc_list.head;
   while (x)
   {
      _xmalloc_log(x);
      x = (xallocation_t *)x->listitem.next;
   }

   if (G_xmalloc_fh)
      fprintf(G_xmalloc_fh, "*** End of xmalloc_report ***\n\n");

   return 0;
}

/** Deinitialize the debug memory allocator.
 */
void xmalloc_deinit(void)
{
   xmalloc_report();

   if (G_xmalloc_fh)
   {
      fclose(G_xmalloc_fh);
      G_xmalloc_fh = NULL;
   }
   while (G_xmalloc_list.count)
      free(SLPListUnlink(&G_xmalloc_list, G_xmalloc_list.head));

   memset(&G_xmalloc_list, 0, sizeof(G_xmalloc_list));
}

#else    /* ?DEBUG */

/** Duplicates a sized memory block.
 *
 * @param[in] ptr - A pointer to the memory block to be duplicated.
 * @param[in] size - The size of @p ptr in bytes.
 * 
 * @return A pointer to the duplicated memory block, or NULL on memory
 *    allocation failure.
 */
void * _xmemdup(const void * ptr, size_t size)
{
   void * cpy = malloc(size);
   if (cpy)
      memcpy(cpy, ptr, size);
   return cpy;
}

#endif   /* ?DEBUG */

/*=========================================================================*/
