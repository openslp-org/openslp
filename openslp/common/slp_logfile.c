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
static char*   G_LogFilePath    = 0;
static int     G_LogLevel       = 2;
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
    FILE*       stream;
    int         result = 0;
    
    #if(defined DEBUG)
    if(path == 0)
    {
        SLPFatal("SLPOpenLogFile() was passed a null pointer");
    }
    #endif
    
    if(G_LogFilePath == 0)
    {
        if(append)
        {
            stream = fopen(path,"a");
        }
        else
        {
            stream = fopen(path,"w");
        }
    
        if(stream != 0)
        {
           G_LogFilePath = (char*)realloc((void*)G_LogFilePath,strlen(path) + 1);
           if(G_LogFilePath)
           {
                strcpy(G_LogFilePath,path);
           }
           else
           {
               result = ENOMEM;
           }   
           
           fclose(stream);
        }                              
        else
        {
            result = EINVAL;
        }
    }
    
    return result;                     
}


/*=========================================================================*/
int SLPLogFileClose()
/* Releases resources associated with the log file                         */
/*=========================================================================*/
{
    if(G_LogFilePath)
    {
        free(G_LogFilePath);
        G_LogFilePath = 0;
    }

    return 0;
}


/*=========================================================================*/
void SLPLogSetLevel(int level)
/*=========================================================================*/
{
    G_LogLevel = level;
}
                  

/*=========================================================================*/
void SLPFatal(const char* msg, ...)
/* Logs a message and halts the process                                    */
/*=========================================================================*/
{
    FILE*   stream;
    va_list ap;
    
    if(G_LogLevel < 1) return;

    if(G_LogFilePath)
    {
        stream = fopen(G_LogFilePath,"a");
        if(stream)
        {
            fprintf(stream,"FATAL: ");
            va_start(ap,msg);
            vfprintf(stream,msg,ap);
            va_end(ap);
            fclose(stream);   
        }
    }
    else
    {
        printf("FATAL: ");
        va_start(ap,msg);
        vprintf(msg,ap);
        va_end(ap);
    }
    
    exit(1);
}


/*=========================================================================*/
void SLPError(const char* msg, ...)
/* Logs an error message                                                   */
/*=========================================================================*/
{
    FILE*   stream;
    va_list ap;
    
    
    if(G_LogLevel < 2) return;

    if(G_LogFilePath)
    {
        stream = fopen(G_LogFilePath,"a");
        if(stream)
        {
            fprintf(stream,"ERROR: ");
            va_start(ap,msg);
            vfprintf(stream,msg,ap);
            va_end(ap);
            fclose(stream);
        }
    }
    else
    {
        printf("ERROR: ");
        va_start(ap,msg);
        vprintf(msg,ap);
        va_end(ap);
    }   
}


/*=========================================================================*/
void SLPLog(const char* msg, ...)
/* Logs a message                                                          */
/*=========================================================================*/
{
    FILE*   stream;
    va_list ap;
    
    if(G_LogLevel < 3) return;

    if(G_LogFilePath)
    {
        stream = fopen(G_LogFilePath,"a");
        if(stream)
        {
            fprintf(stream,"MSG: ");
            va_start(ap,msg);
            vfprintf(stream,msg,ap);
            va_end(ap);
            fclose(stream);
        }
    }
    else
    {
        printf("MSG: ");
        va_start(ap,msg);
        vprintf(msg,ap);
        va_end(ap);
    }   
    
}

/*=========================================================================*/
void SLPDebug(const char* msg, ...)
/* Logs a debug message                                                    */
/*=========================================================================*/
{
    FILE*   stream;
    va_list ap;
    
    if(G_LogLevel < 4) return;

    if(G_LogFilePath)
    {
        stream = fopen(G_LogFilePath,"a");
        if(stream)
        {
            fprintf(stream,"DEBUG: ");
            va_start(ap,msg);
            vfprintf(stream,msg,ap);
            va_end(ap);
            fclose(stream);
        }
    }
    else
    {
        printf("DEBUG: ");
        va_start(ap,msg);
        vprintf(msg,ap);
        va_end(ap);
    }
}

