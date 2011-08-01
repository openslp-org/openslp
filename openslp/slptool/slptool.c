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

/** OpenSLP command line User Agent wrapper.
 *
 * This file contains code that provide command line access to all OpenSLP
 * User Agent library functionality.
 *
 * @file       slptool.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpToolCode
 */

#include "slptool.h"

#ifndef _WIN32
# ifndef HAVE_STRCASECMP
static int strncasecmp(const char * s1, const char * s2, size_t len)
{
   while (len && *s1 && *s2 && ((*s1 ^ *s2) & ~0x20) == 0)
      len--, s1++, s2++;
   return len? (int)(*(unsigned char *)s1 - *(unsigned char *)s2): 0;
}
static int strcasecmp(const char * s1, const char * s2)
{
   while (*s1 && *s2 && ((*s1 ^ *s2) & ~0x20) == 0)
      s1++, s2++;
   return (int)(*(unsigned char *)s1 - *(unsigned char *)s2);
}
# endif
#endif 

static SLPBoolean mySrvTypeCallback(SLPHandle hslp, 
      const char * srvtypes, SLPError errcode, void * cookie) 
{
   char * cpy;
   char * slider1;
   char * slider2;

   (void)hslp;
   (void)cookie;

   if (errcode == SLP_OK && *srvtypes)
   {
      cpy = strdup(srvtypes);
      if (cpy)
      {
         slider1 = slider2 = cpy;
         slider1 = strchr(slider2, ',');
         while (slider1)
         {
            *slider1 = 0;
            printf("%s\n", slider2);
            slider1 ++;
            slider2 = slider1;
            slider1 = strchr(slider2, ',');
         }

         /* print the final itam */
         printf("%s\n", slider2);

         free(cpy);
      }
   }
   return SLP_TRUE;
}

void FindSrvTypes(SLPToolCommandLine * cmdline)
{
   SLPError result;
   SLPHandle hslp;

   if (SLPOpen(cmdline->lang, SLP_FALSE, &hslp) == SLP_OK)
   {
#ifndef UNICAST_NOT_SUPPORTED
      if (cmdline->unicastifc && (result = SLPAssociateIP(hslp, 
            cmdline->unicastifc)) != SLP_OK)
      {
         printf("errorcode: %i\n", result);
         SLPClose(hslp);
         return;
      }
#endif
#ifndef MI_NOT_SUPPORTED
      if (cmdline->interfaces && (result = SLPAssociateIFList(hslp, 
            cmdline->interfaces)) != SLP_OK)
      {
         printf("errorcode: %i\n", result);
         SLPClose(hslp);
         return;
      }

#endif
      if (cmdline->cmdparam1)
         result = SLPFindSrvTypes(hslp, cmdline->cmdparam1, cmdline->scopes,
                        mySrvTypeCallback, 0);
      else
         result = SLPFindSrvTypes(hslp, "*", cmdline->scopes,
                        mySrvTypeCallback, 0);

      if (result != SLP_OK)
         printf("errorcode: %i\n", result);

      SLPClose(hslp);
   }
}

static SLPBoolean myAttrCallback(SLPHandle hslp, 
      const char * attrlist, SLPError errcode, void * cookie)
{
   (void)hslp;
   (void)cookie;

   if (errcode == SLP_OK)
      printf("%s\n", attrlist);

   return SLP_TRUE;
}

void FindAttrs(SLPToolCommandLine * cmdline)
{
   SLPError result;
   SLPHandle hslp;

   if (SLPOpen(cmdline->lang, SLP_FALSE, &hslp) == SLP_OK)
   {
#ifndef UNICAST_NOT_SUPPORTED
      if (cmdline->unicastifc && (result = SLPAssociateIP(hslp, 
            cmdline->unicastifc)) != SLP_OK)
      {
         printf("errorcode: %i\n", result);
         SLPClose(hslp);
         return;
      }
#endif
#ifndef MI_NOT_SUPPORTED 
      if (cmdline->interfaces && (result = SLPAssociateIFList(hslp, 
            cmdline->interfaces)) != SLP_OK)
      {
         printf("errorcode: %i\n", result);
         SLPClose(hslp);
         return;
      }
#endif
      result = SLPFindAttrs(hslp, cmdline->cmdparam1, cmdline->scopes,
                     cmdline->cmdparam2, myAttrCallback, 0);
      if (result != SLP_OK)
         printf("errorcode: %i\n", result);
      SLPClose(hslp);
   }
}

