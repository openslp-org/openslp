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

/** Functions for fetching SPI information from the file system.
 *
 * The current implementation uses OpenSSL. For details see the OpenSSL
 * web site (http://www.openssl.org).
 *
 * @file       slp_spi.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeSPI
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
#include "slp_spi.h"
#include "slp_xmalloc.h"

#include <openssl/pem.h>
#include <openssl/bn.h>

#define MAX_SPI_ENTRY_LEN   1024
#define PUBLIC_TOKEN        "PUBLIC"
#define PRIVATE_TOKEN       "PRIVATE"

/** Free an SPI entry.
 *
 * @param[in] victim - The SPI Entry to destroy.
 * 
 * @internal
 */
static void SLPSpiEntryFree(SLPSpiEntry * victim)
{
   xfree(victim->keyfilename);
   xfree(victim->spistr);
   SLPCryptoDSAKeyDestroy(victim->key);
   xfree(victim);
}

/** Locates a specific SPI entry in an SPI cache.
 *
 * @param[in] cache - The list of SPI entries to search.
 * @param[in] keytype - The key type to associate with the entry.
 * @param[in] spistr - The SPI string to search for.
 * @param[in] spistrlen - The length of @p spistr.
 *
 * @returns The SPI entry object read from the file.
 * 
 * @note Pass in a null spistr to find the first cached entry.
 *
 * @internal
 */
static SLPSpiEntry * SLPSpiEntryFind(SLPList * cache, int keytype, 
      size_t spistrlen, const char * spistr)
{
   SLPSpiEntry * entry = (SLPSpiEntry *)cache->head;
   while (entry)
   {
      if (spistr)
      {
         if (entry->spistrlen == spistrlen 
               && memcmp(entry->spistr,spistr,spistrlen) == 0 
               && entry->keytype == keytype)
            return entry;
      }
      else if (keytype == SLPSPI_KEY_TYPE_ANY || entry->keytype == keytype)
         return entry;
      entry = (SLPSpiEntry*)entry->listitem.next;
   }
   return 0;
}

/** Reads a key from a key file.
 *
 * @param[in] keyfile - An open key file pointer.
 * @param[in] keytype - The type of the key to read.
 *
 * @returns A pointer to the DSA key object that was read.
 * 
 * @internal
 */
static SLPCryptoDSAKey * SLPSpiReadKeyFile(const char * keyfile, int keytype)
{
   FILE * fp;
   SLPCryptoDSAKey * result = 0;

   fp = fopen(keyfile, "r");
   if (fp)
   {
      if (keytype == SLPSPI_KEY_TYPE_PUBLIC)
         result = PEM_read_DSA_PUBKEY(fp, &result, 0, 0);
      else if (keytype == SLPSPI_KEY_TYPE_PRIVATE)
         result =  PEM_read_DSAPrivateKey(fp, &result, 0, 0);
      fclose(fp);
   }
   return result;
}

/** Reads an SPI entry from a file.
 *
 * @param[in] fp - An open SPI file pointer
 * @param[in] keytype - The type of the key to associate with the entry.
 *
 * @returns The SPI entry object read from the file.
 * 
 * @note The caller should free the entry with SLPSpiEntryFree.
 *
 * @internal
 */
