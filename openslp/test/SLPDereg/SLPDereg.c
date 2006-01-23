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

/** Test for SLPDereg.
 *
 * @file       SLPDereg.c
 * @date       Wed May 24 14:26:50 EDT 2000
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    TestCode
 */

#include <slp.h> 
#include <slp_debug.h>
#include <string.h>

void MySLPRegReport(SLPHandle hslp, SLPError errcode, void * cookie)
{
   (void)hslp;
   *(SLPError *)cookie = errcode; /* return the error code in the cookie */ 
} 

SLPBoolean MySLPSrvURLCallback(SLPHandle hslp, const char * srvurl,
      unsigned short lifetime, SLPError errcode, void * cookie)
{
   (void)hslp;
   (void)lifetime;

   switch (errcode)
   {
      case SLP_OK:
         printf("Service Found   = %s\n", srvurl);
         *(SLPError *)cookie = SLP_OK;
         break;

      case SLP_LAST_CALL:
         break;

      default:
         *(SLPError *)cookie = errcode;
         break;
   }

   /* return SLP_TRUE because we want to be called again
    * if more services were found
    */
   return SLP_TRUE;
}

int main(int argc, char * argv[])
{
   /* This test works by:
    * 1.  Register a service.
    * 2.  Query the service to make sure it is there.
    * 3.  Remove the service.
    * 4.  Query the service to ensure it is not there.
    */
   SLPError err; 
   SLPError callbackerr; 
   SLPHandle hslp; 
   char reg_string[4096];
   char dereg_string[4096];

   if ((argc != 3) && (argc != 5))
   {
      printf("SLPDereg\n  This test the SLP de-registration.\n"
            " Usage:\n   SLPDereg\n     <service name to register>\n"
            "     <service address>\n     <service to deregister>\n"
            "     <service deregistration address>\n   SLPDereg\n"
            "     <service to deregister>\n");
      return 0;
   }

   err = SLPOpen("en", SLP_FALSE, &hslp);
   check_error_state(err, "Error opening slp handle.");

   /* Register a service with SLP */
   if (argc == 5)
   {
      sprintf(reg_string, "%s://%s", argv[1], argv[2]);
      printf("Registering     = %s\n", reg_string);
      err = SLPReg(hslp, reg_string, SLP_LIFETIME_MAXIMUM, "", "", SLP_TRUE,
                  MySLPRegReport, &callbackerr); 
      check_error_state(err, "Error registering service with slp");
      printf("Srv. Registered = %s\n", reg_string);
   }

   /* Now make sure that the service is there. */
   printf("Querying        = %s\n", (argc == 5) ? argv[3] : argv[1]);
   err = SLPFindSrvs(hslp, (argc == 5) ? argv[3] : argv[1], "",
               /* use configured scopes */
         "", /* no attr filter        */
         MySLPSrvURLCallback, &callbackerr);
   check_error_state(err, "Error registering service with slp.");

   /* Deregister the service. */
   if (argc == 5)
      sprintf(dereg_string, "%s://%s", argv[3], argv[4]);
   else
      sprintf(dereg_string, "%s://%s", argv[1], argv[2]);

   printf("Deregistering   = %s\n", dereg_string);
   err = SLPDereg(hslp, dereg_string, MySLPRegReport, &callbackerr);
   check_error_state(err, "Error deregistering service with slp.");
   printf("Deregistered    = %s\n", dereg_string);

   /* Now that we're done using slp, close the slp handle */ 
   SLPClose(hslp); 

   return 0;
}

/*=========================================================================*/ 
