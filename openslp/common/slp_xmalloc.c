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

#ifdef DEBUG

#include "slp_xmalloc.h"

/*=========================================================================*/
SLPList G_xmalloc_list = {0,0,0};
FILE*   G_xmalloc_fh = 0;
size_t  G_xmalloc_freemem = 0;
size_t  G_xmalloc_allocmem = 0;
/*=========================================================================*/


/*-------------------------------------------------------------------------*/
void _xmalloc_log(xallocation_t* x)
/*-------------------------------------------------------------------------*/
{
    size_t i;
    if(G_xmalloc_fh)
    {   
        fprintf(G_xmalloc_fh,"xmalloced memory:\n");
        fprintf(G_xmalloc_fh,"   x->where = %s\n",x->where);
        fprintf(G_xmalloc_fh,"   x->size  = %i\n",x->size);
        fprintf(G_xmalloc_fh,"   x->buf   = %p  {",x->buf);
        for (i=0;i<x->size && i < SLPXMALLOC_MAX_BUF_LOG_LEN;i++)
        {
            fprintf(G_xmalloc_fh,"%c",((char*)(x->buf))[i]);
        }
        fprintf(G_xmalloc_fh,"}\n");
    }
}


/*-------------------------------------------------------------------------*/
xallocation_t* _xmalloc_find(void* buf)
/*-------------------------------------------------------------------------*/
{
    xallocation_t* x;     
    x = (xallocation_t*)G_xmalloc_list.head;
    while(x)
    {
        if(x->buf == buf)
        {
            break;
        }
        x = (xallocation_t*)(x->listitem.next);
    }

    return x;
}

/*=========================================================================*/
void* _xmalloc(const char* file,
               int line,
               size_t size)
/*=========================================================================*/
{
    xallocation_t* x;

    if(G_xmalloc_freemem &&
       G_xmalloc_allocmem + size > G_xmalloc_freemem)
    {
        if(G_xmalloc_fh)
        {
            fprintf(G_xmalloc_fh,"\n*** Simulating out of memory error ***\n\n");
        }
        return NULL;
    }

    x = malloc(sizeof(xallocation_t));
    if(x == NULL)
    {
        if(G_xmalloc_fh)
        {
            fprintf(G_xmalloc_fh,"\n*** Real out of memory error ***\n\n");
        }
        return NULL;
    }

    x->buf = malloc(size);
    if(x->buf == NULL)
    {
        if(G_xmalloc_fh)
        {
            fprintf(G_xmalloc_fh,"\n*** Real out of memory error ***\n\n");
        }
        return NULL;
    }
    SLPListLinkTail(&G_xmalloc_list, (SLPListItem*)x);

    x->size = size;
    snprintf(x->where,SLPXMALLOC_MAX_WHERE_LEN,"%s:%i",file,line);
    G_xmalloc_allocmem += size;

    if(G_xmalloc_fh)
    {
        fprintf(G_xmalloc_fh,"Called xmalloc() %s:%i ",file,line);
        _xmalloc_log(x);
    }
    
    
    return x->buf;
}

/*=========================================================================*/
void _xfree(const char* file,
            int line,
            void* buf)
/*=========================================================================*/
{
    xallocation_t* x;
     
    x =_xmalloc_find(buf);
    if(x == NULL)
    {
        if(G_xmalloc_fh)
        {
            fprintf(G_xmalloc_fh, 
                    "*** xfree called on non xmalloced memory ***\n");
        }

        return;
    }

    if(G_xmalloc_fh)
    {
        fprintf(G_xmalloc_fh,"Called xfree() %s:%i ",file,line);
        _xmalloc_log(x);
    }
    
    G_xmalloc_allocmem -= x->size;
    
    free(x->buf);
    
    free(SLPListUnlink(&G_xmalloc_list, (SLPListItem*)x));
}


/*=========================================================================*/
void* _xrealloc(const char* file,
                int line,
                void* buf, 
                size_t size)
