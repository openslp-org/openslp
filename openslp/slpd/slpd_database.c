/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_database.c                                            */
/*                                                                         */
/* Abstract:    Implements database abstraction.  Currently a simple       */
/*              double linked list (common/slp_database.c) is used for the */
/*              underlying storage.                                        */
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


/*=========================================================================*/
/* slpd includes                                                           */
/*=========================================================================*/
#include "slpd_database.h"
#include "slpd_regfile.h"
#include "slpd_property.h"
#include "slpd_log.h"
#ifdef ENABLE_PREDICATES
#include "slpd_predicate.h"
#endif


/*=========================================================================*/
/* common code includes                                                    */
/*=========================================================================*/
#include "../common/slp_compare.h"
#include "../common/slp_xmalloc.h"

/*=========================================================================*/
SLPDDatabase G_SlpdDatabase;
/* slpd database global                                                    */
/*=========================================================================*/


/*=========================================================================*/
void SLPDDatabaseAge(int seconds, int ageall)
/* Ages the database entries and clears new and deleted entry lists        */
/*                                                                         */
/* seconds  (IN) the number of seconds to age each entry by                */
/*																		   */
/* ageall   (IN) age even entries with SLP_LIFETIME_MAXIMUM                */
/*                                                                         */
/* Returns  - None                                                         */
/*=========================================================================*/
{
    SLPDatabaseHandle   dh;
    SLPDatabaseEntry*   entry;
    SLPSrvReg*          srvreg;

    dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
    if(dh)
    {
        while(1)
        {
            entry = SLPDatabaseEnum(dh);
            if(entry == NULL) break;
    
            /* srvreg is the SrvReg message from the database */
            srvreg = &(entry->msg->body.srvreg);

            if(srvreg->urlentry.lifetime == SLP_LIFETIME_MAXIMUM)
            {
                if(srvreg->source == SLP_REG_SOURCE_LOCAL)
                {
                    /* entries that were made from local registrations    */
                    /* that have a lifetime of SLP_LIFETIME_MAXIMUM must  */
                    /* NEVER be aged                                      */
                    continue;
                }
    
                if(ageall == 0)
                {
                    /* Don't age any services that have a lifetime of     */
                    /* SLP_LIFETIME_MAXIMUM unless explicitly told to     */
                    continue;
                }
            }
            
            srvreg->urlentry.lifetime -= seconds;
            
            /* Remove entries that have timed out */
            if(srvreg->urlentry.lifetime <= 0)
            {
                SLPDatabaseRemove(dh,entry);
            }
        }

        SLPDatabaseClose(dh);
    }
}


/*=========================================================================*/
int SLPDDatabaseReg(SLPMessage msg, SLPBuffer buf)
/* Add a service registration to the database                              */
/*                                                                         */
/* msg          (IN) SLPMessage of a SrvReg message as returned by         */
/*                   SLPMessageParse()                                     */
/*                                                                         */
/* buf          (IN) Otherwise unreferenced buffer interpreted by the msg  */
/*                   structure                                             */
/*                                                                         */
/* Returns  -   Zero on success.  Nonzero on error                         */
/*                                                                         */
/* NOTE:        All registrations are treated as fresh                     */
/*=========================================================================*/
{
    SLPDatabaseHandle   dh;
    SLPDatabaseEntry*   entry;
    SLPSrvReg*          entryreg;
    SLPSrvReg*          reg;
    int                 result;
    
    dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
    if(dh)
    {
        
        /* reg is the SrvReg message being registered */
        reg = &(msg->body.srvreg);

        /*-----------------------------------------------------*/
        /* Check to see if there is already an identical entry */
        /*-----------------------------------------------------*/
        while(1)
        {
            entry = SLPDatabaseEnum(dh);
            if(entry == NULL) break;

            /* entry reg is the SrvReg message from the database */
            entryreg = &(entry->msg->body.srvreg);

            if(SLPCompareString(entryreg->urlentry.urllen,
                                entryreg->urlentry.url,
                                reg->urlentry.urllen,
                                reg->urlentry.url) == 0)
            {
                if(SLPIntersectStringList(entryreg->scopelistlen,
                                          entryreg->scopelist,
                                          reg->scopelistlen,
                                          reg->scopelist) > 0)
                {

#ifdef ENABLE_SECURITY
                    if(G_SlpdProperty.checkSourceAddr &&
                       memcmp(&(entry->msg->peer.sin_addr),
                              &(msg->peer.sin_addr),
                              sizeof(struct in_addr)))
                    {
                        SLPDatabaseClose(dh);
                        return SLP_ERROR_AUTHENTICATION_FAILED;
                    }

                    if(entryreg->urlentry.authcount &&
                       entryreg->urlentry.authcount != reg->urlentry.authcount)
                    {
                        SLPDatabaseClose(dh);
                        return SLP_ERROR_AUTHENTICATION_FAILED;
                    }
#endif
                    
                    /* Remove the identical entry */
                    SLPDatabaseRemove(dh,entry);
                    break;
                }
            }
        }

        /*------------------------------------*/
        /* Add the new srvreg to the database */
        /*------------------------------------*/
        entry = SLPDatabaseEntryCreate(msg,buf);
        if(entry)
        {
            /* set the source (allows for quicker aging ) */
            if(ISLOCAL(msg->peer.sin_addr))
            {
                msg->body.srvreg.source = SLP_REG_SOURCE_LOCAL; 
            }
            else
            {
                msg->body.srvreg.source = SLP_REG_SOURCE_REMOTE;     
            }

            /* add to database */
            SLPDatabaseAdd(dh, entry);
            
            /* SUCCESS! */
            result = 0;
        }
        else
        {
            result = SLP_ERROR_INTERNAL_ERROR;
        }

        SLPDatabaseClose(dh);
    }
    else
    {
        result = SLP_ERROR_INTERNAL_ERROR;
    }

    return result;
}


