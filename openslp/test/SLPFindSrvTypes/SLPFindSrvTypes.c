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

/** Test for SLPFindSrvTypes.
 *
 * @file       SLPFindSrvTypes.c
 * @date       Sun Aug 20 21:06:18 IST 2000
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    TestCode
 */

#include <slp.h>
#include <slp_debug.h>
#include <stdio.h>

SLPBoolean MySLPSrvTypeCallback(SLPHandle hslp, const char * pcSrvTypes,
      SLPError errcode, void * cookie)
{
   (void)hslp;
   switch (errcode)
   {
      case SLP_OK:
         printf("Service Types     = %s\n", pcSrvTypes);
         *(SLPError *) cookie = SLP_OK;
         break;

      case SLP_LAST_CALL:
         break;

      default:
         *(SLPError *)cookie = errcode;
         break;
   }
   return SLP_TRUE;
}

int main(int argc, char * argv[])
{
   SLPError err;
   SLPError callbackerr;
   SLPHandle hslp;

   if (argc != 2)
   {
      printf("SLPFindSrvTypes\n  Finds a SLP service.\n "
            "Usage:\n   SLPFindSrvTypes\n     <naming authority>\n");
      return 0;
   } /* End If. */

   err = SLPOpen("en", SLP_FALSE, &hslp);
   check_error_state(err, "Error opening slp handle.");

   err = SLPFindSrvTypes(hslp, argv[1], 0,
         MySLPSrvTypeCallback, &callbackerr);
   check_error_state(err, "Error getting service type with slp.");

   /* Now that we're done using slp, close the slp handle */
   SLPClose(hslp);

   return 0;
}

/*=========================================================================*/ 
