/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP command line UA wrapper                            */
/*                                                                         */
/* File:        slptool.h                                                  */
/*                                                                         */
/* Abstract:    Main header for slptool                                    */
/*                                                                         */
/* Requires:    OpenSLP installation                                       */
/*                                                                         */
/* Author(s):   Matt Peterson <mpeterson@caldera.com>                      */ 
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
/*     Please submit patches to maintainer of http://www.openslp.org       */
/*                                                                         */
/***************************************************************************/


#ifndef _SLPTOOL_H
#define _SLPTOOL_H

#include <slp.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*=========================================================================*/
typedef enum _SLPToolCommand
/*=========================================================================*/
{
    FINDSRVS = 1,
    FINDATTRS,
    FINDSRVTYPES,
    FINDSCOPES,
    GETPROPERTY,
    REGISTER,
    DEREGISTER,
    PRINT_VERSION,
#ifndef MI_NOT_SUPPORTED
    FINDSRVSUSINGIFLIST,
    FINDATTRSUSINGIFLIST,
    FINDSRVTYPESUSINGIFLIST,
#endif /* MI_NOT_SUPPORTED */
#ifndef UNICAST_NOT_SUPPORTED
    UNICASTFINDSRVS,
    UNICASTFINDATTRS,
    UNICASTFINDSRVTYPES,
#endif
    DUMMY
}SLPToolCommand;



/*=========================================================================*/
typedef struct _SLPToolCommandLine
/*=========================================================================*/
{
    SLPToolCommand  cmd;
    const char*     lang;
    const char*     scopes;
    const char*     cmdparam1;
    const char*     cmdparam2;
    const char*     cmdparam3;
}SLPToolCommandLine;


/*=========================================================================*/
void FindSrvs(SLPToolCommandLine* cmdline);
/*=========================================================================*/


#ifndef MI_NOT_SUPPORTED
/*=========================================================================*/
void FindSrvsUsingIFList(SLPToolCommandLine* cmdline);
/*=========================================================================*/
#endif


#ifndef UNICAST_NOT_SUPPORTED
/*=========================================================================*/
void UnicastFindSrvs(SLPToolCommandLine* cmdline);
/*=========================================================================*/
#endif


/*=========================================================================*/
void FindAttrs(SLPToolCommandLine* cmdline);
/*=========================================================================*/


#ifndef UNICAST_NOT_SUPPORTED
/*=========================================================================*/
void UnicastFindAttrs(SLPToolCommandLine* cmdline);
/*=========================================================================*/
#endif


#ifndef MI_NOT_SUPPORTED
/*=========================================================================*/
void FindAttrsUsingIFList(SLPToolCommandLine* cmdline);
/*=========================================================================*/
#endif


/*=========================================================================*/            
void FindSrvTypes(SLPToolCommandLine* cmdline);
/*=========================================================================*/


#ifndef UNICAST_NOT_SUPPORTED
/*=========================================================================*/
void UnicastFindSrvTypes(SLPToolCommandLine* cmdline);
/*=========================================================================*/
#endif


#ifndef MI_NOT_SUPPORTED
/*=========================================================================*/
void FindSrvTypesUsingIFList(SLPToolCommandLine* cmdline);
/*=========================================================================*/
#endif


/*=========================================================================*/
void GetProperty(SLPToolCommandLine* cmdline);
/*=========================================================================*/


/*=========================================================================*/
void Register(SLPToolCommandLine* cmdline);
/*=========================================================================*/


/*=========================================================================*/
void Deregister(SLPToolCommandLine* cmdline);
/*=========================================================================*/

#endif
