/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slplib_parse.c                                             */
/*                                                                         */
/* Abstract:    Implementation for SLPParseSrvUrl(), SLPEscape(),          */
/*              SLPUnescape() and SLPFree() calls.                         */
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
void SLPFree(void* pvMem)                                                  
/*                                                                         */
/* Frees memory returned from SLPParseSrvURL(), SLPFindScopes(),           */
/* SLPEscape(), and SLPUnescape().                                         */
/*                                                                         */
/* pvMem    A pointer to the storage allocated by the SLPParseSrvURL(),    */
/*          SLPEscape(), SLPUnescape(), or SLPFindScopes() function.       */
/*          Ignored if NULL.                                               */
/*=========================================================================*/
{
    if(pvMem)
    {
        free(pvMem);
    }
}


/*=========================================================================*/
SLPError SLPParseSrvURL(const char *pcSrvURL,
                        SLPSrvURL** ppSrvURL)
/*                                                                         */
/* Parses the URL passed in as the argument into a service URL structure   */
/* and returns it in the ppSrvURL pointer.  If a parse error occurs,       */
/* returns SLP_PARSE_ERROR. The input buffer pcSrvURL is destructively     */
/* modified during the parse and used to fill in the fields of the         */
/* return structure.  The structure returned in ppSrvURL should be freed   */
/* with SLPFreeURL().  If the URL has no service part, the s_pcSrvPart     */
/* string is the empty string, "", i.e.  not NULL. If pcSrvURL is not a    */
/* service:  URL, then the s_pcSrvType field in the returned data          */
/* structure is the URL's scheme, which might not be the same as the       */
/* service type under which the URL was registered.  If the transport is   */
/* IP, the s_pcTransport field is the empty string.  If the transport is   */
/* not IP or there is no port number, the s_iPort field is zero.           */
/*                                                                         */
/* pcSrvURL A pointer to a character buffer containing the null terminated */
/*          URL string to parse.                                           */
/*                                                                         */
/* ppSrvURL A pointer to a pointer for the SLPSrvURL structure to receive  */
/*          the parsed URL. The memory should be freed by a call to        */
/*          SLPFree() when no longer needed.                               */
/*                                                                         */
/* Returns: If no error occurs, the return value is SLP_OK. Otherwise, the */
/*          appropriate error code is returned.                            */
/*=========================================================================*/
{
    char*   slider1; /* points to location in the SLPSrvURL buffer */
    char*   slider2;
    char*   slider3;

    *ppSrvURL = (SLPSrvURL*)malloc(strlen(pcSrvURL) + sizeof(SLPSrvURL) + 4);
    /* +4 ensures space for 4 null terminations */
    if(*ppSrvURL == 0)
    {
        return SLP_MEMORY_ALLOC_FAILED;
    }
    memset(*ppSrvURL,0,sizeof(SLPSrvURL));
    
    slider1 = ((char*)*ppSrvURL) + sizeof(SLPSrvURL);
    slider2 = slider3 = (char*)pcSrvURL;

    /* parse out the service type */
    slider3 = (char*)strstr(slider2,"://");
    if(slider3 == 0)
    {
        free(*ppSrvURL);
        *ppSrvURL = 0;
        return SLP_PARSE_ERROR;
    }
    /* ensure that URL is of the service: scheme */
    if(strstr(slider2,"service:") == 0)
    {
        free(*ppSrvURL);
        *ppSrvURL = 0;
        return SLP_PARSE_ERROR;
    }
    memcpy(slider1,slider2,slider3-slider2);
    (*ppSrvURL)->s_pcSrvType = slider1;
    slider1 = slider1 + (slider3 - slider2);
    *slider1 = 0;  /* null terminate */
    slider1 = slider1 + 1;

    /* parse out the host */
    slider3 = slider2 = slider3 + 3; /* + 3 skips the "://" */
    while(*slider3 && *slider3 != '/' && *slider3 != ':') slider3++;
    if(slider3-slider2 < 1)
    {
        /* no host part */
        free(*ppSrvURL);
        *ppSrvURL = 0;
        return SLP_PARSE_ERROR;
    }
    memcpy(slider1,slider2,slider3-slider2);
    (*ppSrvURL)->s_pcHost = slider1;
    slider1 = slider1 + (slider3 - slider2);
    *slider1 = 0;  /* null terminate */
    slider1 = slider1 + 1;

    /* parse out the port */
    if(*slider3 == ':')
    {
        slider3 = slider2 = slider3 + 1; /* + 3 skips the ":" */
        while(*slider3 && *slider3 != '/') slider3++;
        if(slider3)
        {
            *slider3 = 0;
            (*ppSrvURL)->s_iPort = atoi(slider2);
            *slider3 = '/';
        }
        else
        {
            (*ppSrvURL)->s_iPort = atoi(slider2);
        }
        slider1 = slider1 + sizeof(int);   
    }

    /* parse out the remainder of the url */
    if(*slider3)
    {
        slider3 = slider2 = slider3; 
        while(*slider3) slider3 ++;
        memcpy(slider1,slider2,slider3-slider2);
        (*ppSrvURL)->s_pcSrvPart = slider1;
        slider1 = slider1 + (slider3 - slider2);
        *slider1 = 0;  /* null terminate */
        slider1 = slider1 + 1;
    }
    else
    {
        /* no remainder portion */
        (*ppSrvURL)->s_pcSrvPart = slider1;
    }

    /* set  the net family to always be an empty string for IP */
    *slider1 = 0;
    (*ppSrvURL)->s_pcNetFamily = slider1;

    
    return SLP_OK;
}


