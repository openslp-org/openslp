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

/** Common routines for OpenSLP's SLPv2 authentication.
 *
 * This file contains functions that are common to OpenSLP's UA, SA and DA
 * which deal exclusively with SLPv2 authentication message headers and 
 * parsing algorithms. Currently only BSD 0x0002 (DSA-SHA1) is supported.
 *
 * @file       slp_auth.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeAuth
 */

#ifdef _WIN32
// nonstandard extension used : translation unit is empty
# pragma warning (disable : 4206) 
#else
# if HAVE_CONFIG_H
#  include <config.h>
# endif
#endif

#ifdef ENABLE_SLPv2_SECURITY

#include "slp_types.h"
#include "slp_xmalloc.h"
#include "slp_auth.h"
#include "slp_crypto.h"

/** Create a digest for a URL or attribute string.
 *
 * @param[in] spistr - The SPI string to sign.
 * @param[in] spistrlen - The length of @p spistr in bytes.
 * @param[in] string - The string data to sign.
 * @param[in] stringlen - The length of @p string in bytes.
 * @param[in] timestamp - The timestamp to be digested.
 * @param[out] digest - A buffer to receive the digest.
 * 
 * @return Zero on success, or a non-zero error code. The most common
 *    error code at this time is SLP_ERROR_INTERNAL_ERROR.
 *
 * @remarks The @p digest parameter must be at least 
 *    SLPAUTH_SHA1_DIGEST_SIZE bytes in length.
 *
 * @todo Return a real error code here.
 *
 * @internal
 */
static int SLPAuthDigestString(int spistrlen, const char * spistr, 
      int stringlen, const char * string, unsigned long timestamp, 
      unsigned char * digest)
{
   int result = 0;   /* assume success */
   uint8_t * tmpbuf;
   uint8_t * curpos;

   /* allocate temp buffer for contiguous data */
   curpos = tmpbuf = xmalloc(2 + spistrlen + 2 + stringlen + 4);
   if (tmpbuf == 0)
      return SLP_ERROR_INTERNAL_ERROR;

   /* insert SPI string */
   PutUINT16(&curpos, spistrlen);
   memcpy(curpos, spistr, spistrlen);
   curpos += spistrlen;

   /* insert string (attribute string or URL) */ 
   PutUINT16(&curpos, stringlen);
   memcpy(curpos, string, stringlen);
   curpos += stringlen;

   /* insert time stamp */
   PutUINT32(&curpos, timestamp);

   /* generate the digest */
   if (SLPCryptoSHA1Digest(tmpbuf, curpos - tmpbuf, digest))
      result = SLP_ERROR_INTERNAL_ERROR;

   /* cleanup the temporary buffer */
   xfree(tmpbuf);

   return result;
}

/** Digest a DAAdvert Message.
 *
 * @param[in] spistr - The SPI string.
 * @param[in] spistrlen - The length of the @p spistr.
 * @param[in] timestamp - The timestamp of the message.
 * @param[in] bootstamp - The stateless DA boot timestamp.
 * @param[in] url - The URL to sign.
 * @param[in] urllen - The length of @p url.
 * @param[in] attrlist - The attribute list to sign.
 * @param[in] attrlistlen - The length of @p attrlist.
 * @param[in] scopelist - The DA's scope list.
 * @param[in] scopelistlen - length of @p scopelist.
 * @param[in] daspistr - The list of the DA's SPI's.
 * @param[in] daspistrlen - The length of @p daspistr.
 * @param[out] digest - The digest for the specified information.
 *
 * @return Zero on success, or an SLP error code on failure.
 *
 * @internal
 */
