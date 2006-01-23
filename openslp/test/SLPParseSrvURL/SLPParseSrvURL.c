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

/** Test for SLPParseSrvURL.
 *
 * @file       SLPParseSrvURL.c
 * @date       Wed May 24 14:26:50 EDT 2000
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    TestCode
 */

#include <stdio.h>
#include <slp.h>
#include <slp_debug.h>
#include <string.h>

int main (int argc, char * argv[])
{
   SLPError err;
   SLPSrvURL * parsedurl;

   if (argc != 2)
   {
      printf("SLPParseSrvURL\n  This program tests the parsing of a "
            "service url.\n Usage:\n   SLPParseSrvURL <serivce url>\n");
      return 1;
   }

   err = SLPParseSrvURL(argv[1], &parsedurl); 
   check_error_state(err, "Error parsing SrvURL");

   printf("Service Type = %s\n", parsedurl->s_pcSrvType);
   printf("Host Identification = %s\n", parsedurl->s_pcHost);
   printf("Port Number = %d\n", parsedurl->s_iPort);
   printf("Family = %s\n", ((strlen(parsedurl->s_pcNetFamily)==0)?"IP":"Other"));
   printf("URL Remainder = %s\n", parsedurl->s_pcSrvPart);

   return 0;
}

/*=========================================================================*/ 
