/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slp_logfile.h                                              */
/*                                                                         */
/* Abstract:    Header file that defines structures and constants that are */
/*              specific to the SLP log file.                              */
/*                                                                         */
/* WARNING:     NOT thread safe!                                           */
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

#if(!defined SLP_LOGFILE_H_INCLUDED)
#define SLP_LOGFILE_H_INCLUDED

/*=========================================================================*/
int SLPLogFileOpen(const char* path, int append);                          
/* Prepares the file at the specified path as the log file.                */
/*                                                                         */
/* path     - (IN) the path to the log file.                               */
/*                                                                         */
/* append   - (IN) if zero log file will be truncated.                     */
/*                                                                         */
/* Returns  - zero on success. errno on failure                            */
/*=========================================================================*/


/*=========================================================================*/
void SLPLogSetLevel(int level);
/*=========================================================================*/


/*=========================================================================*/
int SLPLogFileClose();
/* Releases resources associated with the log file                         */
/*=========================================================================*/


/*=========================================================================*/
void SLPFatal(const char* msg, ...);
/* Logs a message and halts the process                                    */
/*=========================================================================*/


/*=========================================================================*/
void SLPError(const char* msg, ...);
/* Logs an error message                                                   */
/*=========================================================================*/


/*=========================================================================*/
void SLPLog(const char* msg, ...);
/* Logs a message                                                          */
/*=========================================================================*/

/*=========================================================================*/
void SLPDebug(const char* msg, ...);
/* Logs a debug message                                                    */
/*=========================================================================*/

#endif


