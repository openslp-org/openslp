/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slplib_findattrs.c                                         */
/*                                                                         */
/* Abstract:    Implementation for SLPFindScopes() call.                   */
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
SLPError SLPFindScopes(SLPHandle hSLP,
                       char** ppcScopeList)
/*                                                                         */
/* Sets ppcScopeList parameter to a pointer to a comma separated list      */
/* including all available scope values.  The list of scopes comes from    */
/* a variety of sources:  the configuration file's net.slp.useScopes       */
/* property, unicast to DAs on the net.slp.DAAddresses property, DHCP,     */
/* or through the DA discovery process.  If there is any order to the      */
/*  scopes, preferred scopes are listed before less desirable scopes.      */
/* There is always at least one name in the list, the default scope,       */
/* "DEFAULT".                                                              */
/*                                                                         */
/* hSLP         The SLPHandle on which to search for scopes.               */
/*                                                                         */
/* ppcScopeList A pointer to char pointer into which the buffer pointer is */
/*              placed upon return.  The buffer is null terminated.  The   */
/*              memory should be freed by calling SLPFree().               */
/*                                                                         */
/* Returns:     If no error occurs, returns SLP_OK, otherwise, the a       */
/*              ppropriate error code.                                     */
/*=========================================================================*/
{
    int scopelistlen;

    /*------------------------------*/
    /* check for invalid parameters */
    /*------------------------------*/
    if( hSLP == 0 ||
        *(unsigned long*)hSLP != SLP_HANDLE_SIG ||
        ppcScopeList  == 0 )
    {
        return SLP_PARAMETER_BAD;
    }

    /* start with nothing */
    *ppcScopeList = 0;

    if(KnownDAGetScopes(&scopelistlen,ppcScopeList))
    {
        return SLP_MEMORY_ALLOC_FAILED;
    }

    return SLP_OK;

}


/*=========================================================================*/
unsigned short SLPGetRefreshInterval()
/*                                                                         */
/* Returns the maximum across all DAs of the min-refresh-interval          */
/* attribute.  This value satisfies the advertised refresh interval        */
/* bounds for all DAs, and, if used by the SA, assures that no refresh     */
/* registration will be rejected.  If no DA advertises a min-refresh-      */
/* interval attribute, a value of 0 is returned.                           */
/*                                                                         */
/* Returns: If no error, the maximum refresh interval value allowed by all */
/*          DAs (a positive integer).  If no DA advertises a               */
/*          min-refresh-interval attribute, returns 0.  If an error occurs,*/ 
/*          returns an SLP error code.                                     */
/*=========================================================================*/
{
    return 0;
}

