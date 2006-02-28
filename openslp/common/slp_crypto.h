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

/** Header file for primitive cryptographic functions.
 *
 * These functions exist to support DSA signature of SHA1 digests. The 
 * current implementation uses the OpenSSL http://www.openssl.org) crypto 
 * library.
 *
 * @file       slp_crypto.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeCrypto
 */

#ifndef SLP_CRYPTO_H_INCLUDED
#define SLP_CRYPTO_H_INCLUDED

/*!@defgroup CommonCodeCrypto Cryptography
 * @ingroup CommonCodeSecurity
 * @{
 */

#ifdef ENABLE_SLPv2_SECURITY

#include <openssl/dsa.h>
#include <openssl/sha.h>

/** A more descriptive name for DSA in this context. */
typedef DSA SLPCryptoDSAKey;

int SLPCryptoSHA1Digest(const unsigned char * data, int datalen, 
      unsigned char * digest);

SLPCryptoDSAKey * SLPCryptoDSAKeyDup(SLPCryptoDSAKey * dsa);

void SLPCryptoDSAKeyDestroy(SLPCryptoDSAKey * dsa);

int SLPCryptoDSASignLen(SLPCryptoDSAKey * key);

int SLPCryptoDSASign(SLPCryptoDSAKey * key, const unsigned char * digest,
      int digestlen, unsigned char * signature, int * signaturelen);

int SLPCryptoDSAVerify(SLPCryptoDSAKey * key, const unsigned char * digest,
      int digestlen, const unsigned char * signature, int signaturelen);

#endif   /* ENABLE_SLPv2_SECURITY */

/*! @} */

#endif   /* SLP_CRYPTO_H_INCLUDED */

/*=========================================================================*/