static int SLPAuthDigestDAAdvert(unsigned short spistrlen, 
      const char * spistr, unsigned long timestamp, 
      unsigned long bootstamp, unsigned short urllen, 
      const char * url, unsigned short attrlistlen, 
      const char * attrlist, unsigned short scopelistlen, 
      const char * scopelist, unsigned short daspistrlen, 
      const char * daspistr, unsigned char * digest)
{
   int result;
   uint8_t * tmpbuf;
   uint8_t * curpos;

   /* assume success */
   result = 0;

   /* allocate temporary buffer for contiguous storage */
   curpos = tmpbuf = xmalloc(2 + spistrlen + 4 + 2 + urllen + 2 
         + attrlistlen + 2 + scopelistlen + 2 + daspistrlen + 4);
   if (tmpbuf == 0)
      return SLP_ERROR_INTERNAL_ERROR;

   /* copy data into continguous buffer */
   PutUINT16(&curpos, spistrlen);
   memcpy(curpos, spistr, spistrlen);
   curpos += spistrlen;

   PutUINT32(&curpos, bootstamp);

   PutUINT16(&curpos, urllen);
   memcpy(curpos, url, urllen);
   curpos += urllen;

   PutUINT16(&curpos, scopelistlen);
   memcpy(curpos, scopelist, scopelistlen);
   curpos += scopelistlen;

   PutUINT16(&curpos, attrlistlen);
   memcpy(curpos, attrlist, attrlistlen);
   curpos += attrlistlen;

   PutUINT16(&curpos, daspistrlen);
   memcpy(curpos, daspistr, daspistrlen);
   curpos += daspistrlen;

   PutUINT32(&curpos, timestamp);

   /* Generate the digest */
   if (SLPCryptoSHA1Digest(tmpbuf, curpos - tmpbuf, digest))
      result = SLP_ERROR_INTERNAL_ERROR;

   /* cleanup the temporary buffer */
   xfree(tmpbuf);
   return result;
}

/** Sign a Digest.
 *
 * @param[in] spistr - The SPI string to sign.
 * @param[in] spistrlen - The length of @p spistr.
 * @param[in] key - The key to sign the digest with.
 * @param[in] digest - The digest to be signed.
 * @param[out] authblock - The address of storage for the generated 
 *    authblock signature.
 * @param[out] authblocklen - On entry, the size of @p authblock; 
 *    on exit, the number of bytes stored in @p authblock.
 *
 * @return Zero on success, or an SLP error code on failure.
 *
 * @internal
 */
static int SLPAuthSignDigest(int spistrlen, const char * spistr,
      SLPCryptoDSAKey * key, unsigned char * digest, int * authblocklen,
      unsigned char ** authblock)
{
   int signaturelen;
   int result;
   unsigned char * curpos;

   /* Allocate memory for the authentication block
    *   +10 makes room for:
    *       - the bsd (2 bytes)
    *       - the the authblock length (2 bytes)
    *       - the spi string length (2 bytes)
    *       - the timestamp (4 bytes) 
    */
   signaturelen = SLPCryptoDSASignLen(key);
   *authblocklen = spistrlen + signaturelen + 10;
   *authblock = (unsigned char*)xmalloc(*authblocklen);
   if (*authblock == 0)
   {
      result = SLP_ERROR_INTERNAL_ERROR;
      goto ERROR;
   }

   /* fill in the Authblock with everything but the signature */
   curpos =  *authblock;
   PutUINT16(&curpos, 0x0002); /* the BSD for DSA-SHA1 */
   PutUINT16(&curpos,  *authblocklen);
   PutUINT32(&curpos, 0xffffffff); /* very long expiration (for now) */
   PutUINT16(&curpos, spistrlen);
   memcpy(curpos, spistr, spistrlen);
   curpos += spistrlen;

   /* sign the digest and put it in the authblock */
   if (SLPCryptoDSASign(key, digest, SLPAUTH_SHA1_DIGEST_SIZE, 
         curpos, &signaturelen))
   {
      result = SLP_ERROR_INTERNAL_ERROR;
      goto ERROR;
   }
   return 0; /* success */

ERROR: 

   /* Clean up and return errorcode */
   xfree(*authblock);
   *authblock = 0;
   *authblocklen = 0;
   return result;
}