/*=========================================================================*/
int SLPDDatabaseDeReg(SLPMessage msg)
/* Remove a service registration from the database                         */
/*                                                                         */
/* msg  - (IN) message interpreting an SrvDereg message                    */
/*                                                                         */
/* Returns  -   Zero on success.  Non-zero on failure                      */
/*=========================================================================*/
{
    SLPDatabaseHandle   dh;
    SLPDatabaseEntry*   entry;
    SLPSrvReg*          entryreg;
    SLPSrvDeReg*        dereg;
    
    dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
    if(dh)
    {
        /* dereg is the SrvDereg being deregistered */
        dereg = &(msg->body.srvdereg);
        
        /*---------------------------------------------*/
        /* Check to see if there is an identical entry */
        /*---------------------------------------------*/
        while(1)
        {
            entry = SLPDatabaseEnum(dh);
            if(entry == NULL) break;
        
            /* entry reg is the SrvReg message from the database */
            entryreg = &(entry->msg->body.srvreg);

            if(SLPCompareString(entryreg->urlentry.urllen,
                                entryreg->urlentry.url,
                                dereg->urlentry.urllen,
                                dereg->urlentry.url) == 0)
            {
                if(SLPIntersectStringList(entryreg->scopelistlen,
                                          entryreg->scopelist,
                                          dereg->scopelistlen,
                                          dereg->scopelist) > 0)
                {

#ifdef ENABLE_SECURITY
                    if(G_SlpdProperty.checkSourceAddr &&
                       memcmp(&(entry->msg->peer.sin_addr),
                              &(msg->peer.sin_addr),
                              sizeof(struct in_addr)))
                    {
                        SLPDatabaseClose(dh);
                        return SLP_ERROR_AUTHENTICATION_FAILED;
                    }

                    if(entryreg->urlentry.authcount &&
                       entryreg->urlentry.authcount != dereg->urlentry.authcount)
                    {
                        SLPDatabaseClose(dh);
                        return SLP_ERROR_AUTHENTICATION_FAILED;
                    }
#endif                    
                    /* remove the registration from the database */
                    SLPDatabaseRemove(dh,entry);                   
                    break;
                }
            }
        }

        SLPDatabaseClose(dh);
    }

    return 0;
}

/*=========================================================================*/
int SLPDDatabaseSrvRqstStart(SLPMessage msg,
                             SLPDDatabaseSrvRqstResult** result)
