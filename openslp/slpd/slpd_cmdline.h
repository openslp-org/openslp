/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_cmdline.h                                             */
/*                                                                         */
/* Abstract:    Simple command line processor                              */
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

#ifndef SLPD_CMDLINE_H_INCLUDED
#define SLPD_CMDLINE_H_INCLUDED

#include "slpd.h"

/*=========================================================================*/
/* Filename constants                                                      */
/*=========================================================================*/
#ifndef __WIN32__
#define SLPD_CONFFILE ETCDIR "/slp.conf"
#define SLPD_LOGFILE  VARDIR "/log/slpd.log"
#define SLPD_REGFILE  ETCDIR "/slp.reg"
#define SLPD_PIDFILE  VARDIR "/run/slpd.pid"
#define SLPD_SPIFILE  ETCDIR "/slp.spi"
#else
#define SLPD_CONFFILE "%WINDIR%\\slp.conf"
#define SLPD_LOGFILE  "%WINDIR%\\slpd.log"
#define SLPD_REGFILE  "%WINDIR%\\slp.reg"
#define SLPD_PIDFILE  "%WINDIR%\\slpd.pid"
#define SLPD_SPIFILE  "%WINDIR%\\slp.spi"
#endif


/*=========================================================================*/
/* Make sure MAX_PATH is defined                                           */
/*=========================================================================*/
#if(!defined MAX_PATH)
#define MAX_PATH                    256
#endif


/*=========================================================================*/
typedef struct _SLPDCommandLine
/* Holds  values of parameters read from the command line                  */
/*=========================================================================*/
{
    char   cfgfile[MAX_PATH];
    char   regfile[MAX_PATH];
    char   logfile[MAX_PATH];
    char   pidfile[MAX_PATH];
#ifdef ENABLE_SLPv2_SECURITY
    char   spifile[MAX_PATH];
#endif
    int    action;
    int    detach;
}SLPDCommandLine;


/*=========================================================================*/
extern SLPDCommandLine G_SlpdCommandLine;
/* Global variable containing command line options                         */
/*=========================================================================*/


/*=========================================================================*/
void SLPDPrintUsage();
/* Displays available command line options of SLPD                         */
/*=========================================================================*/


/*=========================================================================*/
int SLPDParseCommandLine(int argc,char* argv[]);
/* Must be called to initialize the command line                           */
/*                                                                         */
/* argc (IN) the argc as passed to main()                                  */
/*                                                                         */
/* argv (IN) the argv as passed to main()                                  */
/*                                                                         */
/* Returns  - zero on success.  non-zero on usage error                    */
/*=========================================================================*/

#endif