/** Verify a digest.
 *
 * @param[in] hspi - The open SPI handle.
 * @param[in] emptyisfail - If non-zero, messages without 
 *    authblocks will fail.
 * @param[in] key - The key to use to verify the digest signature.
 * @param[in] digest - The digest to be verified.
 * @param[in] autharray - An authentication block array to verify.
 * @param[in] authcount - The number of elements in @p autharray.
 *
 * @return Zero on success, or SLP_ERROR_AUTHENTICATION_FAILED if the
 *    digest could not be verified.
 *
 * @internal
 *
 * NOTE: This method should be static, but it's not currently used.
 */
/*static*/ int SLPVerifyDigest(SLPSpiHandle hspi, int emptyisfail,
      SLPCryptoDSAKey * key, unsigned char * digest, int authcount, 
      const SLPAuthBlock * autharray)
{
   int i;
   int signaturelen;
   int result;
   unsigned long timestamp;

   /* should we fail on emtpy authblock */
   if (emptyisfail)
      result = SLP_ERROR_AUTHENTICATION_FAILED;
   else
      result = SLP_ERROR_OK;

   /* get a timestamp */
   timestamp = time(0);

   /* Iterate and check all authentication blocks
    * If any one of the authblocks can be verified then we
    * accept it 
    */
   for (i = 0; i < authcount; i++)
   {
      /* get a public key for the SPI  */
      key = SLPSpiGetDSAKey(hspi, SLPSPI_KEY_TYPE_PUBLIC,
            autharray[i].spistrlen, autharray[i].spistr, &key);

      /* Continue if we have a key and if the authenticator is not timed out */
      if (key && timestamp <= autharray[i].timestamp)
      {
         /* Calculate the size of the DSA signature from the authblock
          * we have to calculate the signature length since
          * autharray[i].length is (stupidly) the length of the entire
          * authblock 
          */
         signaturelen = autharray[i].length - (autharray[i].spistrlen + 10);

         /* verify the signature */
         if (SLPCryptoDSAVerify(key, digest, SLPAUTH_SHA1_DIGEST_SIZE,
               (uint8_t *)autharray[i].authstruct, signaturelen))
            break;

         result = SLP_ERROR_AUTHENTICATION_FAILED;
      }
   }
   return result;
}

/** Verify authenticity of a specified attribute list.
 *
 * @param[in] hspi - An open SPI handle.
 * @param[in] emptyisfail - If non-zero, messages without 
 *    authblocks will fail.
 * @param[in] string - The list to verify.
 * @param[in] stringlen - The length of @p string.
 * @param[in] autharray - An array of authblocks.
 * @param[in] authcount - The number of blocks in @p autharray.
 * 
 * @return Zero on success, or an SLP error code on failure.
 */
int SLPAuthVerifyString(SLPSpiHandle hspi, int emptyisfail, 
      unsigned short stringlen, const char * string, int authcount, 
      const SLPAuthBlock * autharray)
{
   int i;
   int signaturelen;
   int result;
   unsigned long timestamp;
   SLPCryptoDSAKey * key = 0;
   unsigned char digest[SLPAUTH_SHA1_DIGEST_SIZE];

   /* should we fail on emtpy authblock */
   if (emptyisfail)
      result = SLP_ERROR_AUTHENTICATION_FAILED;
   else
      result = SLP_ERROR_OK;

   /* get a timestamp */
   timestamp = time(0);

   /* Iterate and check all authentication blocks
    *  If any one of the authblocks can be verified then we
    *  accept it
    */
   for (i = 0; i < authcount; i++)
   {
      /* get a public key for the SPI  */
      key = SLPSpiGetDSAKey(hspi, SLPSPI_KEY_TYPE_PUBLIC,
            autharray[i].spistrlen, autharray[i].spistr, &key);

      /* Continue if we have a key and if the authenticator is not timed out */
      if (key && timestamp <= autharray[i].timestamp)
      {

         /* generate the SHA1 digest */
         result = SLPAuthDigestString(autharray[i].spistrlen,
               autharray[i].spistr, stringlen, string, 
               autharray[i].timestamp, digest);
         if (result == 0)
         {
            /* Calculate the size of the DSA signature from the authblock
             * we have to calculate the signature length since
             * autharray[i].length is (stupidly) the length of the entire
             * authblock 
             */
            signaturelen = autharray[i].length - (autharray[i].spistrlen + 10);

            /* verify the signature */
            if (SLPCryptoDSAVerify(key, digest, sizeof(digest),
                  (uint8_t *)autharray[i].authstruct, signaturelen))
               break;

            result = SLP_ERROR_AUTHENTICATION_FAILED;
         }
      }
   }
   SLPCryptoDSAKeyDestroy(key);
   return result;
}