/*=========================================================================*/
{
    xallocation_t* x;

    if(G_xmalloc_freemem &&
       G_xmalloc_allocmem + size > G_xmalloc_freemem)
    {
        if(G_xmalloc_fh)
        {
            fprintf(G_xmalloc_fh,"\n*** Simulating out of memory error ***\n\n");
        }
        return NULL;
    }

    if(buf)
    {
        x =_xmalloc_find(buf);
        if(x == NULL)
        {
            if(G_xmalloc_fh)
            {
                fprintf(G_xmalloc_fh, 
                        "*** xrealloc called on non xmalloced memory ***\n");
            }
    
            return NULL;
        }
        G_xmalloc_allocmem -= x->size;
    }
    else
    {
        x = malloc(sizeof(xallocation_t));
        if(x == NULL)
        {
            if(G_xmalloc_fh)
            {
                fprintf(G_xmalloc_fh,"\n*** Real out of memory error ***\n\n");
            }
            return NULL;
        }
        
        SLPListLinkTail(&G_xmalloc_list, (SLPListItem*)x);
    }

    x->buf = realloc(buf,size);
    if(x->buf == NULL)
    {
        if(G_xmalloc_fh)
        {
            fprintf(G_xmalloc_fh,"\n*** Real out of memory error ***\n\n");
        }
        return NULL;
    }
    snprintf(x->where,SLPXMALLOC_MAX_WHERE_LEN,"%s:%i",file,line);
    x->size = size;
    
    G_xmalloc_allocmem += size;

    if(G_xmalloc_fh)
    {
        fprintf(G_xmalloc_fh,"Called xrealloc() %s:%i ", file, line);
        _xmalloc_log(x);
    }
    
    return x->buf;
}

                     
/*=========================================================================*/
char* _xstrdup(const char* file,
               int line,
               const char* str)
/*=========================================================================*/
{
    xallocation_t* x;

    size_t strlength = strlen(str);

    if(G_xmalloc_freemem &&
       G_xmalloc_allocmem + strlength > G_xmalloc_freemem)
    {
        if(G_xmalloc_fh)
        {
            fprintf(G_xmalloc_fh,"\n*** Simulating out of memory error ***\n\n");
        }
        return NULL;
    }

    x = malloc(sizeof(xallocation_t));
    if(x == NULL)
    {
        if(G_xmalloc_fh)
        {
            fprintf(G_xmalloc_fh,"\n*** Real out of memory error ***\n\n");
        }
        return NULL;
    }

    x->buf = strdup(str);
    if(x->buf == NULL)
    {
        if(G_xmalloc_fh)
        {
            fprintf(G_xmalloc_fh,"\n*** Real out of memory error ***\n\n");
        }
        return NULL;
    }
    x->size = strlength;
    snprintf(x->where,SLPXMALLOC_MAX_WHERE_LEN,"%s:%i",file,line);
    G_xmalloc_allocmem += strlength;

    if(G_xmalloc_fh)
    {
        fprintf(G_xmalloc_fh,"Called xstrdup() %s:%i ",file,line);
        _xmalloc_log(x);
    }
    
    SLPListLinkTail(&G_xmalloc_list, (SLPListItem*)x);

    return (char*)x->buf;
}




/*=========================================================================*/
int xmalloc_init(const char* filename, size_t freemem)
/*=========================================================================*/
{
    G_xmalloc_fh = fopen(filename, "w");
    if(G_xmalloc_fh)
    {
        return 0;
    }

    G_xmalloc_freemem = freemem;

    return 1;
}


/*=========================================================================*/
int xmalloc_report()
/*=========================================================================*/
{
    xallocation_t* x;

    if(G_xmalloc_fh)
    {
        fprintf(G_xmalloc_fh, 
                "\n*** Start of xmalloc_report ***\n");
    }
    x = (xallocation_t*)G_xmalloc_list.head;
    while(x)
    {
        _xmalloc_log(x);
        x = (xallocation_t*)(x->listitem.next);;
    }

    if(G_xmalloc_fh)
    {
	fprintf(G_xmalloc_fh, 
		"*** End of xmalloc_report ***\n\n");
    }

    return 0;

}


/*=========================================================================*/
void xmalloc_deinit()
/*=========================================================================*/
{
    xmalloc_report();

    if(G_xmalloc_fh)
    {
        fclose(G_xmalloc_fh);
        G_xmalloc_fh = NULL;
    }

    while(G_xmalloc_list.count)
    {
        free((xallocation_t*)SLPListUnlink(&G_xmalloc_list,G_xmalloc_list.head));
    }
    memset(&G_xmalloc_list,0,sizeof(G_xmalloc_list));
}

#endif