static SLPSpiEntry * SLPSpiReadSpiFile(FILE * fp, int keytype)
{
   SLPSpiEntry * result;
   char tmp;
   char * line;
   char * slider1;
   char * slider2;

   /* allocate memory for result */
   line = (char *)xmalloc(MAX_SPI_ENTRY_LEN);
   result = (SLPSpiEntry *)xmalloc(sizeof(SLPSpiEntry));
   if (result == 0 || line == 0)
   {
      xfree(line);
      return 0;
   }
   memset(result, 0, sizeof(SLPSpiEntry));

   /* read the next valid entry */
   while (fgets(line, MAX_SPI_ENTRY_LEN, fp))
   {
      /* read the first token */
      slider1 = line;
      /* skip leading whitespace */
      while (*slider1 && *slider1 <= 0x20) 
         slider1++;
      /* skip all white lines */
      if (*slider1 == 0) 
         continue;
      /* skip commented lines */
      if (*slider1 == '#') 
         continue;
      /* PUBLIC|PRIVATE */
      slider2 = slider1;
      while (*slider2 && *slider2 > 0x20) 
         slider2++;
      if (strncasecmp(PUBLIC_TOKEN,slider1,slider2-slider1) == 0)
      {
         if (keytype == SLPSPI_KEY_TYPE_PRIVATE) 
            continue;
         result->keytype = SLPSPI_KEY_TYPE_PUBLIC;
      }
      else if (strncasecmp(PRIVATE_TOKEN, slider1, slider2-slider1) == 0)
      {
         if (keytype == SLPSPI_KEY_TYPE_PUBLIC) 
            continue;
         result->keytype = SLPSPI_KEY_TYPE_PRIVATE;
      }
      else
         continue;   /* unknown token */

      /* read the second token */
      slider1=slider2;
      /* skip leading whitespace */
      while (*slider1 && *slider1 <= 0x20) 
         slider1++;
      /* SPI string */
      slider2 = slider1;
      while (*slider2 && *slider2 > 0x20) 
         slider2++;
      /* SPI string is at slider1 length slider2 - slider1 */

      result->spistr = (char *)xmalloc(slider2-slider1);
      if (result->spistr)
      {
         memcpy(result->spistr, slider1, slider2-slider1);
         result->spistrlen = slider2-slider1;    
      }   

      /* read the third token */
      slider1=slider2;
      /* skip leading whitespace */
      while (*slider1 && *slider1 <= 0x20) 
         slider1++;
      /* SPI string */
      slider2 = slider1;
      while (*slider2 && *slider2 > 0x20) 
         slider2++;
      /* key file path is at slider1 length slider2 - slider1 */
      tmp = *slider2; 
      *slider2 = 0;
      result->keyfilename = xstrdup(slider1);
      result->key = 0; /* read it later */ 
      *slider2 = tmp;

      /* See what we got */
      if (result && result->spistr && result->keyfilename)
         goto SUCCESS;

      xfree(result->keyfilename);
      xfree(result->spistr);
   }
   xfree(result);
   result = 0;

SUCCESS:

   xfree(line);
   return result;
}

/** Initializes SLP SPI data storage.
 *
 * @param[in] spifile - The path of the slp.spi file.
 * @param[in] cacheprivate - Flag indicating whether private keys should
 *    be cached in the handle, or not. True if they should be cached.
 * 
 * @returns A valid SPI handle on success; 0 on failure.
 */
SLPSpiHandle SLPSpiOpen(const char * spifile, int cacheprivate)
{
   FILE * fp;
   SLPSpiHandle result = 0;
   SLPSpiEntry * spientry;      

   fp = fopen(spifile,"r");
   if (fp)
   {
      result = xmalloc(sizeof(struct _SLPSpiHandle));
      if (result == 0) 
         return 0;
      memset(result, 0, sizeof(struct _SLPSpiHandle));

      result->spifile = xstrdup(spifile);
      result->cacheprivate = cacheprivate;
      while (1)
      {
         spientry = SLPSpiReadSpiFile(fp, SLPSPI_KEY_TYPE_ANY);
         if (spientry == 0) 
            break;

         /* destroy the key if we're not suppose to cache it */
         if (spientry->keytype == SLPSPI_KEY_TYPE_PRIVATE && cacheprivate == 0)
            SLPCryptoDSAKeyDestroy(spientry->key);

         SLPListLinkHead(&result->cache, (SLPListItem *)spientry);
      } 
      fclose(fp); 
   }
   return result;
}

/** Release SLP SPI data storage.
 *
 * Memory associated with the specified SPI handle is released.
 *
 * @param[in] hspi - An open SPI handle to be freed.
 */
void SLPSpiClose(SLPSpiHandle hspi)
{
   if (hspi)
   {
      xfree(hspi->spifile);
      while (hspi->cache.count)
         SLPSpiEntryFree((SLPSpiEntry *)
               SLPListUnlink(&hspi->cache,hspi->cache.head));
      xfree(hspi);
   }
}

