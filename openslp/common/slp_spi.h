/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slp_spi.h                                                  */
/*                                                                         */
/* Abstract:    Functions for fetching SPI information from the filesystem */
/*              Current implementation uses OpenSSL. For details see       */
/*              (see http://www.openssl.org                                */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*     Please submit patches to http://www.openslp.org                     */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/* Copyright (C) 2000 Caldera Systems, Inc                                 */
/* All rights reserved.                                                    */
/*                                                                         */
/* Redistribution and use in source and binary forms, with or without      */
/* modification, are permitted provided that the following conditions are  */
/* met:                                                                    */ 
/*                                                                         */
/*      Redistributions of source code must retain the above copyright     */
/*      notice, this list of conditions and the following disclaimer.      */
/*                                                                         */
/*      Redistributions in binary form must reproduce the above copyright  */
/*      notice, this list of conditions and the following disclaimer in    */
/*      the documentation and/or other materials provided with the         */
/*      distribution.                                                      */
/*                                                                         */
/*      Neither the name of Caldera Systems nor the names of its           */
/*      contributors may be used to endorse or promote products derived    */
/*      from this software without specific prior written permission.      */
/*                                                                         */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS     */
/* `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT      */
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   */
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CALDERA      */
/* SYSTEMS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, */
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON       */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    */
/*                                                                         */
/***************************************************************************/

#ifndef SLP_SPI_H_INCLUDED
#define SLP_SPI_H_INCLUDED

#include "slp_linkedlist.h"
#include "slp_crypto.h"


/*-------------------------------------------------------------------------*/
typedef struct _SLPSpiEntry
/*-------------------------------------------------------------------------*/
{
    SLPListItem         listitem;
    int                 spistrlen;
    char*               spistr;
    char*               keyfilename;
    SLPCryptoDSAKey*    key;
    int                 keytype;
}SLPSpiEntry;


/*-----------------*/
/* Key type values */
/*-----------------*/
#define SLPSPI_KEY_TYPE_ANY     0
#define SLPSPI_KEY_TYPE_PUBLIC  1
#define SLPSPI_KEY_TYPE_PRIVATE 2


/*=========================================================================*/
typedef struct _SLPSpiHandle
/*=========================================================================*/
{
    char*       spifile;
    int         cacheprivate;
    SLPList     cache;
}* SLPSpiHandle;


/*=========================================================================*/
SLPSpiHandle SLPSpiOpen(const char* spifile, int cacheprivate);
/* Initializes SLP SPI data storage.                                       */
/*                                                                         */
/* Parameters: spifile      (IN) path of slp.spi file                      */
/*             cacheprivate (IN) should private keys be cached in handle   */
/*                                                                         */
/* Returns:  valid pointer.  NULL on failure                               */
/*=========================================================================*/


/*=========================================================================*/
void SLPSpiClose(SLPSpiHandle hspi);
/* Release SLP SPI data storage associated with the specified SLPSpiHandle */
/*                                                                         */
/* Parameters: hspi (IN) SLPSpiHandle to deinitialize                      */
/*=========================================================================*/


/*=========================================================================*/
SLPCryptoDSAKey* SLPSpiGetDSAKey(SLPSpiHandle hspi,
                                 int keytype,
                                 int spistrlen,
                                 const char* spistr,
                                 SLPCryptoDSAKey **key);
/* Fetches a copy of the private key file used to sign SLP messages.       */
/*                                                                         */
/* Parameters: hspi      (IN)  handle obtained from call to SLPSpiOpen()   */
/*             keytype   (IN)  the type of key desired                     */
/*             spistrlen (IN)  the length of the spistr                    */
/*             spistr    (IN)  spistr associated with the key              */
/*             key       (OUT) the private key.  Caller should use         */
/*                             SLPCryptoDSAKeyDestroy() to free key memory */
/*                                                                         */
/* Returns: A valid pointer. NULL on failure. Caller should use            */
/*          SLPCryptoDSAKeyDestroy() to free key memory                    */ 
/*=========================================================================*/


/*=========================================================================*/
char* SLPSpiGetDefaultSPI(SLPSpiHandle hspi, 
                          int keytype,
                          int* spistrlen,
                          char** spistr);
/* Gets a reference to the default SPI string for the specified keytype    */
/*                                                                         */
/* Parameters: hspi      (IN) handle obtained from call to SLPSpiOpen()    */
/*             keytype   (IN) type of key                                  */
/*             spistrlen (OUT) length or the returned spistr               */
/*             spistr    (OUT) pointer to spistr.  MUST be freed by        */
/*                             caller!!                                    */
/*                                                                         */
/* Returns: Pointer to the default SPI string.  Pointer may *not* be NULL  */
/*          terminated                                                     */
/*=========================================================================*/


/*=========================================================================*/
int SLPSpiCanVerify(SLPSpiHandle hspi,
                    int spistrlen,
                    const char* spistr);
/* Determine if we understand the specified SPI.  No SPI is always         */
/* returns true                                                            */
/*                                                                         */
/* Parameters: hspi      (IN)  handle obtained from call to SLPSpiOpen()   */
/*             spistrlen (IN)  the length of the spistr                    */
/*             spistr    (IN)  the SPI string                              */
/*                                                                         */
/* Returns     Non-zero if we verify specified the SPI                     */
/*=========================================================================*/


/*=========================================================================*/
int SLPSpiCanSign(SLPSpiHandle hspi,
                  int spistrlen,
                  const char* spistr);
/* Determine if we understand the specified SPI.  No SPI is always         */
/* return true                                                             */
/*                                                                         */
/* Parameters: hspi      (IN)  handle obtained from call to SLPSpiOpen()   */
/*             spistrlen (IN)  the length of the spistr                    */
/*             spistr    (IN)  the SPI string                              */
/*                                                                         */
/* Returns     Non-zero if we sign using the specified SPI                 */
/*=========================================================================*/

#endif



