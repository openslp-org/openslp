/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_crpyto.c                                               */
/*                                                                         */
/* Abstract:    Primitive cryptographic functions to support DSA signature */
/*              of SHA1 digests.  Current implementation is uses the       */
/*              OpenSSL (http://www.openssl.org)) crypto library.          */
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

#include "slp_crypto.h"
#include "slp_message.h"

/*=========================================================================*/
int SLPCryptoSHA1Digest(const unsigned char* data,
                        int datalen,
                        unsigned char* digest)
/* Generate a SHA1 digest for the specified block data                     */
/*                                                                         */
/* Parameters: data     (IN)  pointer to buffer that to be hashed          */
/*             datalen  (IN)  size of the data buffer in bytes             */
/*             digest   (OUT) pointer to buffer of at least 20 bytes in    */
/*                            size where the digest will be copied         */
/*                                                                         */
/* Returns: zero on success.  non-zero on failure                          */
/*=========================================================================*/
{
    if(SHA1(data,datalen,digest))
    {
        return 0;
    }

    return -1;
}


/*=========================================================================*/
SLPCryptoDSAKey* SLPCryptoDSAReadPrivateKey(FILE* fp)
/* Read a private DSA key from the specified PEM encoded key file.         */
/*                                                                         */
/* Parameters:  fp (IN) open PEM encoded key file                          */
/*                                                                         */
/* Returns: pointer to calid DSA key which the caller must destroy         */
/*          (see SLPCryptoDSAKeyDestroy()).  Returns null on failure       */   
/*=========================================================================*/
{
    DSA* result;
    
    result =  PEM_read_DSAPrivateKey(fp, &result, NULL, NULL);

    return result;
}
                                       

/*=========================================================================*/
SLPCryptoDSAKey* SLPCryptoDSAReadPublicKey(FILE* fp)
/* Read a public DSA key from the specified PEM encoded key file.          */
/*                                                                         */
/* Parameters:  fp (IN) open PEM encoded key file                          */
/*                                                                         */
/* Returns: pointer to calid DSA key which the caller must destroy         */
/*          (see SLPCryptoDSAKeyDestroy()).  Returns null on failure       */   
/*=========================================================================*/
{
    DSA* result;
    
    result = PEM_read_DSA_PUBKEY(fp, &result, NULL, NULL);
    
    return result;
}


/*=========================================================================*/
void SLPCryptoDSAKeyDestroy(SLPCryptoDSAKey* dsa)
/* Destroy a key that was created by SLPCryptoDSAKeyCreate(). Care should  */
/* be taken to make sure all private keys are destroyed                    */
/*                                                                         */
/* Parameters: dsa (IN) the key to destroy                                 */
/*                                                                         */
/* Returns:  None                                                          */
/*=========================================================================*/
{
    DSA_free(dsa);
}


/*=========================================================================*/
int SLPCryptoDSASignLen(SLPCryptoDSAKey* key)
/* Determine the length of a signatures produced with specified key.       */
/*                                                                         */
/* Parameters: key (IN) the key that will be used for signing              */
/*                                                                         */
/* Returns: The length of signatures in bytes                              */
/*=========================================================================*/
{
    return DSA_size(key);
}


/*=========================================================================*/
int SLPCryptoDSASign(SLPCryptoDSAKey* key,
                     const unsigned char* digest,
                     int digestlen,
                     unsigned char* signature,
                     int* signaturelen)
/* Sign the specified digest with the specified DSA key                    */
/*                                                                         */
/* Parameters: key          (IN)  Signing (private) key                    */
/*             digest       (IN)  pointer to digest buffer                 */
/*             digestlen    (IN)  length of the digest buffer              */
/*             signature    (OUT) buffer that will hold the ASN.1 DER      */
/*                                encoded signature.                       */
/*             signaturelen (OUT) The length of the signature buffer       */
/*                                SLPCryptoDSASignLen(key) should be       */
/*                                called to determine how big signature    */
/*                                should be.                               */
/*                                                                         */
/* Returns: zero on success. non-zero on failure                           */
/*=========================================================================*/
{
    return DSA_sign(0, /* it does not look like the type param is used? */
                    digest,
                    digestlen,
                    signature,
                    signaturelen,
                    key);
}


/*=========================================================================*/
int SLPCryptoDSAVerify(SLPCryptoDSAKey* key,
                       const unsigned char* digest,
                       int digestlen,
                       const unsigned char* signature,
                       int signaturelen)
/* Verify a DSA signature to ensure it matches the specified digest        */
/*                                                                         */
/* Parameters: key          (IN) Verifying (public) key                    */
/*                          (IN) pointer to the digest buffer              */
/*                          (IN) length of the digest buffer               */
/*                          (IN) the ASN.1 DER encoded signature           */
/*                          (IN) the length of the signature               */
/*                                                                         */
/* Returns: 1 if the signature is valid, 0 of it is not                    */
/*=========================================================================*/
{
    return DSA_verify(0, /* it does not look like the type param is used? */
                      digest,
                      digestlen,
                      (unsigned char*)signature,  /* broken DSA_verify() declaration */
                      signaturelen,
                      key);
}