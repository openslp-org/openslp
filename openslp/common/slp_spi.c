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

#include "slp_spi.h"

#include <string.h>
#include <stdio.h>
#include <openssl/pem.h>
#include <openssl/bn.h>

#define MAX_SPI_ENTRY_LEN   1024
#define PUBLIC_TOKEN        "PUBLIC"
#define PRIVATE_TOKEN       "PRIVATE"

/*-------------------------------------------------------------------------*/
void SLPSpiEntryFree(SLPSpiEntry* victim)
/*-------------------------------------------------------------------------*/
{
    if(victim->spistr) free(victim->spistr);
    if(victim->key) SLPCryptoDSAKeyDestroy(victim->key);
    if(victim) free(victim);
}

/*-------------------------------------------------------------------------*/
SLPSpiEntry* SLPSpiEntryFind(SLPList* cache,
                             int spistrlen,
                             const char* spistr)
/* pass in null spistr to find the first PRIVATE entry                     */
/*-------------------------------------------------------------------------*/
{
    SLPSpiEntry* entry = (SLPSpiEntry*)cache->head;
    while(entry)
    {
        if(spistr)
        {
            if (entry->spistrlen == spistrlen &&
                strncmp(entry->spistr,spistr,spistrlen) == 0)
            {
                return entry;
            }
        }
        else
        {
            if(entry->keytype == SLPSPI_KEY_TYPE_PRIVATE)
            {
                return entry;
            }
        }
        entry = (SLPSpiEntry*)entry->listitem.next;
    }

    return 0;
}

/*-------------------------------------------------------------------------*/
SLPCryptoDSAKey* SLPSpiReadKeyFile(const char* keyfile, int keytype)
/*-------------------------------------------------------------------------*/
{
    FILE*            fp;
    SLPCryptoDSAKey* result = 0;

    fp = fopen(keyfile,"r");
    if(fp)
    {
        if(keytype == SLPSPI_KEY_TYPE_PUBLIC)
        {
            result = PEM_read_DSA_PUBKEY(fp, &result, NULL, NULL);
        }
        else if (keytype == SLPSPI_KEY_TYPE_PRIVATE)
        {
            result =  PEM_read_DSAPrivateKey(fp, &result, NULL, NULL);
        }
    }

    return result;
}


/*-------------------------------------------------------------------------*/
SLPSpiEntry* SLPSpiReadSpiFile(FILE* fp, int keytype)
/*-------------------------------------------------------------------------*/
{
    SLPSpiEntry*    result;
    char            tmp;
    char*           line;
    char*           slider1;
    char*           slider2;

    /*----------------------------*/
    /* Allocate memory for result */
    /*----------------------------*/
    line = (char*) malloc(MAX_SPI_ENTRY_LEN);
    result = (SLPSpiEntry*) malloc(sizeof(SLPSpiEntry));
    if(result == 0 || line == 0)
    {
        return 0;
    }
    memset(result,0,sizeof(SLPSpiEntry));
    

    /*---------------------------*/
    /* Read the next valid entry */
    /*---------------------------*/
    while(fgets(line, MAX_SPI_ENTRY_LEN, fp))
    {
        /*----------------------*/
        /* read the first token */
        /*----------------------*/
        slider1 = line;
        /* skip leading whitespace */
        while(*slider1 && *slider1 <= 0x20) slider1++;
        /* skip all white lines */
        if(*slider1 == 0) continue;
        /* skip commented lines */
        if(*slider1 == '#') continue;
        /* PUBLIC|PRIVATE */
        slider2 = slider1;
        while(*slider2 && *slider2 > 0x20) slider2++;
        if(strncasecmp(PUBLIC_TOKEN,slider1,slider2-slider1) == 0)
        {
            if(keytype == SLPSPI_KEY_TYPE_PRIVATE) continue;
            result->keytype = SLPSPI_KEY_TYPE_PUBLIC;
        }
        else if(strncasecmp(PRIVATE_TOKEN,slider1,slider2-slider1) == 0)
        {
            if(keytype == SLPSPI_KEY_TYPE_PUBLIC) continue;
            result->keytype = SLPSPI_KEY_TYPE_PRIVATE;
        }
        else
        {
            /* unknown token */
            continue;
        }

        /*-----------------------*/
        /* read the second token */
        /*-----------------------*/
        slider1=slider2;
        /* skip leading whitespace */
        while(*slider1 && *slider1 <= 0x20) slider1++;
        /* SPI string */
        slider2 = slider1;
        while(*slider2 && *slider2 > 0x20) slider2++;
        /* SPI string is at slider1 length slider2 - slider1 */
    
        result->spistr = (char*)malloc(slider2-slider1);
        if(result->spistr)
        {
            memcpy(result->spistr,slider1,slider2-slider1);
            result->spistrlen = slider2-slider1;    
        }   
                
        /*----------------------*/
        /* read the third token */
        /*----------------------*/
        slider1=slider2;
        /* skip leading whitespace */
        while(*slider1 && *slider1 <= 0x20) slider1++;
        /* SPI string */
        slider2 = slider1;
        while(*slider2 && *slider2 > 0x20) slider2++;
        /* key file path is at slider1 length slider2 - slider1 */
        tmp = *slider2; 
        *slider2 = 0;
        result->key = SLPSpiReadKeyFile(slider1,result->keytype);
        *slider2 = tmp;
        
        /*-----------------*/
        /* See what we got */
        /*-----------------*/
        if(result &&
           result->spistr &&
           result->key)
        {
            /* Good entry. Let's return it */
            return result;
        }

        if(result->spistr) free(result->spistr);
        if(result->key) SLPCryptoDSAKeyDestroy(result->key);
    }

    if (result) free(result);

    return 0;
}