/* Find services in the database                                           */
/*                                                                         */
/* msg      (IN) the SrvRqst to find.                                      */
/*                                                                         */
/* result   (OUT) pointer result structure                                 */
/*                                                                         */
/* Returns  - Zero on success. Non-zero on failure                         */
/*                                                                         */
/* Note:    Caller must pass *result to SLPDDatabaseSrvRqstEnd() to free   */
/*=========================================================================*/
{
    SLPDatabaseHandle           dh;
    SLPDatabaseEntry*           entry;
    SLPSrvReg*                  entryreg;
    SLPSrvRqst*                 srvrqst;
#ifdef ENABLE_SECURITY
    int                         i;
#endif

    
    /* start with the result set to NULL just to be safe */
    *result = NULL;
    
    dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
    if(dh)
    {
        /* srvrqst is the SrvRqst being made */
        srvrqst = &(msg->body.srvrqst);

        while(1)
        {
            /*-----------------------------------------------------------*/
            /* Allocate result with generous array of url entry pointers */
            /*-----------------------------------------------------------*/
            *result = (SLPDDatabaseSrvRqstResult*) xrealloc(*result, sizeof(SLPDDatabaseSrvRqstResult) + (sizeof(SLPUrlEntry*) * G_SlpdDatabase.urlcount));
            if(*result == NULL)
            {
                /* out of memory */
                SLPDatabaseClose(dh);
                return SLP_ERROR_INTERNAL_ERROR;
            }
            (*result)->urlarray = (SLPUrlEntry**)((*result) + 1);
            (*result)->urlcount = 0;
            (*result)->reserved = dh;
            
            /*-------------------------------------------------*/
            /* Rewind enumeration in case we had to reallocate */
            /*-------------------------------------------------*/
            SLPDatabaseRewind(dh);
            
            /*-----------------------------------------*/
            /* Check to see if there is matching entry */
            /*-----------------------------------------*/
            while(1)
            {
                entry = SLPDatabaseEnum(dh);
                if(entry == NULL) 
                {
                    /* This is the only successful way out */
                    return 0;
                }
        
                /* entry reg is the SrvReg message from the database */
                entryreg = &(entry->msg->body.srvreg);

                /* check the service type */
                if(SLPCompareSrvType(srvrqst->srvtypelen,
                                     srvrqst->srvtype,
                                     entryreg->srvtypelen,
                                     entryreg->srvtype) == 0 &&
                   SLPIntersectStringList(entryreg->scopelistlen,
                                          entryreg->scopelist,
                                          srvrqst->scopelistlen,
                                          srvrqst->scopelist) > 0 )
                { 
#ifdef ENABLE_PREDICATES
                    if(SLPDPredicateTest(msg->header.version,
                                         entryreg->attrlistlen,
                                         entryreg->attrlist,
                                         srvrqst->predicatelen,
                                         srvrqst->predicate) )
#endif
                    {
                        
#ifdef ENABLE_SECURITY
                        if(srvrqst->spistrlen)
                        {
                            for(i=0; i< entryreg->urlentry.authcount;i++)
                            {
                                if(SLPCompareString(srvrqst->spistrlen,
                                                    srvrqst->spistr,
                                                    entryreg->urlentry.autharray[i].spistrlen,
                                                    entryreg->urlentry.autharray[i].spistr) == 0)
                                {
                                    break;
                                }
                            }
                            if(i == entryreg->urlentry.authcount)
                            {
                                continue;
                            }
                        }
#endif
                        if((*result)->urlcount + 1 > G_SlpdDatabase.urlcount)
                        {
                            /* Oops we did not allocate a big enough result */
                            G_SlpdDatabase.urlcount *= 2;
                            break;
                        }
    
                        (*result)->urlarray[(*result)->urlcount] = &(entryreg->urlentry);
                        (*result)->urlcount ++;
                    }
                }
            }
        }
    }

    return 0;
}


/*=========================================================================*/
void SLPDDatabaseSrvRqstEnd(SLPDDatabaseSrvRqstResult* result)
/* Find services in the database                                           */
/*                                                                         */
/* result   (IN) pointer result structure previously passed to             */
/*               SLPDDatabaseSrvRqstStart                                  */
/*                                                                         */
/* Returns  - None                                                         */
/*=========================================================================*/
{
    if(result)
    {
        SLPDatabaseClose((SLPDatabaseHandle)result->reserved);
        xfree(result);
    }
}


/*=========================================================================*/
int SLPDDatabaseSrvTypeRqstStart(SLPMessage msg,
                                 SLPDDatabaseSrvTypeRqstResult** result)
