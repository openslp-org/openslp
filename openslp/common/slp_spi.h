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

/** Prototypes for access SPI information from the file system. 
 *
 * Current implementation uses OpenSSL. For details see 
 * http://www.openssl.org.
 *
 * @file       slp_spi.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#ifndef SLP_SPI_H_INCLUDED
#define SLP_SPI_H_INCLUDED

#include "slp_linkedlist.h"
#include "slp_crypto.h"

/*!@defgroup CommonCodeSPI Security Parameter Index */

/*!@addtogroup CommonCodeSPI
 * @ingroup CommonCode
 * @{
 */

typedef struct _SLPSpiEntry
{
   SLPListItem listitem;
   /*!< @brief Make an SPI entry object list-able. 
    */
   int spistrlen;
   /*!< @brief The length of the spi string member.
    */
   char * spistr;
   /*!< @brief The SPI string.
    */
   char * keyfilename;
   /*!< @brief The key file name.
    */
   SLPCryptoDSAKey * key;
   /*!< @brief The cryptographic key material.
    */
   int keytype;
   /*!< @brief The type of key material stored in the key member.
    */
} SLPSpiEntry;

/* Key type values */
#define SLPSPI_KEY_TYPE_ANY     0
#define SLPSPI_KEY_TYPE_PUBLIC  1
#define SLPSPI_KEY_TYPE_PRIVATE 2

typedef struct _SLPSpiHandle
{
   char * spifile;
   /*!< @brief The SPI file name.
    */
   int cacheprivate;
   /*!< @brief A flag indicating whether to privately cache the SPI information.
    */
   SLPList cache;
   /*!< @brief The cache (used only if the cacheprivate member is true).
    */
} * SLPSpiHandle;

SLPSpiHandle SLPSpiOpen(const char* spifile, int cacheprivate);

void SLPSpiClose(SLPSpiHandle hspi);

SLPCryptoDSAKey* SLPSpiGetDSAKey(SLPSpiHandle hspi,
                                 int keytype,
                                 int spistrlen,
                                 const char* spistr,
                                 SLPCryptoDSAKey **key);

char* SLPSpiGetDefaultSPI(SLPSpiHandle hspi, 
                          int keytype,
                          int* spistrlen,
                          char** spistr);

int SLPSpiCanVerify(SLPSpiHandle hspi,
                    int spistrlen,
                    const char* spistr);

int SLPSpiCanSign(SLPSpiHandle hspi,
                  int spistrlen,
                  const char* spistr);

/*! @} */

#endif   /* SLP_SPI_H_INCLUDED */

/*=========================================================================*/
