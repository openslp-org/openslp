/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_auth.c                                                 */
/*                                                                         */
/* Abstract:    Common for OpenSLP's SLPv2 authentication implementation   */
/*              Currently only bsd 0x0002 (DSA-SHA1) is supported          */
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

#include "slp_auth.h"
#include "slp_crypto.h"
/*#include "slp_keys.h"*/

/*-------------------------------------------------------------------------*/
int SLPAuthVerifyBuffer(int spistrlen,
                        const char* spistr,
                        int buflen,
                        const unsigned char* buf,
                        int signaturelen,,
                        const unsigned char signature)
/*-------------------------------------------------------------------------*/
{
    unsigned char   digest[20];
    
    SLPCryptoSHA1Digest(buf,buflen,digest);

    
    if(SLPCryptoDSAVerify(SLPCryptoDSAKey* key,
                          digest,
                          sizeof(digest),
                          autharray[i].authstruct,
                          signaturelen))
    {
        return 0;
    }

    return SLP_ERROR_AUTHENTICATION_FAILED;
}
                        


/*=========================================================================*/
int SLPAuthVerifyString(unsigned short stringlen,
                        const char* string,
                        int authcount,
                        const SLPAuthBlock* autharray)
/* Verify authenticity of  the specified attribute list                    */
/*                                                                         */
/* Parameters: stringlen (IN) the length of string to verify               */
/*             string (IN) the list to verify                              */
/*             authcount (IN) the number of blocks in autharray            */
/*             autharray (IN) array of authblocks                          */
/*                                                                         */
/* Returns: 0 on success or SLP_ERROR_xxx code on failure                  */
/*=========================================================================*/
{
    int             i;
    int             tempbufsize;
    unsigned char*  tempbuf         = 0;
    int             result          = SLP_ERROR_AUTHENTICATION_FAILED;
    
    
    /* TODO: Actually get the key */

    for(i=0;i<authcount;i++)
    {
        /* TODO: what if the time stamp is expired? */
        
        /*-------------------------------------------------------*/
        /* Allocate temporary buffer for contiguous data         */
        /*-------------------------------------------------------*/
        /* +8 makes room for:  stringlen (2 bytes)               */
        /*                     autharray[i].spistrlen (2 bytes)  */
        /*                     timestamp 4 bytes                 */
        tmpbufsize = stringlen + autharray[i].spistrlen + 8;
        tmpbuf = realloc(tmpbuf,tmpbufsize);
        if(tmpbuf == 0)
        {
            return SLP_ERROR_INTERNAL;
        }
        
        /*------------------------------------------------------------*/
        /* Calculate the size of the DSA signature from the authblock */
        /*------------------------------------------------------------*/
        /* we have to calculate the signature length since            */
        /* autharray[i].length is (stupidly) the length of the entire */
        /* authblock                                                  */
        signaturelen = autharray[i].length - (autharray[i].spistrlen + 10);
        
        result = SLPAuthVerifyBuffer(autharray[i].spistrlen,
                                     autharray[i].spi,
                                     tmpbufsize,
                                     tmpbuf,
                                     signaturelen,
                                     autharray[i].authstruct);
        if(result == 0)
        {
            break;
        }
    }
    
    if(tmpbuf)
    {
        free(tmpbuf);
    }

    return result;
}


/*=========================================================================*/
int SLPAuthVerifyUrl(const SLPUrlEntry* urlentry)
/* Verify authenticity of  the specified url entry                         */
/*                                                                         */
/* Parameters: urlentry (IN) the url entry to verify                       */
/*                                                                         */
/* Returns: 0 on success or SLP_ERROR_xxx code on failure                  */
/*=========================================================================*/
{
    return 0;
}


/*=========================================================================*/
int SLPAuthVerifyDAAdvert(const SLPDAAdvert* daadvert)
/* Verify authenticity of  the specified DAAdvert                          */
/*                                                                         */
/* Parameters: spistrlen (IN) length of the spi string                     */
/*             sprstr (IN) the spi string                                  */
/*             daadvert (IN) the DAAdvert to verify                        */
/*                                                                         */
/* Returns: 0 on success or SLP_ERROR_xxx code on failure                  */
/*=========================================================================*/
{
    return 0;
}


/*=========================================================================*/
int SLPAuthVerifySAAdvert(const SLPSAAdvert* saadvert)
/* Verify authenticity of  the specified SAAdvert                          */
/*                                                                         */
/* Parameters: spistrlen (IN) length of the spi string                     */
/*             sprstr (IN) the spi string                                  */
/*             saadvert (IN) the SAADVERT to verify                        */
/*                                                                         */
/* Returns: 0 on success or SLP_ERROR_xxx code on failure                  */
/*=========================================================================*/
{
    return 0;
}