/* Find service types in the database                                      */
/*                                                                         */
/* msg      (IN) the SrvTypRqst to find.                                   */
/*                                                                         */
/* result   (OUT) pointer result structure                                 */
/*                                                                         */
/* Returns  - Zero on success. Non-zero on failure                         */
/*                                                                         */
/* Note:    Caller must pass *result to SLPDDatabaseSrvtypeRqstEnd() to    */
/*          free                                                           */
/*=========================================================================*/
{
    SLPDatabaseHandle           dh;
    SLPDatabaseEntry*           entry;
    SLPSrvReg*                  entryreg;
    SLPSrvTypeRqst*             srvtyperqst;
    
    dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
    if(dh)
    {
        /* srvtyperqst is the SrvTypeRqst being made */
        srvtyperqst = &(msg->body.srvtyperqst);

        while(1)
        {
            /*-----------------------------------------------------------------*/
            /* Allocate result with generous srvtypelist of url entry pointers */
            /*-----------------------------------------------------------------*/
            *result = (SLPDDatabaseSrvTypeRqstResult*) xrealloc(*result, sizeof(SLPDDatabaseSrvTypeRqstResult) + G_SlpdDatabase.srvtypelistlen);
            if(*result == NULL)
            {
                /* out of memory */
                SLPDatabaseClose(dh);
                return SLP_ERROR_INTERNAL_ERROR;
            }
            (*result)->srvtypelist = (char*)((*result) + 1);
            (*result)->srvtypelistlen = 0;
            (*result)->reserved = dh;

            /*-------------------------------------------------*/
            /* Rewind enumeration in case we had to reallocate */
            /*-------------------------------------------------*/
            SLPDatabaseRewind(dh);
        
            while(1)
            {
                entry = SLPDatabaseEnum(dh);
                if(entry == NULL) 
                {
                    /* This is the only successful way out */
                    return 0;
                }
    
                /* entry reg is the SrvReg message from the database */
                entryreg = &(entry->msg->body.srvreg);
    
                if(SLPCompareNamingAuth(entryreg->srvtypelen,
                                        entryreg->srvtype,
                                        srvtyperqst->namingauthlen,
                                        srvtyperqst->namingauth) == 0 && 
                   SLPIntersectStringList(srvtyperqst->scopelistlen,
                                          srvtyperqst->scopelist,
                                          entryreg->scopelistlen,
                                          entryreg->scopelist) &&
                   SLPContainsStringList((*result)->srvtypelistlen, 
                                         (*result)->srvtypelist,
                                         entryreg->srvtypelen,
                                         entryreg->srvtype) == 0 )
                {
                    /* Check to see if we allocated a big enough srvtypelist */
                    if((*result)->srvtypelistlen + entryreg->srvtypelen > G_SlpdDatabase.srvtypelistlen)
                    {
                        /* Oops we did not allocate a big enough result */
                        G_SlpdDatabase.srvtypelistlen *= 2;
                        break;
                    }

                    /* Append a comma if needed */
                    if((*result)->srvtypelistlen)
                    {
                        (*result)->srvtypelist[(*result)->srvtypelistlen] = ',';
                        (*result)->srvtypelistlen += 1;
                    }
                    /* Append the service type */
                    memcpy(((*result)->srvtypelist) + (*result)->srvtypelistlen,
                           entryreg->srvtype,
                           entryreg->srvtypelen);
                    (*result)->srvtypelistlen += entryreg->srvtypelen;
                }
            }
        }
       
        SLPDatabaseClose(dh);
    }

    return 0;
}


/*=========================================================================*/
void SLPDDatabaseSrvTypeRqstEnd(SLPDDatabaseSrvTypeRqstResult* result)
/* Release resources used to find service types in the database            */
/*                                                                         */
/* result   (IN) pointer result structure previously passed to             */
/*               SLPDDatabaseSrvTypeRqstStart                              */
/*                                                                         */
/* Returns  - None                                                         */
/*=========================================================================*/
{
    if(result)
    {
        SLPDatabaseClose((SLPDatabaseHandle)result->reserved);
        xfree(result);
    }
}


/*=========================================================================*/
int SLPDDatabaseAttrRqstStart(SLPMessage msg,
                              SLPDDatabaseAttrRqstResult** result)