/*=========================================================================*/
SLPError SLPEscape(const char* pcInbuf,
                   char** ppcOutBuf,
                   SLPBoolean isTag)
/*                                                                         */
/* Process the input string in pcInbuf and escape any SLP reserved         */
/* characters.  If the isTag parameter is SLPTrue, then look for bad tag   */
/* characters and signal an error if any are found by returning the        */
/* SLP_PARSE_ERROR code.  The results are put into a buffer allocated by   */
/* the API library and returned in the ppcOutBuf parameter.  This buffer   */
/* should be deallocated using SLPFree() when the memory is no longer      */
/* needed.                                                                 */
/*                                                                         */
/* pcInbuf      Pointer to he input buffer to process for escape           */
/*              characters.                                                */
/*                                                                         */
/* ppcOutBuf    Pointer to a pointer for the output buffer with the SLP    */
/*              reserved characters escaped.  Must be freed using          */
/*              SLPFree()when the memory is no longer needed.              */ 
/*                                                                         */
/* isTag        When true, the input buffer is checked for bad tag         */
/*              characters.                                                */
/*                                                                         */
/* Returns:     Return SLP_PARSE_ERROR if any characters are bad tag       */
/*              characters and the isTag flag is true, otherwise SLP_OK,   */
/*              or the appropriate error code if another error occurs.     */
/*=========================================================================*/
{
    return SLP_NOT_IMPLEMENTED;
}


/*=========================================================================*/
SLPError SLPUnescape(const char* pcInbuf,
                     char** ppcOutBuf,
                     SLPBoolean isTag)
/*                                                                         */
/* Process the input string in pcInbuf and unescape any SLP reserved       */
/* characters.  If the isTag parameter is SLPTrue, then look for bad tag   */
/* characters and signal an error if any are found with the                */
/* SLP_PARSE_ERROR code.  No transformation is performed if the input      */
/* string is an opaque.  The results are put into a buffer allocated by    */
/* the API library and returned in the ppcOutBuf parameter.  This buffer   */
/* should be deallocated using SLPFree() when the memory is no longer      */
/* needed.                                                                 */
/*                                                                         */
/* pcInbuf      Pointer to he input buffer to process for escape           */
/*              characters.                                                */
/*                                                                         */
/* ppcOutBuf    Pointer to a pointer for the output buffer with the SLP    */
/*              reserved characters escaped.  Must be freed using          */
/*              SLPFree() when the memory is no longer needed.             */
/*                                                                         */
/* isTag        When true, the input buffer is checked for bad tag         */
/*              characters.                                                */
/*                                                                         */
/* Returns:     Return SLP_PARSE_ERROR if any characters are bad tag       */
/*              characters and the isTag flag is true, otherwise SLP_OK,   */
/*              or the appropriate error code if another error occurs.     */
/*=========================================================================*/
{
    return SLP_NOT_IMPLEMENTED;
}
