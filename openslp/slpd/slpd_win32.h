/*-------------------------------------------------------------------------
 * Copyright (C) 2000 Caldera Systems, Inc
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    Neither the name of Caldera Systems nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CALDERA
 * SYSTEMS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *-------------------------------------------------------------------------*/

/** Header file for win32 service main.
 *
 * @file       slpd_win32.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#ifndef SLPD_WIN32_H_INCLUDED
#define SLPD_WIN32_H_INCLUDED

/*!@defgroup SlpdCodeSvcMainFile Win32 Service Main */

/*!@addtogroup SlpdCodeSvcMainFile
 * @ingroup SlpdCode
 * @{
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h> 
#include <stdio.h>
#include <process.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h> 
#include <limits.h>

#define G_SERVICENAME "slpd"
/*!< Internal name of the service.
 */

#define G_SERVICEDISPLAYNAME "Service Location Protocol"
/*!< Displayed name of the service.
 */

#ifndef MAX_PATH
# define MAX_PATH    256
#endif

typedef DWORD pid_t;
typedef DWORD uid_t;
typedef int socklen_t;
typedef DWORD gid_t;
typedef SOCKET sockfd_t;

/** enum detailing what to do with SLPD when launching it
 * Run in the console for debug, install it as a service or uninstall it. 
 */
typedef enum SLPDAction
{
   SLPD_DEBUG = 0,
   SLPD_INSTALL = 1,
   SLPD_REMOVE = 2,
   SLPD_START = 3,
   SLPD_STOP = 4
} SLPDAction;

#define inet_aton(opt,bind) ((bind)->s_addr = inet_addr(opt))
/*!< Definition for inet_aton() since Microsoft does not have this yet.
 */

VOID WINAPI SLPDServiceMain(DWORD argc, LPTSTR * argv);

VOID SLPDCmdInstallService(int automatic);

VOID SLPDCmdRemoveService(void);

VOID SLPDCmdDebugService(int argc, char **argv);

/*! @} */

#endif /* SLPD_WIN32_H_INCLUDED */

/*=========================================================================*/

