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

/** Test for SLPFindAttrs.
 *
 * @file       SLPFindAttrs.c
 * @date       Wed May 24 14:26:50 EDT 2000
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    TestCode
 */

#include <slp.h>
#include <slp_debug.h>
#include <stdio.h>
#include <string.h>

SLPBoolean
MySLPSrvURLCallback(SLPHandle hslp,
      const char *srvurl,
      unsigned short lifetime, SLPError errcode, void *cookie)
{
   switch (errcode)
   {
      case SLP_OK:
         printf("Service URL                   = %s\n", srvurl);
         *(SLPError *) cookie = SLP_OK;
         break;
      case SLP_LAST_CALL:
         break;
      default:
         *(SLPError *) cookie = errcode;
         break;
   } /* End switch. */

   return SLP_TRUE;
}
void MySLPRegReport(SLPHandle hslp, SLPError errcode, void* cookie)
{
   /* return the error code in the cookie */
   *(SLPError*)cookie = errcode;
}

SLPBoolean 
MySLPAttrCallback(SLPHandle hslp,
      const char* attrlist, 
      SLPError errcode, 
      void* cookie)
{
   switch (errcode)
   {
      case SLP_OK:
         printf("Service Attributes            = %s\n", attrlist);
         *(SLPError *) cookie = SLP_OK;
         break;
      case SLP_LAST_CALL:
         break;
      default:
         check_error_state(errcode, "Error on Attribute Callback.");
         *(SLPError *) cookie = errcode;
         break;
   } /* End switch. */
   return (1);
}

int
main(int argc, char *argv[])
{
   SLPError err;
   SLPError callbackerr;
   SLPHandle hslp;

   char	server_url[4096];
   char	*attrids;
   char	reg_string[MAX_STRING_LENGTH];

   if ((argc != 4) && (argc != 5))
   {
      printf("SLPFindAttrs\n  This test the SLP Find Attributes.\n Usage:\n   SLPFindAttrs <srv name> <srv adr> <atrb name = value> <atrb query>\n   SLPFindAttrs <srv name> <srv adr> <atrb query>\n");
      return (0);
   }

   err = SLPOpen("en", SLP_FALSE, &hslp);
   check_error_state(err, "Error opening slp handle");

   if (argc == 5)
   {
      sprintf(reg_string, "%s://%s", argv[1], argv[2]);
      printf("Registering                   = %s\n",reg_string);
      /* Register a service with SLP */
      err = SLPReg(hslp,
            reg_string,
            SLP_LIFETIME_MAXIMUM,
            NULL,
            argv[3],
            SLP_TRUE,
            MySLPRegReport,
            &callbackerr);

      check_error_state(err, "Error registering service with slp.");
      check_error_state(callbackerr, "Error registering service with slp.");
   } /* End If. */

   // Check to ensure the service we want to ask about is actually there.
   printf("Querying                      = %s\n",argv[1]);
   err = SLPFindSrvs(
         hslp, 
         argv[1],
         NULL,		/* use configured scopes */
         NULL,		/* no attr filter        */
         MySLPSrvURLCallback,
         &callbackerr);
   check_error_state(err, "Error verifying service with slp.");
   check_error_state(callbackerr, "Error verifying service with slp.");

   // Check to see if the attrivbutes are set correctly
   if (argc == 4)
      attrids = (char *) strdup(argv[3]);
   else
      attrids = (char *) strdup(argv[4]);
   sprintf(server_url, "%s://%s", argv[1], argv[2]);
   printf("Querying Attributes           = %s\n", attrids);
   err = SLPFindAttrs(
         hslp,
         server_url,
         NULL,
         attrids,
         MySLPAttrCallback,
         &callbackerr);
   check_error_state(err, "Error find service attributes.");

   /* Now that we're done using slp, close the slp handle */
   SLPClose(hslp);

   return (0);
}

/*=========================================================================*/