/** Verify the authenticity of a specified URL entry.
 *
 * @param[in] hspi - An open SPI handle.
 * @param[in] emptyisfail - If non-zero, messages without 
 *    authblocks will fail.
 * @param[in] urlentry - The URL entry to verify.
 * 
 * @return Zero on success, or an SLP error code on failure.
 */
int SLPAuthVerifyUrl(SLPSpiHandle hspi, int emptyisfail, 
      const SLPUrlEntry * urlentry)
{
   return SLPAuthVerifyString(hspi, emptyisfail, urlentry->urllen, 
         urlentry->url, urlentry->authcount, urlentry->autharray);
}

/** Verify the authenticity of the specified DAAdvert.
 *
 * @param[in] hspi - An open SPI handle.
 * @param[in] emptyisfail - If non-zero, messages without 
 *    authblocks will fail.
 * @param[in] daadvert - The DAAdvert message to verify.
 * 
 * @return Zero on success, or an SLP error code on failure.
 */
int SLPAuthVerifyDAAdvert(SLPSpiHandle hspi, int emptyisfail, 
      const SLPDAAdvert * daadvert)
{
   int i;
   int signaturelen;
   int result;
   unsigned long timestamp;
   const SLPAuthBlock * autharray;
   int authcount;
   SLPCryptoDSAKey * key = 0;
   unsigned char digest[SLPAUTH_SHA1_DIGEST_SIZE];

   /* should we fail on emtpy authblock */
   if (emptyisfail)
      result = SLP_ERROR_AUTHENTICATION_FAILED;
   else
      result = SLP_ERROR_OK;

   /* get a timestamp */
   timestamp = time(0);

   /* Iterate and check all authentication blocks
    * If any one of the authblocks can be verified then we
    * accept it 
    */
   authcount = daadvert->authcount;
   autharray = daadvert->autharray;
   for (i = 0; i < authcount; i++)
   {
      /* get a public key for the SPI  */
      key = SLPSpiGetDSAKey(hspi, SLPSPI_KEY_TYPE_PUBLIC,
            autharray[i].spistrlen, autharray[i].spistr, &key);

      /* Continue if we have a key and if authenticator is not timed out */
      if (key && timestamp <= autharray[i].timestamp)
      {
         /* generate the SHA1 digest */
         result = SLPAuthDigestDAAdvert(autharray[i].spistrlen,
               autharray[i].spistr, autharray[i].timestamp, 
               daadvert->bootstamp, daadvert->urllen, daadvert->url, 
               daadvert->attrlistlen, daadvert->attrlist, 
               daadvert->scopelistlen, daadvert->scopelist, 
               daadvert->spilistlen, daadvert->spilist, digest);
         if (result == 0)
         {
            /* calculate the size of the DSA signature from the authblock
             * we have to calculate the signature length since
             * autharray[i].length is (stupidly) the length of the entire
             * authblock 
             */
            signaturelen = autharray[i].length - (autharray[i].spistrlen + 10);

            /* verify the signature */
            if (SLPCryptoDSAVerify(key, digest, sizeof(digest),
                  (uint8_t *)autharray[i].authstruct, signaturelen))
               break;

            result = SLP_ERROR_AUTHENTICATION_FAILED;
         }
      }
   }
   SLPCryptoDSAKeyDestroy(key);
   return result;
}

