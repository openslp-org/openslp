/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slplib_property.c                                          */
/*                                                                         */
/* Abstract:    Implementation for SLPGetProperty() and SLPSetProperty()   */
/*              calls.                                                     */
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
int G_PropertyInit  = 0;
/*=========================================================================*/


/*=========================================================================*/
const char* SLPGetProperty(const char* pcName)
/*                                                                         */
/* Returns the value of the corresponding SLP property name.  The returned */
/* string is owned by the library and MUST NOT be freed.                   */
/*                                                                         */
/* pcName   Null terminated string with the property name, from            */
/*          Section 2.1 of RFC 2614.                                       */
/*                                                                         */
/* Returns: If no error, returns a pointer to a character buffer containing*/ 
/*          the property value.  If the property was not set, returns the  */
/*          default value.  If an error occurs, returns NULL. The returned */
/*          string MUST NOT be freed.                                      */
/*=========================================================================*/
{
    if(G_PropertyInit == 0)
    {
        if(SLPPropertyReadFile(LIBSLP_CONFFILE) == 0)
        {
            G_PropertyInit = 1;
        }
        else
        {
            return 0;
        }
    }

    return SLPPropertyGet(pcName);
}



/*=========================================================================*/
void SLPSetProperty(const char *pcName,
                    const char *pcValue)
/*                                                                         */
/* Sets the value of the SLP property to the new value.  The pcValue       */
/* parameter should be the property value as a string.                     */
/*                                                                         */
/* pcName   Null terminated string with the property name, from Section    */
/*          2.1. of RFC 2614.                                              */
/*                                                                         */
/* pcValue  Null terminated string with the property value, in UTF-8       */
/*          character encoding.                                            */
/*=========================================================================*/
{
    /* Following commented out for threading reasons 
    
    if(G_PropertyInit == 0)
    {
        if(SLPPropertyReadFile(LIBSLP_CONFFILE) == 0)
        {
            G_PropertyInit = 1;
        }
    }

    SLPPropertySet(pcName,pcValue);
    
    */
}