/* Find attributes in the database                                         */
/*                                                                         */
/* msg      (IN) the AttrRqst to find.                                     */
/*                                                                         */
/* result   (OUT) pointer result structure                                 */
/*                                                                         */
/* Returns  - Zero on success. Non-zero on failure                         */
/*                                                                         */
/* Note:    Caller must pass *result to SLPDDatabaseAttrRqstEnd() to       */
/*          free                                                           */
/*=========================================================================*/
{
    SLPDatabaseHandle           dh;
    SLPDatabaseEntry*           entry;
    SLPSrvReg*                  entryreg;
    SLPAttrRqst*                attrrqst;
#ifdef ENABLE_SECURITY
    int                         i;
#endif
    
    *result = xmalloc(sizeof(SLPDDatabaseAttrRqstResult));
    if(*result == NULL)
    {                   
        return SLP_ERROR_INTERNAL_ERROR;
    }
    memset(*result,0,sizeof(SLPDDatabaseAttrRqstResult));
    
    dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
    if(dh)
    {
        (*result)->reserved = dh;
        
        /* attrrqst is the AttrRqst being made */
        attrrqst = &(msg->body.attrrqst);

        while(1)
        {
            entry = SLPDatabaseEnum(dh);
            if(entry == NULL) 
            {
                return 0;
            }

            /* entry reg is the SrvReg message from the database */
            entryreg = &(entry->msg->body.srvreg);


            if(SLPCompareString(attrrqst->urllen,
                                attrrqst->url,
                                entryreg->urlentry.urllen,
                                entryreg->urlentry.url) == 0 ||
               SLPCompareSrvType(attrrqst->urllen,
                                 attrrqst->url,
                                 entryreg->srvtypelen,
                                 entryreg->srvtype) == 0)
            {
                if(SLPIntersectStringList(attrrqst->scopelistlen,
                                          attrrqst->scopelist,
                                          entryreg->scopelistlen,
                                          entryreg->scopelist))
                {
                    if(attrrqst->taglistlen == 0)
                    {
#ifdef ENABLE_SECURITY
                        if(attrrqst->spistrlen)
                        {
                            for(i=0; i< entryreg->authcount;i++)
                            {
                                if(SLPCompareString(attrrqst->spistrlen,
                                                    attrrqst->spistr,
                                                    entryreg->autharray[i].spistrlen,
                                                    entryreg->autharray[i].spistr) == 0)
                                {
                                    break;
                                }
                            }
                            if(i == entryreg->authcount)
                            {
                                continue;
                            }
                        }
#endif
                        /* Send back what was registered */
                        (*result)->attrlistlen = entryreg->attrlistlen;
                        (*result)->attrlist = (char*)entryreg->attrlist;
                        (*result)->authcount = entryreg->authcount;
                        (*result)->autharray = entryreg->autharray;                        
                    }
#ifdef ENABLE_PREDICATES
                    else
                    {
                        /* Send back a partial list as specified by taglist */
                        if(SLPDFilterAttributes(entryreg->attrlistlen,
                                                entryreg->attrlist,
                                                attrrqst->taglistlen,
                                                attrrqst->taglist,
                                                &(*result)->attrlistlen,
                                                &(*result)->attrlist) == 0)
                        {
                            (*result)->ispartial = 1;
                            break;
                        }
                    }
#endif
                }
            }
        }
    }

    return 0;
}


/*=========================================================================*/
void SLPDDatabaseAttrRqstEnd(SLPDDatabaseAttrRqstResult* result)
/* Release resources used to find attributes in the database               */
/*                                                                         */
/* result   (IN) pointer result structure previously passed to             */
/*               SLPDDatabaseSrvTypeRqstStart                              */
/*                                                                         */
/* Returns  - None                                                         */
/*=========================================================================*/
{
    if(result)
    {
        SLPDatabaseClose((SLPDatabaseHandle)result->reserved);
        if(result->ispartial && result->attrlist) free(result->attrlist);
        xfree(result);
    }
}


/*=========================================================================*/
void* SLPDDatabaseEnumStart()
/* Start an enumeration of the entire database                             */
/*                                                                         */
/* Returns: An enumeration handle that is passed to subsequent calls to    */
/*          SLPDDatabaseEnum().  Returns NULL on failure.  Returned        */
/*          enumeration handle (if not NULL) must be passed to             */
/*          SLPDDatabaseEnumEnd() when you are done with it.               */
/*=========================================================================*/
{
    return SLPDatabaseOpen(&G_SlpdDatabase.database);   
}
    

