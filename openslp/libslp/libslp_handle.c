/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slplib_handle.h                                            */
/*                                                                         */
/* Abstract:    Implementation for SLPOpen() and SLPClose() functions      */
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

#include "slp.h"
#include "libslp.h"

/*=========================================================================*/
int G_OpenSLPHandleCount = 0;
/* Global variable that keeps track of the number of handles that are open */
/*=========================================================================*/


/*=========================================================================*/
SLPError SLPOpen(const char *pcLang, SLPBoolean isAsync, SLPHandle *phSLP)
/*                                                                         */
/* Returns a SLPHandle handle in the phSLP parameter for the language      */
/* locale passed in as the pcLang parameter.  The client indicates if      */
/* operations on the handle are to be synchronous or asynchronous          */
/* through the isAsync parameter.  The handle encapsulates the language    */
/* locale for SLP requests issued through the handle, and any other        */
/* resources required by the implementation.  However, SLP properties      */
/* are not encapsulated by the handle; they are global.  The return        */
/* value of the function is an SLPError code indicating the status of      */
/* the operation.  Upon failure, the phSLP parameter is NULL.              */
/*                                                                         */
/* An SLPHandle can only be used for one SLP API operation at a time.      */
/* If the original operation was started asynchronously, any attempt to    */
/* start an additional operation on the handle while the original          */
/* operation is pending results in the return of an SLP_HANDLE_IN_USE      */
/* error from the API function.  The SLPClose() API function terminates    */
/* any outstanding calls on the handle.  If an implementation is unable    */
/* to support a asynchronous( resp.  synchronous) operation, due to        */
/* memory constraints or lack of threading support, the                    */
/* SLP_NOT_IMPLEMENTED flag may be returned when the isAsync flag is       */
/* SLP_TRUE (resp.  SLP_FALSE).                                            */
/*                                                                         */
/* pcLang   A pointer to an array of characters containing the RFC 1766    */
/*          Language Tag RFC 1766 for the natural language locale of       */
/*          requests and registrations issued on the handle. Pass in NULL  */
/*          or the empty string, "" to use the default locale              */
/*                                                                         */
/* isAsync  An SLPBoolean indicating whether the SLPHandle should be opened*/
/*          for asynchronous operation or not.                             */
/*                                                                         */
/* phSLP    A pointer to an SLPHandle, in which the open SLPHandle is      */
/*          returned.  If an error occurs, the value upon return is NULL.  */
/*                                                                         */
/* Returns  SLPError code                                                  */
/*=========================================================================*/
{
    SLPError        result = SLP_OK;
    PSLPHandleInfo  handle = 0;
    
    /*------------------------------*/
    /* check for invalid parameters */
    /*------------------------------*/
    if(phSLP == 0)
    {
        result =  SLP_PARAMETER_BAD;
        goto FINISHED;
    }
    
    /* assign out param to zero in just for paranoia */
    *phSLP = 0;
    

    /*-------------------------------------------------------*/
    /* TODO: remove this line when you implement async calls */
    /*-------------------------------------------------------*/
    if(isAsync == SLP_TRUE)
    {
        result =  SLP_NOT_IMPLEMENTED;
        goto FINISHED;
    }

    /*------------------------------------*/
    /* allocate a SLPHandleInfo structure */
    /*------------------------------------*/
    handle = (PSLPHandleInfo)malloc(sizeof(SLPHandleInfo));
    if(handle == 0)
    {
        result =  SLP_PARAMETER_BAD;
        goto FINISHED;
    }
    memset(handle,0,sizeof(SLPHandleInfo));
    
    /*-------------------------------*/
    /* Set the language tag          */
    /*-------------------------------*/
    if(pcLang && *pcLang)
    {
        handle->langtaglen = strlen(pcLang);
        handle->langtag = (char*)malloc(handle->langtaglen + 1);
        if(handle->langtag == 0)
        {
            free(handle);
            result =  SLP_PARAMETER_BAD;
            goto FINISHED;
        }
        memcpy(handle->langtag,pcLang,handle->langtaglen + 1); 
    }
    else
    {
        handle->langtaglen = strlen(SLPGetProperty("net.slp.locale"));
        handle->langtag = (char*)malloc(handle->langtaglen + 1);
        if(handle->langtag == 0)
        {
            free(handle);
            result =  SLP_PARAMETER_BAD;
            goto FINISHED;
        }
        memcpy(handle->langtag,SLPGetProperty("net.slp.locale"),handle->langtaglen + 1);       
    }

    /*---------------------------------------------------------*/
    /* Seed the XID generator if this is the first open handle */
    /*---------------------------------------------------------*/
    if(G_OpenSLPHandleCount == 0)
    {
        SLPXidSeed();
    }
     
    handle->inUse = SLP_FALSE;
    handle->isAsync = isAsync;
    handle->sig = SLP_HANDLE_SIG;

    G_OpenSLPHandleCount ++;  
    
    *phSLP = (SLPHandle)handle;
    
    
    FINISHED:                  
    
    if(result)
    {
        *phSLP = 0;
    }
    
    return result;
}
         

/*=========================================================================*/
void SLPClose(SLPHandle hSLP)                                             
/*                                                                         */
/* Frees all resources associated with the handle.  If the handle was      */
/* invalid, the function returns silently.  Any outstanding synchronous    */
/* or asynchronous operations are cancelled so their callback functions    */
/* will not be called any further.                                         */
/*                                                                         */
/* SLPHandle    A SLPHandle handle returned from a call to SLPOpen().      */
/*=========================================================================*/
{
    PSLPHandleInfo   handle;
    
    /*------------------------------*/
    /* check for invalid parameters */
    /*------------------------------*/
    if(hSLP == 0 || *(unsigned long*)hSLP != SLP_HANDLE_SIG)
    {
        return;
    }
    
    handle = (PSLPHandleInfo)hSLP;

    if(handle->isAsync)
    {
        /* TODO: stop the usage of this handle (kill threads, etc) */
    }
    if(handle->langtag)
    {
        free(handle->langtag);
    }

    handle->sig = 0;  /* If they use the handle again, it won't be valid */

    free(hSLP);
}