/** Verify authenticity of the specified SAAdvert.
 *
 * @param[in] hspi - An open SPI handle.
 * @param[in] emptyisfail - If non-zero, messages without 
 *    authblocks will fail.
 * @param[in] saadvert - The SAAdvert message to verify.
 * 
 * @return Zero on success, or an SLP error code on failure.
 */
int SLPAuthVerifySAAdvert(SLPSpiHandle hspi, int emptyisfail, 
      const SLPSAAdvert * saadvert)
{
   return 0;
}

/** Sign an authblock.
 *
 * @param[in] hspi - An open SPI handle.
 * @param[in] spistr - The SPI string.
 * @param[in] spistrlen - The length of @p spistr.
 * @param[in] string - The attribute list to sign.
 * @param[in] stringlen - The length of @p string.
 * @param[out] authblock - The address of storage for the 
 *    generated authblock signature.
 * @param[out] authblocklen - On entry, the size of @p authblock; 
 *    on exit, the number of bytes stored in @p authblock.
 * 
 * @return Zero on success, or an SLP error code on failure.
 *
 * @todo Fix the expiration time.
 */
int SLPAuthSignString(SLPSpiHandle hspi, int spistrlen, const char * spistr,
      unsigned short stringlen, const char * string, int * authblocklen,
      unsigned char ** authblock)
{
   int result;
   SLPCryptoDSAKey * key;
   unsigned char digest[20];
   size_t defaultspistrlen = 0;
   char * defaultspistr = 0;

   /* NULL out the authblock and spistr just to be safe */
   key = 0;
   *authblock = 0;
   *authblocklen = 0;

   /* get a private key for the SPI  */
   if (spistr == 0 && SLPSpiGetDefaultSPI(hspi, SLPSPI_KEY_TYPE_PRIVATE,
         &defaultspistrlen, &defaultspistr))
   {
      spistr = defaultspistr;
      spistrlen = defaultspistrlen;
   }
   key = SLPSpiGetDSAKey(hspi, SLPSPI_KEY_TYPE_PRIVATE, 
         spistrlen, spistr, &key);
   if (key == 0)
   {
      result = SLP_ERROR_AUTHENTICATION_UNKNOWN;
      goto ERROR;
   }

   /* Generate the SHA1 digest - use a very long expiration for now */
   result = SLPAuthDigestString(spistrlen, spistr, stringlen, string,
      0xffffffff, digest);

   /* sign the digest and put it in the authblock */
   if (result == 0)
      result = SLPAuthSignDigest(spistrlen, spistr, key, digest, 
            authblocklen, authblock);

ERROR: 

   /* cleanup */
   xfree(defaultspistr);
   SLPCryptoDSAKeyDestroy(key);
   return result;
}

/** Generate an authblock signature for a URL.
 *
 * @param[in] hspi - An open SPI handle.
 * @param[in] spistr - The SPI string.
 * @param[in] spistrlen - The length of @p spistr.
 * @param[in] url - The URL to sign.
 * @param[in] urllen - The length of @p url.
 * @param[out] authblock - The address of storage for the 
 *    generated authblock signature.
 * @param[out] authblocklen - On entry, the size of @p authblock;
 *    on exit, the number of bytes stored in @p authblock.
 * 
 * @return Zero on success, or an SLP error code on failure.
 */
int SLPAuthSignUrl(SLPSpiHandle hspi, int spistrlen, const char * spistr,
      unsigned short urllen, const char * url, int * authblocklen, 
      unsigned char ** authblock)
{
   return SLPAuthSignString(hspi, spistrlen, spistr, urllen, url,
         authblocklen, authblock);
}