/*=========================================================================*/
SLPMessage SLPDDatabaseEnum(void* eh, SLPMessage* msg, SLPBuffer* buf)
/* Enumerate through all entries of the database                           */
/*                                                                         */
/* eh (IN) pointer to opaque data that is used to maintain                 */
/*         enumerate entries.  Pass in a pointer to NULL to start          */
/*         enumeration.                                                    */
/*                                                                         */
/* msg (OUT) pointer to the SrvReg message that discribes buf              */
/*                                                                         */
/* buf (OUT) pointer to the SrvReg message buffer                          */
/*                                                                         */
/* returns: Pointer to enumerated entry or NULL if end of enumeration      */
/*=========================================================================*/
{
    SLPDatabaseEntry*   entry;
    entry = SLPDatabaseEnum((SLPDatabaseHandle) eh);
    if(entry)
    {
        *msg = entry->msg;
        *buf = entry->buf;
    }
    else
    {
        *msg = 0;
        *buf = 0;
    }

    return *msg;
}


/*=========================================================================*/
void SLPDDatabaseEnumEnd(void* eh)
/* End an enumeration started by SLPDDatabaseEnumStart()                   */
/*                                                                         */
/* Parameters:  eh (IN) The enumeration handle returned by                 */
/*              SLPDDatabaseEnumStart()                                    */
/*=========================================================================*/
{
    if(eh)
    {
        SLPDatabaseClose((SLPDatabaseHandle)eh);
    }
}


/*=========================================================================*/
int SLPDDatabaseIsEmpty()
/* Returns an boolean value indicating whether the database is empty       */
/*=========================================================================*/
{
    int result = 1;

    SLPDatabaseHandle dh;
    dh = SLPDatabaseOpen(&G_SlpdDatabase.database);
    {
        result = SLPDatabaseCount(dh) == 0;
        SLPDatabaseClose(dh);
    }
    return result;
}


/*=========================================================================*/
int SLPDDatabaseInit(const char* regfile)
/* Initialize the database with registrations from a regfile.              */
/*                                                                         */
/* regfile  (IN)    the regfile to register.  Pass in NULL for no regfile  */
/*                                                                         */
/* Returns  - zero on success or non-zero on error.                        */
/*=========================================================================*/
{
    FILE* fd;
    int result;
    SLPMessage msg;
    SLPBuffer buf;

    /* Set initial values */
    memset(&G_SlpdDatabase,0,sizeof(G_SlpdDatabase));
    G_SlpdDatabase.urlcount = SLPDDATABASE_INITIAL_URLCOUNT;
    G_SlpdDatabase.srvtypelistlen = SLPDDATABASE_INITIAL_SRVTYPELISTLEN;
    SLPDatabaseInit(&G_SlpdDatabase.database);
    
    /*--------------------------------------*/
    /* Read static registration file if any */
    /*--------------------------------------*/
    if(regfile)
    {
        fd = fopen(regfile,"rb");
        if(fd)
        {
            
            while(SLPDRegFileReadSrvReg(fd, &msg, &buf) >= 0)
            {
                /* Log registration */
                result = SLPDDatabaseReg(msg, buf);
                if(result == 0)
                {
                    SLPDLogRegistration("Service Registration (STATIC)",msg);
                }
                
            }

            fclose(fd);
        }
    }
    
    return 0;
}


/*=========================================================================*/
int SLPDDatabaseReInit(const char* regfile)
/* Re-initialize the database with changed registrations from a regfile.   */
/*                                                                         */
/* regfile  (IN)    the regfile to register.                               */
/*                                                                         */
/* Returns  - zero on success or non-zero on error.                        */
/*=========================================================================*/
{
    return 0;
}

#ifdef DEBUG
/*=========================================================================*/
void SLPDDatabaseDeinit(void)
/* Cleans up all resources used by the database                            */
/*=========================================================================*/
{
    SLPDatabaseDeinit(&G_SlpdDatabase.database);
}


/*=========================================================================*/
void SLPDDatabaseDump(void)
/* Dumps currently valid service registrations present with slpd           */
/*=========================================================================*/
{
    SLPMessage      msg;
    SLPBuffer       buf;
    void* eh;

    eh = SLPDDatabaseEnumStart();
    if(eh)
    {
        SLPDLog("\n========================================================================\n");
        SLPDLog("Dumping Registrations\n");
        SLPDLog("========================================================================\n");
        while(SLPDDatabaseEnum(eh, &msg, &buf))
        {
            SLPDLogMessageInternals(msg);
            SLPDLog("\n");
        }

        SLPDDatabaseEnumEnd(eh);
    }
}
#endif
