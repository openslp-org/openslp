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
/* See header file for detailed documentation                              */
/*=========================================================================*/
{
    PSLPHandleInfo  handle;
    
    /*------------------------------*/
    /* check for invalid parameters */
    /*------------------------------*/
    if(pcLang == 0 || phSLP == 0)
    {
        return SLP_PARAMETER_BAD;
    }
    
    *phSLP = 0;

    /* TODO: remove this line when you implement async calls */
    if(isAsync == SLP_TRUE)
    {
        return SLP_NOT_IMPLEMENTED;
    }

    /*------------------------------------*/
    /* allocate a SLPHandleInfo structure */
    /*------------------------------------*/
    handle = (PSLPHandleInfo)malloc(sizeof(SLPHandleInfo));
    if(handle == 0)
    {
        return SLP_MEMORY_ALLOC_FAILED;
    }
    memset(handle,0,sizeof(SLPHandleInfo));

    
    /*-------------------------------*/
    /* Set the language tag          */
    /*-------------------------------*/
    handle->langtaglen = strlen(pcLang);
    handle->langtag = (char*)malloc(handle->langtaglen);
    if(handle->langtag == 0)
    {
        free(handle);
        return SLP_MEMORY_ALLOC_FAILED;
    }
    memcpy(handle->langtag,pcLang,handle->langtaglen);

    
    /*---------------------------------------*/
    /* Get a the socket to talk to SLPD with */
    /*---------------------------------------*/
    handle->slpdsock = NetworkGetSlpdSocket(&(handle->slpdaddr),
                                            sizeof(handle->slpdaddr));
    
    
    /*---------------------------------------------------------*/
    /* Seed the XID generator if this is the first open handle */
    /*---------------------------------------------------------*/
    if(G_OpenSLPHandleCount == 0)
    {
        XidSeed();
    }
     

    handle->inUse = SLP_FALSE;
    handle->isAsync = isAsync;
    G_OpenSLPHandleCount ++;  
    
    
    *phSLP = (SLPHandle)handle;
    
    
    return SLP_OK;
}
         

/*=========================================================================*/
void SLPClose(SLPHandle hSLP)
/* See header file for detailed documentation                              */
/*=========================================================================*/
{
    PSLPHandleInfo   handle;
    /*------------------------------*/
    /* check for invalid parameters */
    /*------------------------------*/
    if(hSLP == 0)
    {
        return;
    }
    
    handle = (PSLPHandleInfo)hSLP;
    
    if(handle->isAsync)
    {
        /* TODO: stop the usage of this handle (kill threads, etc) */
    }

    G_OpenSLPHandleCount --;
    if(G_OpenSLPHandleCount <= 0)
    {
        NetworkCloseSlpdSocket();
    }

    free(hSLP);
}