static SLPBoolean mySrvUrlCallback(SLPHandle hslp, const char * srvurl,
      unsigned short lifetime, SLPError errcode, void * cookie) 
{
   (void)hslp;
   (void)cookie;

   if (errcode == SLP_OK)
      printf("%s,%i\n", srvurl, lifetime);

   return SLP_TRUE;
}

void FindSrvs(SLPToolCommandLine * cmdline)
{
   SLPError result;
   SLPHandle hslp;

   if (SLPOpen(cmdline->lang, SLP_FALSE, &hslp) == SLP_OK)
   {
#ifndef UNICAST_NOT_SUPPORTED
      if (cmdline->unicastifc && (result = SLPAssociateIP(hslp, 
            cmdline->unicastifc)) != SLP_OK)
      {
         printf("errorcode: %i\n", result);
         SLPClose(hslp);
         return;
      }
#endif
#ifndef MI_NOT_SUPPORTED
      if (cmdline->interfaces && (result = SLPAssociateIFList(hslp, 
            cmdline->interfaces)) != SLP_OK)
      {
         printf("errorcode: %i\n", result);
         SLPClose(hslp);
         return;
      }
#endif
      result = SLPFindSrvs(hslp, cmdline->cmdparam1, cmdline->scopes,
                     cmdline->cmdparam2, mySrvUrlCallback, 0);
      if (result != SLP_OK)
         printf("errorcode: %i\n", result);
      SLPClose(hslp);
   }
}

void FindScopes(SLPToolCommandLine * cmdline)
{
   SLPError result;
   SLPHandle hslp;
   char * scopes;

   if (SLPOpen(cmdline->lang, SLP_FALSE, &hslp) == SLP_OK)
   {
      result = SLPFindScopes(hslp, &scopes);
      if (result == SLP_OK)
      {
         printf("%s\n", scopes);
         SLPFree(scopes);
      }

      SLPClose(hslp);
   }
}

static void mySLPRegReport(SLPHandle hslp, SLPError errcode, 
      void * cookie)
{
   (void)hslp;
   (void)cookie;

   if (errcode)
      printf("(de)registration errorcode %d\n", errcode);
}

void Register(SLPToolCommandLine * cmdline)
{
   SLPError result;
   SLPHandle hslp;
   char srvtype[80] = "", * s;
   size_t len = 0;
   unsigned int lt = 0;

   if (cmdline->time) {
       lt = atoi(cmdline->time);
   }

   if (strncasecmp(cmdline->cmdparam1, "service:", 8) == 0)
      len = 8;

   s = strchr(cmdline->cmdparam1 + len, ':');
   if (!s)
   {
      printf("Invalid URL: %s\n", cmdline->cmdparam1);
      return;
   }
   len = s - cmdline->cmdparam1;
   strncpy(srvtype, cmdline->cmdparam1, len);
   srvtype[len] = 0;

   /* Clear property (if set), otherwise the register function is quite useless */
   SLPSetProperty("net.slp.watchRegistrationPID", 0);

   if (SLPOpen(cmdline->lang, SLP_FALSE, &hslp) == SLP_OK)
   {
      if (!lt || lt > SLP_LIFETIME_MAXIMUM)
           result = SLPReg(hslp, cmdline->cmdparam1, SLP_LIFETIME_DEFAULT, srvtype,
                           cmdline->cmdparam2, SLP_TRUE, mySLPRegReport, 0);
      else
           result = SLPReg(hslp, cmdline->cmdparam1, (unsigned short)lt, srvtype,
                           cmdline->cmdparam2, SLP_TRUE, mySLPRegReport, 0);
      if (result != SLP_OK)
         printf("errorcode: %i\n", result);
      SLPClose(hslp);
   }
}

void Deregister(SLPToolCommandLine * cmdline)
{
   SLPError result;
   SLPHandle hslp;

   if (SLPOpen(cmdline->lang, SLP_FALSE, &hslp) == SLP_OK)
   {
      result = SLPDereg(hslp, cmdline->cmdparam1, mySLPRegReport, 0);
      if (result != SLP_OK)
         printf("errorcode: %i\n", result);
      SLPClose(hslp);
   }
}