/** Gets a reference to the default SPI string for a given key type.
 *
 * @param[in] hspi - An open SPI handle.
 * @param[in] keytype - The type of key whose SPI string is desired.
 * @param[out] spistr - The address of storage for the returned 
 *    spistr pointer.
 * @param[out] spistrlen - The length of the value whose address is
 *    returned in @p spistr.
 * 
 * @returns A pointer to the default SPI string.
 *
 * @note The pointer returned may NOT be null-terminated. Use the length 
 *    returned to properly access the memory in the buffer.
 */
char * SLPSpiGetDefaultSPI(SLPSpiHandle hspi, int keytype, 
      size_t * spistrlen, char ** spistr)
{
   SLPSpiEntry * entry;

   *spistr = 0;
   *spistrlen = 0;

   if (hspi)
   {
      entry = SLPSpiEntryFind(&hspi->cache, keytype, 0, 0);
      if (entry)
      {
         *spistr = xmalloc(entry->spistrlen);
         if (*spistr)
         {
            memcpy(*spistr, entry->spistr, entry->spistrlen);
            *spistrlen = entry->spistrlen;
         }
      }
   }
   return *spistr;
}
    
/** Fetches a copy of the DA's private key file.
 *
 * @param[in] hspi - An open SPI handle.
 * @param[in] keytype - The type of key desired.
 * @param[in] spistr - The SPI string associated with the key.
 * @param[in] spistrlen - The length of @p spistr.
 * @param[out] key - The address of storage for the returned private key.
 * 
 * @returns A valid pointer on success, 0 on failure. 
 *
 * @note SLPCryptoDSAKeyDestroy should be used to free key memory.
 */
SLPCryptoDSAKey * SLPSpiGetDSAKey(SLPSpiHandle hspi, int keytype, 
      size_t spistrlen, const char * spistr, SLPCryptoDSAKey ** key)
{
   SLPSpiEntry * tmp = 0;

   /* For safety NULL out the key from the beginning */
   *key = 0;

   if (hspi)
   {
      tmp = SLPSpiEntryFind(&hspi->cache, keytype, spistrlen, spistr);
      if (tmp)
      {
         if (tmp->key == 0)
         {
            if (keytype == SLPSPI_KEY_TYPE_PRIVATE && hspi->cacheprivate == 0)
            {
               *key = SLPSpiReadKeyFile(tmp->keyfilename, 
                     SLPSPI_KEY_TYPE_PRIVATE);
               return *key;
            }
            tmp->key = SLPSpiReadKeyFile(tmp->keyfilename,keytype);
         }
         *key = SLPCryptoDSAKeyDup(tmp->key);
      }
   }
   return *key;
}

/** Determine if we verify using the specified SPI.
 *
 * @param[in] hspi - An open SPI handle.
 * @param[in] spistr - The SPI to check.
 * @param[in] spistrlen - The length of @p spistr.
 * 
 * @returns True (non-zero) if the we verify using the specified SPI; 
 *    False (zero) if not.
 *
 * @note No SPI always returns True.
 */
int SLPSpiCanVerify(SLPSpiHandle hspi, size_t spistrlen, const char * spistr)
{
   if (hspi == 0)
      return 0;

   if (spistrlen == 0 || spistr == NULL)
      return 1;

   return SLPSpiEntryFind(&hspi->cache, SLPSPI_KEY_TYPE_PUBLIC, 
         spistrlen, spistr) != 0;
}

/** Determines if we sign using the specified SPI.
 *
 * @param[in] hspi - An open SPI handle.
 * @param[in] spistr - The SPI to check.
 * @param[in] spistrlen - The length of @p spistr.
 * 
 * @returns True (non-zero) if the we sign using the specified SPI; 
 *    False (zero) if not.
 *
 * @note No SPI always returns True.
 */
int SLPSpiCanSign(SLPSpiHandle hspi, size_t spistrlen, const char * spistr)
{
   return SLPSpiEntryFind(&hspi->cache, SLPSPI_KEY_TYPE_PRIVATE, 
         spistrlen, spistr) != 0;
}

#endif   /* ENABLE_SLPv2_SECURITY */

/*=========================================================================*/