/*=========================================================================*/
SLPSpiHandle SLPSpiOpen(const char* spifile, int cacheprivate)
/* Initializes SLP SPI data storage.                                       */
/*                                                                         */
/* Parameters: spifile      (IN) path of slp.spi file                      */
/*             cacheprivate (IN) should private keys be cached in handle   */
/*                                                                         */
/* Returns:  valid pointer.  NULL on failure                               */
/*=========================================================================*/
{
    FILE*           fp;
    SLPSpiHandle    result = 0;
    SLPSpiEntry*    spientry;      

    fp = fopen(spifile,"r");
    if(fp)
    {
        result = malloc(sizeof(struct _SLPSpiHandle));
        if(result == 0) return 0;
        memset(result, 0, sizeof(struct _SLPSpiHandle));
        
        result->spifile = strdup(spifile);
        result->cacheprivate = cacheprivate;
        while(1)
        {
            spientry = SLPSpiReadSpiFile(fp, cacheprivate ? SLPSPI_KEY_TYPE_NONE : SLPSPI_KEY_TYPE_PUBLIC);
            if(spientry == 0) break;
            SLPListLinkHead(&(result->cache),(SLPListItem*)spientry);
        }   
    }

    return result;
}

/*=========================================================================*/
void SLPSpiClose(SLPSpiHandle hspi)
/* Release SLP SPI data storage associated with the specified SLPSpiHandle */
/*                                                                         */
/* Parameters: hspi (IN) SLPSpiHandle to deinitialize                      */
/*=========================================================================*/
{
    while(hspi->cache.count)
    {
        SLPSpiEntryFree((SLPSpiEntry*)SLPListUnlink(&(hspi->cache),hspi->cache.head));
    }
    
    if(hspi->spifile) free(hspi->spifile);
    free(hspi);
}

/*=========================================================================*/
SLPCryptoDSAKey* SLPSpiFetchPublicDSAKey(SLPSpiHandle hspi,
                                         int spistrlen,
                                         const char* spistr)
/* Fetches a pointer to public DSA key associated with the specified SPI   */
/*                                                                         */
/* Parameters: hspi      (IN)  handle obtained from call to SLPSpiOpen()   */
/*             spistrlen (IN)  the length of the spistr                    */
/*             spistr    (IN)  the SPI string                              */
/*                                                                         */
/* Returns: A valid pointer. NULL on failure.  Returned pointer is valid   */
/*          for the life of the handle.  DO NOT free returned pointer      */
/*=========================================================================*/
{
    SLPSpiEntry* result;

    result = SLPSpiEntryFind(&(hspi->cache), spistrlen, spistr);
    if(result)
    {
        return result->key;
    } 

    return 0;
}


/*=========================================================================*/
SLPCryptoDSAKey* SLPSpiFetchPrivateDSAKey(SLPSpiHandle hspi,
                                          int* spistrlen, 
                                          char** spistr,
                                          SLPCryptoDSAKey **key)
/* Fetches a copy of the private key file used to sign SLP messages.       */
/*                                                                         */
/* Parameters: hspi      (IN)  handle obtained from call to SLPSpiOpen()   */
/*             spistrlen (OUT) the length of *spistr                       */
/*             spistr    (OUT) the SPI string associated with the key in   */
/*                             returned file pointer.  The caller should   */
/*                             free() to free spistr memory.               */
/*                             responsible for freeing the returned memory */
/*             key       (OUT) the private key.  Caller should use         */
/*                             SLPCryptoDSAKeyDestroy() to free key memory */
/*                                                                         */
/* Returns: A valid pointer. NULL on failure. Caller should use            */
/*          SLPCryptoDSAKeyDestroy() to free key memory                    */ 
/*=========================================================================*/
{
    FILE*           fp;
    SLPSpiEntry*    tmp;
    
    if(hspi->cacheprivate)
    {
        tmp = SLPSpiEntryFind(&(hspi->cache), 0, 0);
        if(tmp == 0) return 0;
    }
    else
    {
        fp = fopen(hspi->spifile,"r");
        if(!fp)
        {
            return 0;
        }
        tmp = SLPSpiReadSpiFile(fp, SLPSPI_KEY_TYPE_PRIVATE);
        if(tmp == 0)
        {
            return 0;
        }
    }

    *key =  SLPCryptoDSAKeyDup(tmp->key);
    *spistr = strdup(tmp->spistr);
    *spistrlen = tmp->spistrlen;
    
    return *key;
}


int main(int argc, char* argv[])
{
    SLPSpiHandle hspi;
    SLPCryptoDSAKey* pubkey;
    char* spistr;
    int   spistrlen;
    SLPCryptoDSAKey* privkey;

    hspi = SLPSpiOpen("test.spi",1);
    pubkey = SLPSpiFetchPublicDSAKey(hspi,strlen("mytestspi"),"mytestspi");
    privkey = SLPSpiFetchPrivateDSAKey(hspi,&spistrlen,&spistr,&privkey);
    
    SLPCryptoDSAKeyDestroy(privkey);
    //SLPSpiClose(hspi);
}
