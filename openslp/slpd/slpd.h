/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd.h                                                     */
/*                                                                         */
/* Abstract:    Makes all declarations used by slpd. Included by all slpd  */
/*              source files                                               */
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

#ifndef SLPD_H_INCLUDED
#define SLPD_H_INCLUDED

/*=========================================================================*/
/* Include platform specific headers files                                 */
/*=========================================================================*/
#ifdef WIN32
#include "slpd_win32.h"
#else
#include "slpd_unistd.h"
#endif


/*=========================================================================*/
/* Misc "adjustable" constants (I would not adjust the if I were you)      */
/*=========================================================================*/
#define SLPD_CONFIG_MAX_RECONN      1    /* max number tcp of reconnects   */
#define SLPD_MAX_SOCKETS            128  /* maximum number of sockets      */
#define SLPD_COMFORT_SOCKETS        64   /* a comfortable number of socket */
#define SLPD_CONFIG_CLOSE_CONN      900  /* max idle time (60 min)         */
#define SLPD_AGE_INTERVAL           15   /* age every 15 seconds           */
#define SLPD_ATTR_RECURSION_DEPTH   50   /* max recursion depth for attr   */
                                         /* parser                         */


/*=========================================================================*/
/* Global variables representing signals */
/*=========================================================================*/
extern int G_SIGALRM;
extern int G_SIGTERM;
extern int G_SIGHUP;
extern int G_SIGINT;


#endif /*(!defined SLPD_H_INCLUDED) */