void PrintVersion(SLPToolCommandLine * cmdline)
{
   (void)cmdline;

   printf("slptool version = %s\n", SLP_VERSION);
   printf("libslp version = %s\n", SLPGetProperty("net.slp.OpenSLPVersion"));

   printf("libslp configuration file = %s\n",
         SLPGetProperty("net.slp.OpenSLPConfigFile"));
}

void GetProperty(SLPToolCommandLine * cmdline)
{
   const char * propertyValue;

   propertyValue = SLPGetProperty(cmdline->cmdparam1);
   printf("%s = %s\n", cmdline->cmdparam1,
         propertyValue == 0 ? "" : propertyValue);
}

/* Returns Zero on success. Non-zero on error. */
int ParseCommandLine(int argc, char * argv[], SLPToolCommandLine * cmdline)
{
   int i;

   if (argc < 2)
      return 1; /* not enough arguments */

   for (i = 1; i < argc; i++)
   {
      if (strcasecmp(argv[i], "-v") == 0
            || strcasecmp(argv[i], "--version") == 0)
      {
         if (i < argc)
         {
            cmdline->cmd = PRINT_VERSION;
            return 0;
         }
         else
            return 1;
      }
      else if (strcasecmp(argv[i], "-s") == 0 
            || strcasecmp(argv[i], "--scopes") == 0)
      {
         i++;
         if (i < argc)
            cmdline->scopes = argv[i];
         else
            return 1;
      }
      else if (strcasecmp(argv[i], "-l") == 0
            || strcasecmp(argv[i], "--language") == 0)
      {
         i++;
         if (i < argc)
            cmdline->lang = argv[i];
         else
            return 1;
      }
      else if (strcasecmp(argv[i], "-t") == 0
            || strcasecmp(argv[i], "--time") == 0)
      {
         i++;
         if (i < argc)
            cmdline->time = argv[i];
         else
            return 1;
      }
#ifndef MI_NOT_SUPPORTED
      else if (strcasecmp(argv[i], "-i") == 0
            || strcasecmp(argv[i], "--interfaces") == 0)
      {
         if (cmdline->unicastifc != 0)
         {
            printf("slptool: Can't use -i and -u together.\n");
            return -1;
         }
         i++;
         if (i < argc)
            cmdline->interfaces = argv[i];
         else
            return 1;
      }
#endif
#ifndef UNICAST_NOT_SUPPORTED
      else if (strcasecmp(argv[i], "-u") == 0
            || strcasecmp(argv[i], "--unicastifc") == 0)
      {
         if (cmdline->interfaces != 0)
         {
            printf("slptool: Can't use -i and -u together.\n");
            return -1;
         }
         i++;
         if (i < argc)
            cmdline->unicastifc = argv[i];
         else
            return 1;
      }
#endif
      else if (strcasecmp(argv[i], "findsrvs") == 0)
      {
         cmdline->cmd = FINDSRVS;

         /* service type */
         i++;
         if (i < argc)
            cmdline->cmdparam1 = argv[i];
         else
            return 1;

         /* (optional) filter */
         i++;
         if (i < argc)
            cmdline->cmdparam2 = argv[i];

         break;
      }
      else if (strcasecmp(argv[i], "findattrs") == 0)
      {
         cmdline->cmd = FINDATTRS;     

         /* url or service type */
         i++;
         if (i < argc)
            cmdline->cmdparam1 = argv[i];
         else
            return 1;

         /* (optional) attrids */
         i++;
         if (i < argc)
            cmdline->cmdparam2 = argv[i];
      }
      else if (strcasecmp(argv[i], "findsrvtypes") == 0)
      {
         cmdline->cmd = FINDSRVTYPES;

         /* (optional) naming authority */
         i++;
         if (i < argc)
            cmdline->cmdparam1 = argv[i];
      }
      else if (strcasecmp(argv[i], "findscopes") == 0)
         cmdline->cmd = FINDSCOPES;
      else if (strcasecmp(argv[i], "register") == 0)
      {
            cmdline->cmd = REGISTER;

            /* url */
            i++;
         if (i < argc)
            cmdline->cmdparam1 = argv[i];
         else
            return 1;

            /* Optional attrids */
            i++;
         if (i < argc)
            cmdline->cmdparam2 = argv[i];
         else
            cmdline->cmdparam2 = cmdline->cmdparam1
                  + strlen(cmdline->cmdparam1);

            break;
      }
      else if (strcasecmp(argv[i], "deregister") == 0)
      {
         cmdline->cmd = DEREGISTER;

         /* url */
         i++;
         if (i < argc)
            cmdline->cmdparam1 = argv[i];
         else
            return 1;
      }
      else if (strcasecmp(argv[i], "getproperty") == 0)
      {
         cmdline->cmd = GETPROPERTY;
         i++;
         if (i < argc)
            cmdline->cmdparam1 = argv[i];
         else
            return 1;
      }
      else
         return 1;
   }
   return 0;
}