/*=========================================================================*/
int SLPAuthSignString(unsigned short spistrlen,
                      const char* spistr,
                      unsigned short stringlen,
                      const char* string,
                      int* authblocklen,
                      void** authblock)
/* Generate an authblock signature for an attribute list                   */
/*                                                                         */
/* Parameters: spistrlen (IN) length of the spi string                     */
/*             sprstr (IN) the spi string                                  */
/*             attrlistlen (IN) the length of the URL to sign              */
/*             attrlist (IN) the url to sign                               */
/*             authblocklen (OUT) the length of the authblock signature    */
/*             authblock (OUT) buffer containing authblock signature       */
/*                                                                         */
/* Returns: 0 on success or SLP_ERROR_xxx code on failure                  */
/*=========================================================================*/
{
    *authblocklen = 0;
    *authblock = 0;
    return 0;
}


/*=========================================================================*/
int SLPAuthSignUrl(unsigned short spistrlen,
                   const char* spistr,
                   unsigned short urllen,
                   const char* url,
                   int* authblocklen,
                   void** authblock)
/* Generate an authblock signature for a Url                               */
/*                                                                         */
/* Parameters: spistrlen (IN) length of the spi string                     */
/*             sprstr (IN) the spi string                                  */
/*             urllen (IN) the length of the URL to sign                   */
/*             url (IN) the url to sign                                    */
/*             authblocklen (OUT) the length of the authblock signature    */
/*             authblock (OUT) buffer containing authblock signature       */
/*                                                                         */
/* Returns: 0 on success or SLP_ERROR_xxx code on failure                  */
/*=========================================================================*/
{
    *authblocklen = 0;
    *authblock = 0;
    return 0;
}


/*=========================================================================*/
int SLPAuthSignDAAdvert(unsigned short spistrlen,
                        const char* spistr,
                        unsigned long bootstamp,
                        unsigned short urllen,
                        const char* url,
                        unsigned short attrlistlen,
                        const char* attrlist,
                        unsigned short scopelistlen,
                        const char* scopelist,
                        unsigned short daspistrlen,
                        const char* daspistr,
                        int* authblocklen,
                        void** authblock)
/* Generate an authblock signature for a DAADVERT                          */
/*                                                                         */
/* Parameters: spistrlen (IN) length of the spi string                     */
/*             sprstr (IN) the spi string                                  */
/*             bootstamp (IN) the statless DA boot timestamp               */
/*             urllen (IN) the length of the URL to sign                   */
/*             url (IN) the url to sign                                    */
/*             attrlistlen (IN) the length of the URL to sign              */
/*             attrlist (IN) the url to sign                               */
/*             scopelistlen (IN) the length of the DA's scope list         */
/*             scopelist (IN) the DA's scope list                          */
/*             daspistrlen (IN) the length of the list of DA's SPIs        */
/*             daspistr (IN) the list of the DA's SPI's                    */
/*             authblocklen (OUT) the length of the authblock signature    */
/*             authblock (OUT) buffer containing authblock signature       */
/*                                                                         */
/* Returns: 0 on success or SLP_ERROR_xxx code on failure                  */
/*=========================================================================*/
{
    *authblocklen = 0;
    *authblock = 0;
    return 0;
}


/*=========================================================================*/
int SLPAuthSignSAAdvert(unsigned short spistrlen,
                        const char* spistr,
                        unsigned short urllen,
                        const char* url,
                        unsigned short attrlistlen,
                        const char* attrlist,
                        unsigned short scopelistlen,
                        const char* scopelist,
                        int* authblocklen,
                        void** authblock)
/* Generate an authblock signature for a SAADVERT                          */
/*                                                                         */
/* Parameters: spistrlen (IN) length of the spi string                     */
/*             sprstr (IN) the spi string                                  */
/*             urllen (IN) the length of the URL to sign                   */
/*             url (IN) the url to sign                                    */
/*             attrlistlen (IN) the length of the URL to sign              */
/*             attrlist (IN) the url to sign                               */
/*             scopelistlen (IN) the length of the DA's scope list         */
/*             scopelist (IN) the DA's scope list                          */
/*             authblocklen (OUT) the length of the authblock signature    */
/*             authblock (OUT) buffer containing authblock signature       */
/*                                                                         */
/* Returns: 0 on success or SLP_ERROR_xxx code on failure                  */
/*=========================================================================*/
{
    *authblocklen = 0;
    *authblock = 0;
    return 0;
}