/** Generate an authblock signature for a DAADVERT
 *
 * @param[in] hspi - An open SPI handle.
 * @param[in] spistr - The SPI string.
 * @param[in] spistrlen - The length of the @p spistr.
 * @param[in] bootstamp - The stateless DA boot timestamp.
 * @param[in] url - The URL to sign.
 * @param[in] urllen - The length of @p url.
 * @param[in] attrlist - The attribute list to sign.
 * @param[in] attrlistlen - The length of @p attrlist.
 * @param[in] scopelist - The DA's scope list.
 * @param[in] scopelistlen - The length of @p scopelist.
 * @param[in] daspistr - The list of the DA's SPI's.
 * @param[in] daspistrlen - The length of @p daspistr.
 * @param[out] authblock - The address of storage for the 
 *    generated authblock signature.
 * @param[out] authblocklen - On entry, the size of @p authblock;
 *    on exit, the number of bytes stored in @p authblock.
 *
 * @return Zero on success, or an SLP error code on failure.
 */
int SLPAuthSignDAAdvert(SLPSpiHandle hspi, unsigned short spistrlen, 
      const char * spistr, unsigned long bootstamp, unsigned short urllen, 
      const char * url, unsigned short attrlistlen, const char * attrlist, 
      unsigned short scopelistlen, const char * scopelist, 
      unsigned short daspistrlen, const char * daspistr, 
      int * authblocklen, unsigned char ** authblock)
{
   int result;
   SLPCryptoDSAKey * key;
   unsigned char digest[20];
   size_t defaultspistrlen = 0;
   char * defaultspistr = 0;

   /* NULL out the authblock and spistr just to be safe */
   key = 0;
   *authblock = 0;
   *authblocklen = 0;
   spistr = 0;
   spistrlen = 0;

   /* get a private key for the SPI  */
   if (spistr)
      key = SLPSpiGetDSAKey(hspi, SLPSPI_KEY_TYPE_PRIVATE, spistrlen, 
            spistr, &key);
   else
   {
      if (SLPSpiGetDefaultSPI(hspi, SLPSPI_KEY_TYPE_PRIVATE,
            &defaultspistrlen, &defaultspistr))
      {
         spistr = defaultspistr;
         spistrlen = defaultspistrlen;
         key = SLPSpiGetDSAKey(hspi, SLPSPI_KEY_TYPE_PRIVATE, 
               spistrlen, spistr, &key);
      }
   }
   if (key == 0)
   {
      result = SLP_ERROR_AUTHENTICATION_UNKNOWN;
      goto ERROR;
   }

   /* generate the SHA1 digest */
   result = SLPAuthDigestDAAdvert(spistrlen, spistr, 0xffffffff, bootstamp,
         urllen, url, attrlistlen, attrlist, scopelistlen, scopelist,
         daspistrlen, daspistr, digest);
   if (result == 0)   /* sign the digest and put it in the authblock */
      result = SLPAuthSignDigest(spistrlen, spistr, key, digest, authblocklen,
         authblock);

ERROR: 

   /* cleanup */
   xfree(defaultspistr);
   SLPCryptoDSAKeyDestroy(key);
   return result;
}

/** Generate an authblock signature for an SAADVERT message.
 *
 * @param[in] spistr - The SPI string.
 * @param[in] spistrlen - The length of the @p spistr.
 * @param[in] url - The URL to sign.
 * @param[in] urllen - The length of @p url.
 * @param[in] attrlist - The attribute list to sign.
 * @param[in] attrlistlen - The length of @p attrlist.
 * @param[in] scopelist - The DA's scope list.
 * @param[in] scopelistlen - The length of @p scopelist.
 * @param[out] authblock - The address of storage for the 
 *    generated authblock signature.
 * @param[out] authblocklen - On entry, the size of @p authblock;
 *    on exit, the number of bytes stored in @p authblock.
 *
 * @return Zero on success, or an SLP error code on failure.
 */
int SLPAuthSignSAAdvert(unsigned short spistrlen, const char * spistr,
      unsigned short urllen, const char * url, unsigned short attrlistlen, 
      const char * attrlist, unsigned short scopelistlen, 
      const char * scopelist, int * authblocklen, 
      unsigned char ** authblock)
{
   *authblocklen = 0;
   *authblock = 0;
   return 0;
}

#endif   /* ENABLE_SLPv2_SECURITY */

/*=========================================================================*/ 
