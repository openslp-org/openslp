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
        *(unsigned int*)hSLP != SLP_HANDLE_SIG ||
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

