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
void SLPDPrintUsage()
/* Displays available command line options of SLPD                         */
/*=========================================================================*/
{

#ifdef WIN32
    printf("USAGE: slpd -install|-remove|-debug [-d] [-c conf file] [-l log file] [-r reg file] [-v version]\n");
#else
    printf("USAGE: slpd [-d] [-c conf file] [-l log file] [-r reg file] [-v version]\n");
#endif

}


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
#ifdef WIN32
    G_SlpdCommandLine.action = -1;
#endif

#if(defined DEBUG)
    G_SlpdCommandLine.detach = 0;
#else
    G_SlpdCommandLine.detach = 1;
#endif

    for (i=1; i<argc; i++)
    {
#ifdef WIN32
        if (strcmp(argv[i],"-install") == 0)
        {
            G_SlpdCommandLine.action = SLPD_INSTALL;
        }
        else if (strcmp(argv[i],"-remove") == 0)
        {
            G_SlpdCommandLine.action = SLPD_REMOVE;
        }
        else if (strcmp(argv[i],"-debug") == 0)
        {
            G_SlpdCommandLine.action = SLPD_DEBUG;
        }
        else
#endif
            if (strcmp(argv[i],"-l") == 0)
        {
            i++;
            if (i >= argc) goto USAGE;
            strncpy(G_SlpdCommandLine.logfile,argv[i],MAX_PATH);
        }
        else if (strcmp(argv[i],"-r") == 0)
        {
            i++;
            if (i >= argc) goto USAGE;
            strncpy(G_SlpdCommandLine.regfile,argv[i],MAX_PATH);
        }
        else if (strcmp(argv[i],"-c") == 0)
        {
            i++;
            if (i >= argc) goto USAGE;
            strncpy(G_SlpdCommandLine.cfgfile,argv[i],MAX_PATH);        
        }
        else if (strcmp(argv[i],"-p") == 0)
        {
            i++;
            if (i >= argc) goto USAGE;
            strncpy(G_SlpdCommandLine.pidfile,argv[i],MAX_PATH);        
        }
        else if (strcmp(argv[i],"-d") == 0)
        {
            G_SlpdCommandLine.detach = 0;
        }
        else if ((strcmp(argv[i], "-v") == 0) 
                 || (strcmp(argv[i], "-V") == 0)
                 || (strcmp(argv[i], "--version") == 0)
                 || (strcmp(argv[i], "-version") == 0))
        {
#ifdef WIN32
            printf("slpd version: %s\n", SLP_VERSION);
#else /* UNIX */
            printf("slpd version: %s\n", VERSION);
#endif

            /* Show options. */
            printf("compile options:\n"
                   "            debugging     "
#ifdef DEBUG
                   "enabled (--enable-debug) "
#else
                   "disabled"
#endif /* NDEBUG */
                   "\n"
                   "           predicates     "
#ifdef USE_PREDICATES
                   "enabled "
#else 
                   "disabled (--disable-predicates) "
#endif /* USE_PREDICATES */
                   "\n"
                   "  slpv1 compatability     "
#ifdef ENABLE_SLPv1
                   "enabled "
#else
                   "disabled (--enable-slpv1=no)"
#endif /* ENABLE_SLPv1 */
                   "\n"
                  );
            exit(1);
        }
        else if ((strcmp(argv[i], "-h") == 0) 
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
    SLPDPrintUsage();
    return 1;
}
