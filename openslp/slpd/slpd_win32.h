/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_win32.h                                               */
/*                                                                         */
/* Abstract:    Win32 specific header file                                 */
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

#ifndef SLPD_WIN32_H_INCLUDED
#define SLPD_WIN32_H_INCLUDED

#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <winsock.h>
#include <process.h>
#include <ctype.h>

#define SLP_VERSION "0.7.5"

/*  internal name of the service  */
#define G_SERVICENAME        "slpd"    

/*  displayed name of the service  */
#define G_SERVICEDISPLAYNAME "Service Location Protocol"

#if(!defined MAX_PATH)
#define MAX_PATH    256
#endif

#define strncasecmp(String1, String2, Num) strnicmp(String1, String2, Num)
#define strcasecmp(String1, String2, Num) stricmp(String1, String2, Num)

typedef DWORD               pid_t;
typedef DWORD               uid_t;
typedef int                 socklen_t;
typedef DWORD               gid_t;
typedef SOCKET              sockfd_t;

/* enum detailing what to do with SLPD when launching it :
   run in the console for debug, install it as a service or uninstall it */
typedef enum _SLPDAction
{
  SLPD_DEBUG   = 0,
  SLPD_INSTALL = 1,
  SLPD_REMOVE  = 2
} SLPDAction;


/*=========================================================================*/
VOID WINAPI SLPDServiceMain(DWORD argc, LPTSTR *argv); 
/* Performs actual initialization of the service                           */
/* This routine performs the service initialization and then calls the     */
/* user defined ServiceStart() routine to perform majority of the work     */
/*                                                                         */
/* argc (IN)   number of command line arguments                            */
/*                                                                         */
/* argv (IN)  array of command line arguments                              */
/*                                                                         */
/*  returns  none                                                          */
/*                                                                         */
/*=========================================================================*/


/*=========================================================================*/
VOID SLPDCmdInstallService(); 
/* Installs the SLPD service in the system                                 */
/*                                                                         */
/* returns  none                                                           */
/*=========================================================================*/


/*=========================================================================*/
VOID SLPDCmdRemoveService(); 
/* Removes the SLPD service from the system                                */
/*                                                                         */
/* returns  none                                                           */
/*=========================================================================*/


/*=========================================================================*/
VOID SLPDCmdDebugService(int argc, char **argv);
/* Runs SLPD in a terminal, to help debugging                              */
/*                                                                         */
/* argc (IN)   number of command line arguments                            */
/*                                                                         */
/* argv (IN)  array of command line arguments                              */
/*                                                                         */
/* returns  none                                                           */
/*=========================================================================*/
#endif /* SLPD_WIN32_H_INCLUDED */


