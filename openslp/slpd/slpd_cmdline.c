/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_cmdline.c                                             */
/*                                                                         */
/* Abstract:    Simple command line processor                              */
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

#include "slpd.h"

/*=========================================================================*/
SLPDCommandLine G_SlpdCommandLine;
/* Global variable containing command line options                         */
/*=========================================================================*/


/*=========================================================================*/
int SLPDParseCommandLine(int argc,char* argv[])
/* Must be called to initialize the command line                           */
/*                                                                         */
/* argc (IN) the argc as passed to main()                                  */
/*                                                                         */
/* argv (IN) the argv as passed to main()                                  */
/*                                                                         */
/* Returns  - zero on success.  non-zero on usage error                    */
/*=========================================================================*/
{
    int i;

    /* Set defaults */
    memset(&G_SlpdCommandLine,0,sizeof(SLPDCommandLine));
    strcpy(G_SlpdCommandLine.cfgfile,"/etc/slp.conf");
    strcpy(G_SlpdCommandLine.logfile,"/var/log/slpd.log");
    strcpy(G_SlpdCommandLine.regfile,"/etc/slp.reg");
    strcpy(G_SlpdCommandLine.pidfile,"/var/run/slpd.pid");
    #if(defined DEBUG)
    G_SlpdCommandLine.detach = 0;
    #else
    G_SlpdCommandLine.detach = 1;
    #endif
     
    for(i=1; i<argc; i++)
    {
        if(strcmp(argv[i],"-l") == 0)
        {
            i++;
            if(i >= argc) goto USAGE;
            strncpy(G_SlpdCommandLine.logfile,argv[i],MAX_PATH);
        }
        else if(strcmp(argv[i],"-r") == 0)
        {
            i++;
            if(i >= argc) goto USAGE;
            strncpy(G_SlpdCommandLine.regfile,argv[i],MAX_PATH);
        }
        else if(strcmp(argv[i],"-c") == 0)
        {
            i++;
            if(i >= argc) goto USAGE;
            strncpy(G_SlpdCommandLine.cfgfile,argv[i],MAX_PATH);        
        }
        else if(strcmp(argv[i],"-p") == 0)
        {
            i++;
            if(i >= argc) goto USAGE;
            strncpy(G_SlpdCommandLine.pidfile,argv[i],MAX_PATH);        
        }
        else if(strcmp(argv[i],"-d") == 0)
        {
            G_SlpdCommandLine.detach = 0;
        }
		else if((strcmp(argv[i], "-v") == 0) 
				|| (strcmp(argv[i], "-V") == 0)
				|| (strcmp(argv[i], "--version") == 0)
			   	|| (strcmp(argv[i], "-version") == 0))
		{
			printf("slpd version: %s\n", VERSION);
			exit(1);
		}
		else if((strcmp(argv[i], "-h") == 0) 
			   	|| (strcmp(argv[i], "-help") == 0)
			   	|| (strcmp(argv[i], "--help") == 0))
		{
    		printf("USAGE: slpd [-d] [-c conf file] [-l log file] [-r reg file] [-v version]\n");
			exit(1);
		}
        else
        {
            goto USAGE;
        }
    }

    return 0;

    USAGE:
    printf("USAGE: slpd [-d] [-c conf file] [-l log file] [-r reg file] [-v version]\n");
    
    return 1;
}