void DisplayUsage()
{
   printf("Usage: slptool [options] command-and-arguments \n");
   printf("   options may be:\n");
   printf("      -v (or --version) displays the versions of slptool and OpenSLP.\n");
   printf("      -s (or --scope) followed by a comma-separated list of scopes.\n");
   printf("      -l (or --language) followed by a language tag.\n");
   printf("      -t (or --time) followed by a lifetime tag.\n");
#ifndef MI_NOT_SUPPORTED
   printf("      -i (or --interfaces) followed by a comma-separated list of interfaces.\n");
#endif /* MI_NOT_SUPPORTED */
#ifndef UNICAST_NOT_SUPPORTED
   printf("      -u (or --unicastifc) followed by a single interface.\n");
#endif
   printf("\n");
   printf("   command-and-arguments may be:\n");
   printf("      findsrvs service-type [filter]\n");
   printf("      findattrs url [attrids]\n");
   printf("      findsrvtypes [authority]\n");
   printf("      findscopes\n");
   printf("      register url [attrs]\n");
   printf("      deregister url\n");
   printf("      getproperty propertyname\n");
   printf("\n");
   printf("Examples:\n");
   printf("   slptool register service:myserv.x://myhost.com \"(attr1=val1),(attr2=val2)\"\n");
   printf("   slptool findsrvs service:myserv.x\n");
   printf("   slptool findsrvs service:myserv.x \"(attr1=val1)\"\n");
#ifndef MI_NOT_SUPPORTED
   printf("   slptool -i 10.77.13.240,192.168.250.240 findsrvs service:myserv.x\n");
#endif /* MI_NOT_SUPPORTED */
#ifndef UNICAST_NOT_SUPPORTED
   printf("   slptool -u 10.77.13.237 findsrvs service:myserv.x \"(attr1=val1)\"\n");
#endif
   printf("   slptool findattrs service:myserv.x://myhost.com\n");
   printf("   slptool findattrs service:myserv.x://myhost.com attr1\n");
#ifndef MI_NOT_SUPPORTED
   printf("   slptool -i 10.77.13.243 findattrs service:myserv.x://myhost.com attr1\n");
#endif /* MI_NOT_SUPPORTED */
#ifndef UNICAST_NOT_SUPPORTED
   printf("   slptool -u 10.77.13.237 findattrs service:myserv.x://myhost.com attr1 \n");
#endif
   printf("   slptool deregister service:myserv.x://myhost.com\n");
   printf("   slptool getproperty net.slp.useScopes\n");
}

int main(int argc, char * argv[])
{
   int result;
   SLPToolCommandLine cmdline;

   /* zero out the cmdline */
   memset(&cmdline, 0, sizeof(cmdline));

   /* Parse the command line */
   if (ParseCommandLine(argc, argv, &cmdline) == 0)
      switch (cmdline.cmd)
      {
         case FINDSRVS:
            FindSrvs(&cmdline);
            break;

         case FINDATTRS:
            FindAttrs(&cmdline);
            break;

         case FINDSRVTYPES:
            FindSrvTypes(&cmdline);
            break;

         case FINDSCOPES:
            FindScopes(&cmdline);
            break;

         case GETPROPERTY:
            GetProperty(&cmdline);
            break;

         case REGISTER:
            Register(&cmdline);
            break;

         case DEREGISTER:
            Deregister(&cmdline);
            break;

         case PRINT_VERSION:
            PrintVersion(&cmdline);
	    break;

	 case DUMMY:
	    break;
      }
   else
   {
      DisplayUsage();
      result = 1;
   }

   return 0;
}

/*=========================================================================*/ 
