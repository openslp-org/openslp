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

/** Command line parser.
 *
 * @file       slpd_cmdline.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#include "slpd_cmdline.h"

#ifndef ETCDIR
# define ETCDIR "/etc"
#endif

#ifndef VARDIR
# define VARDIR "/var"
#endif

#ifndef SLP_VERSION
# define SLP_VERSION "unknown"
#endif

#ifdef _WIN32
# define SLPD_USAGE_STRING                                           \
         "-install [auto]|-remove|-start|-stop"                      \
         "|-debug [-d] [-c conf file] [-l log file] [-s spi file] "  \
         "[-r reg file] [-v version]"
#else
# define SLPD_USAGE_STRING                                           \
         "[-d] [-c conf file] [-l log file] "                        \
         "[-r reg file] [-s spi file] [-p pidfile] [-v version]"
#endif

/** Global variable containing command line options.
 */
SLPDCommandLine G_SlpdCommandLine;

/** Displays available command line options of SLPD
 */
void SLPDPrintUsage(void)
{
   fprintf(stderr, "USAGE: slpd " SLPD_USAGE_STRING "\n");
}

/** Must be called to initialize the command line
 *
 * @param[in] argc - The argc as passed to main.
 * @param[in] argv - The argv as passed to main.
 *
 * @return Zero on success, or a non-zero value on usage error.
 */
int SLPDParseCommandLine(int argc, char * argv[])
{
   int i;

   /* Set defaults */
   memset(&G_SlpdCommandLine, 0, sizeof(SLPDCommandLine));
   strnenv(G_SlpdCommandLine.cfgfile, SLPD_CONFFILE, sizeof(G_SlpdCommandLine.cfgfile));
   strnenv(G_SlpdCommandLine.logfile, SLPD_LOGFILE, sizeof(G_SlpdCommandLine.logfile));
   strnenv(G_SlpdCommandLine.regfile, SLPD_REGFILE, sizeof(G_SlpdCommandLine.regfile));
   strnenv(G_SlpdCommandLine.pidfile, SLPD_PIDFILE, sizeof(G_SlpdCommandLine.pidfile));
#ifdef ENABLE_SLPv2_SECURITY
   strnenv(G_SlpdCommandLine.spifile, SLPD_SPIFILE, sizeof(G_SlpdCommandLine.spifile));
#endif
   G_SlpdCommandLine.action = -1;

   G_SlpdCommandLine.detach = 1;

   for (i = 1; i < argc; i++)
   {
#ifdef _WIN32
      if (strcmp(argv[i], "-install") == 0)
      {
         G_SlpdCommandLine.action = SLPD_INSTALL;
         if (i + 1 < argc && strcmp(argv[i + 1], "auto") == 0)
         {
            i++;
            G_SlpdCommandLine.autostart = 1;
         }
      }
      else if (strcmp(argv[i], "-remove") == 0)
      {
         G_SlpdCommandLine.action = SLPD_REMOVE;
      }
	  else if ((strcmp(argv[i], "-debug") == 0) || (strcmp(argv[i], "-d") == 0))
      {
         G_SlpdCommandLine.action = SLPD_DEBUG;
      }
      else if (strcmp(argv[i], "-start") == 0)
      {
         G_SlpdCommandLine.action = SLPD_START;
      }
      else if (strcmp(argv[i], "-stop") == 0)
      {
         G_SlpdCommandLine.action = SLPD_STOP;
      }
      else
#endif
      if (strcmp(argv[i], "-l") == 0)
      {
         i++;
         if (i >= argc) 
            goto USAGE;
         strncpy(G_SlpdCommandLine.logfile,argv[i],MAX_PATH-1);
      }
      else if(strcmp(argv[i],"-r") == 0)
      {
         i++;
         if (i >= argc) 
            goto USAGE;
         strncpy(G_SlpdCommandLine.regfile,argv[i],MAX_PATH-1);
      }
      else if (strcmp(argv[i], "-c") == 0)
      {
         i++;
         if (i >= argc) 
            goto USAGE;
         strncpy(G_SlpdCommandLine.cfgfile, argv[i], MAX_PATH - 1);
      }
#ifdef ENABLE_SLPv2_SECURITY
      else if (strcmp(argv[i], "-s") == 0)
      {
         i++;
         if (i >= argc) 
            goto USAGE;
         strncpy(G_SlpdCommandLine.spifile, argv[i], MAX_PATH - 1);        
      }
#endif
      else if (strcmp(argv[i], "-p") == 0)
      {
         i++;
         if (i >= argc) 
            goto USAGE;
         strncpy(G_SlpdCommandLine.pidfile, argv[i], MAX_PATH - 1);
      }
      else if (strcmp(argv[i], "-d") == 0)
         G_SlpdCommandLine.detach = 0;
      else if (strcmp(argv[i], "-v") == 0 || (strcmp(argv[i], "-V") == 0)
            || (strcmp(argv[i], "--version") == 0)
            || (strcmp(argv[i], "-version") == 0))
      {
         fprintf(stderr, "slpd version: %s\n", SLP_VERSION);

         /* Show options. */
         fprintf(stderr,"compile options:\n"
               "   debugging:            "
#ifdef DEBUG
               "enabled"
#else
               "disabled"
#endif
               "\n"                    
               "   predicates:           "
#ifdef ENABLE_PREDICATES
               "enabled"
#else 
               "disabled"
#endif
               "\n"
               "   slpv1 compatability:  "
#ifdef ENABLE_SLPv1
               "enabled"
#else
               "disabled"
#endif
               "\n"
               "   slpv2 security:       "
#ifdef ENABLE_SLPv2_SECURITY
               "enabled"
#else
               "disabled"
#endif
               "\n"
         );
         exit(1);
      }
      else
         goto USAGE;
   }
   return 0;

USAGE:

   SLPDPrintUsage();
   return 1;
}

/*=========================================================================*/
