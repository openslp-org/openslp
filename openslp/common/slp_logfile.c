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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#include <slp_logfile.h>

/*=========================================================================*/
static FILE*   G_LogFile    = 0;
/*=========================================================================*/


/********************************************/
/* TODO: Make these functions thread safe!! */
/********************************************/


/*=========================================================================*/
int SLPLogFileOpen(const char* path, int append)                           
/* Prepares the file at the specified path as the log file.                */
/*                                                                         */
/* path     - (IN) the path to the log file.                               */
/*                                                                         */
/* append   - (IN) if zero log file will be truncated.                     */
/*                                                                         */
/* Returns  - zero on success. errno on failure.                           */
/*=========================================================================*/
{
    if(G_LogFile)
    {
        /* logfile was already open close it */
        fclose(G_LogFile);
    }

    if(append)
    {
        G_LogFile = fopen(path,"a");
    }
    else
    {
        G_LogFile = fopen(path,"w");
    }
    
    if(G_LogFile == 0)
    {
        /* could not open the log file */
        return -1;
    }
    
    return 0;
}


/*=========================================================================*/
int SLPLogFileClose()
/* Releases resources associated with the log file                         */
/*=========================================================================*/
{
    fclose(G_LogFile);
    
    return 0;
}


/*=========================================================================*/
void SLPLog(const char* msg, ...)
/* Logs a message                                                          */
/*=========================================================================*/
{
    va_list ap;

    if(G_LogFile)
    {
        va_start(ap,msg);
        vfprintf(G_LogFile,msg,ap); 
        va_end(ap);
        fflush(G_LogFile);
    }
}


/*=========================================================================*/
void SLPFatal(const char* msg, ...)
/* Logs a message and halts the process                                    */
/*=========================================================================*/
{
    va_list ap;
    
    if(G_LogFile)
    {
        fprintf(G_LogFile,"A FATAL Error has occured:\n");
        va_start(ap,msg);
        vfprintf(G_LogFile,msg,ap); 
        va_end(ap);
        fflush(G_LogFile);
    }
    else
    {
        printf("A FATAL Error has occured:\n");
        va_start(ap,msg);
        vprintf(msg,ap);
        va_end(ap);
    }
    
    exit(1);
}

/*=========================================================================*/
void SLPLogBuffer(const char* buf, int bufsize)
/* Writes a buffer to the logfile                                          */
/*=========================================================================*/
{
    fwrite(buf,bufsize,1,G_LogFile);
    fflush(G_LogFile);
}

