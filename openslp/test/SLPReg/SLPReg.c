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

/** Test for SLPReg.
 *
 * @file       SLPReg.c
 * @date       Wed May 24 14:26:50 EDT 2000
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    TestCode
 */

#include <slp.h>
#include <slp_debug.h>
#include <string.h>

void MySLPRegReport(SLPHandle hslp, SLPError errcode, void* cookie) 
{ 
   (void)hslp;
   /* return the error code in the cookie */ 
   *(SLPError*)cookie = errcode; 
} 

SLPBoolean
MySLPSrvURLCallback (SLPHandle hslp,
           const char *srvurl,
           unsigned short lifetime, SLPError errcode, void *cookie)
{
   (void)hslp;
   (void)lifetime;
   switch(errcode) {
      case SLP_OK:
         printf ("Service Found   = %s\n", srvurl);
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

int main(int argc, char* argv[]) 
{ 
   SLPError err; 
   SLPError callbackerr; 
   SLPHandle   hslp; 
   char     reg_string[4096];

   if ((argc < 2) || (argc > 4))
   {
      printf("SLPReg\n  This test the SLP registration.\n Usage:\n   SLPReg <service name to create> <service host address> <service to search>\n");
      return(0);
   }
   err = SLPOpen("en",SLP_FALSE,&hslp);
   check_error_state(err, "Error opening slp handle");
   sprintf(reg_string,"%s://%s",argv[1], argv[2]);

   /* Register a service with SLP */ 
   printf("Registering     = %s\n",reg_string);
   err = SLPReg( hslp, 
      reg_string,
      SLP_LIFETIME_MAXIMUM, 
      0, 
      "(public-key=......my_pgp_key.......)", 
      SLP_TRUE, 
      MySLPRegReport, 
      &callbackerr ); 
   check_error_state(err, "Error registering service with slp.");
   check_error_state(callbackerr, "Error registering service with slp.");

   printf("Querying        = %s\n",argv[3]);
   /* Now make sure that the service is there. */
   err = SLPFindSrvs (
         hslp, 
         argv[3],
         0,    /* use configured scopes */
         0,    /* no attr filter        */
         MySLPSrvURLCallback,
         &callbackerr);

   /* err may contain an error code that occured as the slp library    */
   /* _prepared_ to make the call.                                     */
   check_error_state(err, "Error registering service with slp.");
   check_error_state(callbackerr, "Error registering service with slp.");
   
   /* Now that we're done using slp, close the slp handle */ 
   SLPClose(hslp); 

   return(0);
}

/*=========================================================================*/ 
