/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_log.h                                                 */
/*                                                                         */
/* Abstract:    slpd logging functions                                     */
/*                                                                         */
/* WARNING:     NOT thread safe!                                           */
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

#ifndef SLPD_LOG_H_INCLUDED
#define SLPD_LOG_H_INCLUDED

#include "slpd.h"

/*=========================================================================*/
/* common code includes                                                    */
/*=========================================================================*/
#include "slp_message.h"
#include "slp_database.h"


/*=========================================================================*/
int SLPDLogFileOpen(const char* path, int append);
/* Prepares the file at the specified path as the log file.                */
/*                                                                         */
/* path     - (IN) the path to the log file. If path is the empty string   */
/*            (""), then we log to stdout.                                 */
/*                                                                         */
/* append   - (IN) if zero log file will be truncated.                     */
/*                                                                         */
/* Returns  - zero on success. errno on failure.                           */
/*=========================================================================*/


/*=========================================================================*/
int SLPDLogFileClose();
/* Releases resources associated with the log file                         */
/*=========================================================================*/


/*=========================================================================*/
void SLPDLogBuffer(const char* prefix, int bufsize, const char* buf);
/* Writes a buffer to the logfile                                          */
/*=========================================================================*/


/*=========================================================================*/
void SLPDLog(const char* msg, ...);
/* Logs a message                                                          */
/*=========================================================================*/


/*=========================================================================*/
void SLPDFatal(const char* msg, ...);
/* Logs a message and halts the process                                    */
/*=========================================================================*/


/*=========================================================================*/
void SLPDLogTime();
/* Logs a timestamp                                                        */
/*=========================================================================*/


/*=========================================================================*/
void SLPDLogMessageInternals(SLPMessage message);
/*=========================================================================*/


/*=========================================================================*/
void SLPDLogMessage(const char* prefix, 
                    struct sockaddr_in* peerinfo,
                    SLPBuffer buf);
/* Log record of receiving or sending an SLP Message.  Logging will only   */
/* occur if message logging is enabled G_SlpProperty.traceMsg != 0         */
/*                                                                         */
/* prefix   (IN) an informative prefix for the log entry                   */
/*                                                                         */
/* peerinfo (IN) the source or destination peer                            */
/*                                                                         */
/* msg      (IN) the message to log                                        */
/*                                                                         */
/* Returns: none                                                           */
/*=========================================================================*/


/*=========================================================================*/
void SLPDLogRegistration(const char* prefix, SLPDatabaseEntry* entry);
/* Log record of having added a registration to the database.  Logging of  */
/* registraions will only occur if registration trace is enabled           */
/* G_SlpProperty.traceReg != 0                                             */
/*                                                                         */
/* prefix   (IN) an informative prefix for the log entry                   */
/*                                                                         */
/* entry    (IN) the database entry that was affected                     */
/*                                                                         */
/* Returns: none                                                           */
/*=========================================================================*/


/*=========================================================================*/
void SLPDLogDAAdvertisement(const char* prefix,
                            SLPDatabaseEntry* entry);
/* Log record of addition or removal of a DA to the store of known DAs.    */
/* Will only occur if DA Advertisment message logging is enabled           */
/* G_SlpProperty.traceDATraffic != 0                                       */
/*                                                                         */
/* prefix   (IN) an informative prefix for the log entry                   */
/*                                                                         */
/* entry    (IN) the database entry that was affected                      */
/*                                                                         */
/* Returns: none                                                           */
/*=========================================================================*/
#endif
