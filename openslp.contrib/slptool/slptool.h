/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP command line UA wrapper                            */
/*                                                                         */
/* File:        slptool.h                                                  */
/*                                                                         */
/* Abstract:    Main implementation for slptool                            */
/*                                                                         */
/* Requires:    OpenSLP installation                                       */
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


#include <slp.h>
#include <string.h>


/*=========================================================================*/
typedef enum _SLPToolCommand
/*=========================================================================*/
{
    FINDSRVS = 1,
    FINDATTRS,
    FINDSRVTYPES,
    FINDSCOPES,
    GETPROPERTY
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
}SLPToolCommandLine;


/*=========================================================================*/
void FindSrvs(SLPToolCommandLine* cmdline);
/*=========================================================================*/


/*=========================================================================*/
void FindAttrs(SLPToolCommandLine* cmdline);
/*=========================================================================*/


/*=========================================================================*/            
void FindSrvTypes(SLPToolCommandLine* cmdline);
/*=========================================================================*/
            

/*=========================================================================*/
void GetProperty(SLPToolCommandLine* cmdline);
/*=========================================================================*/

