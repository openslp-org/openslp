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

/** Primitive crypto functions to support DSA signature of SHA1 digests. 
 *
 * The current implementation uses the OpenSSL (http://www.openssl.org) 
 * cryptographic library.
 *
 * @file       slp_crypto.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeCrypto
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

/** Used to stop compiler warnings about empty translation units. */
int G_Dummy_Enable_SLPv2_Security_Crypto;

#ifdef ENABLE_SLPv2_SECURITY

#include "slp_crypto.h"
#include "slp_message.h"
  
/* 1.1.0 -> 1.0.x compatibility layer
 * See https://wiki.openssl.org/index.php/OpenSSL_1.1.0_Changes#Compatibility_Layer
 * for details and additiona compatibility routines if needed in the future.
 */
#if OPENSSL_VERSION_NUMBER < 0x10100000L
static void DSA_get0_pqg(const DSA *d, const BIGNUM **p, const BIGNUM **q, const BIGNUM **g)
{
    if (p != NULL)
        *p = d->p;
    if (q != NULL)
        *q = d->q;
    if (g != NULL)
        *g = d->g;
}

static int DSA_set0_pqg(DSA *d, BIGNUM *p, BIGNUM *q, BIGNUM *g)
{
    /* If the fields p, q and g in d are NULL, the corresponding input
     * parameters MUST be non-NULL.
     */
    if ((d->p == NULL && p == NULL)
            || (d->q == NULL && q == NULL) 
            || (d->g == NULL && g == NULL))
        return 0;

    if (p != NULL)
    {
        BN_free(d->p);
        d->p = p;
    }
    if (q != NULL)
    {
        BN_free(d->q);
        d->q = q;
    }
    if (g != NULL)
    {
        BN_free(d->g);
        d->g = g;
    }
    return 1;
}

static void DSA_get0_key(const DSA *d, const BIGNUM **pub_key, const BIGNUM **priv_key)
{
    if (pub_key != NULL)
        *pub_key = d->pub_key;
    if (priv_key != NULL)
        *priv_key = d->priv_key;
}

static int DSA_set0_key(DSA *d, BIGNUM *pub_key, BIGNUM *priv_key)
{
    /* If the field pub_key in d is NULL, the corresponding input
     * parameters MUST be non-NULL.  The priv_key field may
     * be left NULL.
     */
    if (d->pub_key == NULL && pub_key == NULL)
        return 0;

    if (pub_key != NULL)
    {
        BN_free(d->pub_key);
        d->pub_key = pub_key;
    }
    if (priv_key != NULL)
    {
        BN_free(d->priv_key);
        d->priv_key = priv_key;
    }
    return 1;
}
#endif

/** Generate a SHA1 digest for the specified block data.
 *
 * @param[in] data - The data block to be hashed.
 * @param[in] datalen - The length of @p data in bytes.
 * @param[out] digest - The address of storage for the digest.
 * 
 * @return Zero on success, or a non-zero error code.
 *
 * @remarks The @p digest parameter must point to a buffer of at least 
 *    20 bytes.
 */
int SLPCryptoSHA1Digest(const unsigned char * data, int datalen, 
      unsigned char * digest)
{
   if (SHA1(data, datalen, digest))
      return 0;
   return -1;
}

/** Duplicates a key.
 *
 * @param[in] dsa - The key to be duplicated.
 * 
 * @return A pointer to a duplicate of @p dsa.
 *
 * @remarks The caller must use SLPCryptoDSAKeyDestroy to release the 
 *    resources used by this key.
 */
SLPCryptoDSAKey * SLPCryptoDSAKeyDup(SLPCryptoDSAKey * dsa)
{
   SLPCryptoDSAKey * result;

   result =  DSA_new();
   if (result)
   {
      const BIGNUM *p, *q, *g;
      const BIGNUM *priv_key, *pub_key;

      DSA_get0_pqg(dsa, &p, &q, &g);
      DSA_get0_key(dsa, &pub_key, &priv_key);

      /* would be nice to check return values,
       * but original code didn't do that either...
       */
      DSA_set0_pqg(result, BN_dup(p), BN_dup(q), BN_dup(g));
      DSA_set0_key(result, BN_dup(pub_key), BN_dup(priv_key));
   }
   return result;
}

/** Destroy a key that was created by SLPCryptoDSAKeyCreate.
 *
 * @param[in] dsa - The key to be destroyed.
 * 
 * @remarks Care should be taken to make sure all private keys 
 *    are destroyed.
 */
void SLPCryptoDSAKeyDestroy(SLPCryptoDSAKey * dsa)
{
   DSA_free(dsa);
}

/** Determine the length of a signatures produced with specified key.
 *
 * @param[in] key - The key that will be used for signing.
 * 
 * @return The length of all signatures generated by this key.
 */
int SLPCryptoDSASignLen(SLPCryptoDSAKey * key)
{
   return DSA_size(key);
}

/** Signs the specified digest with the specified DSA key.
 *
 * @param[in] key - The key to sign with.
 * @param[in] digest - The digest to be signed.
 * @param[in] digestlen - The length of @p digest.
 * @param[out] signature - The address of storage for the signature.
 * @param[in] signaturelen - The length of @p signature.
 * 
 * @return Zero on success, or a non-zero error code.
 *
 * @remarks The caller should call SLPCryptoDSASignLen(key) to determine
 *    how large signature should be.
 */
int SLPCryptoDSASign(SLPCryptoDSAKey * key, const unsigned char * digest,
      int digestlen, unsigned char * signature, int * signaturelen)
{
   /* it does not look like the type param is used? */
   return DSA_sign(0, digest, digestlen, signature, (unsigned*)signaturelen, key) == 0;
}

/** Verifies a DSA signature to ensure it matches the specified digest.
 *
 * @param[in] key - The verifying key.
 * @param[in] digest - The digest buffer to be verified.
 * @param[in] digestlen - The length of @p digest.
 * @param[in] signature - The ASN.1 DER encoded signature.
 * @param[in] signaturelen - The length of @p signature.
 * 
 * @return True (1) if the signature is value; False (0) if not.
 */
int SLPCryptoDSAVerify(SLPCryptoDSAKey * key, const unsigned char * digest,
      int digestlen, const unsigned char * signature, int signaturelen)
{
   /* it does not look like the type param is used? */
   /* broken DSA_verify() declaration */
   return DSA_verify(0, digest, digestlen, (unsigned char *)signature,
         signaturelen, key) > 0? 1:0;
}

#endif   /* ENABLE_SLPv2_SECURITY */

/*=========================================================================*/
