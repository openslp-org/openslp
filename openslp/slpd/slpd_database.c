/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_database.c                                            */
/*                                                                         */
/* Abstract:    Implements database abstraction.  Currently a simple       */
/*              double linked list is used for the underlying storage.     */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/* Copyright (c) 1995, 1999  Caldera Systems, Inc.                         */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify it */
/* under the terms of the GNU Lesser General Public License as published   */
/* by the Free Software Foundation; either version 2.1 of the License, or  */
/* (at your option) any later version.                                     */
/*                                                                         */
/*     This program is distributed in the hope that it will be useful,     */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       */
/*     GNU Lesser General Public License for more details.                 */
/*                                                                         */
/*     You should have received a copy of the GNU Lesser General Public    */
/*     License along with this program; see the file COPYING.  If not,     */
/*     please obtain a copy from http://www.gnu.org/copyleft/lesser.html   */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*     Please submit patches to http://www.openslp.org                     */
/*                                                                         */
/***************************************************************************/

#include "slpd.h"
#include <assert.h>

/*=========================================================================*/
SLPList G_DatabaseList = {0,0,0};
/*=========================================================================*/


/*=========================================================================*/
void SLPDDatabaseEntryFree(SLPDDatabaseEntry* entry)
/* Free all resource related to the specified entry                        */
/*=========================================================================*/
{
    if(entry)
    {
        if(entry->scopelist) free(entry->scopelist);
        if(entry->srvtype) free(entry->srvtype);
        if(entry->url) free(entry->url);
        if(entry->attrlist) free(entry->attrlist);
        if(entry->partiallist) free(entry->partiallist);
        #ifdef USE_PREDICATES
        if(entry->attr) SLPAttrFree(entry->attr);
        #endif 
        free(entry);
    }
}


/*=========================================================================*/
void SLPDDatabaseAge(int seconds)
/* Agea the database entries and clears new and deleted entry lists        */
/*                                                                         */
/* seconds  (IN) the number of seconds to age each entry by                */
/*                                                                         */
/* Returns  - None                                                         */
/*=========================================================================*/
{
    SLPDDatabaseEntry* entry;
    SLPDDatabaseEntry* del   = 0;

    /* Age the database */
    entry = (SLPDDatabaseEntry*)G_DatabaseList.head;
    while(entry)
    {
        /* don't age services with lifetime > SLP_LIFETIME_MAXIMUM */
        if(entry->lifetime < SLP_LIFETIME_MAXIMUM)
        {
            entry->lifetime = entry->lifetime - seconds;
            if(entry->lifetime <= 0)
            {
                del = entry;
            }
        }

        entry = (SLPDDatabaseEntry*)entry->listitem.next;

        if(del)
        {
            SLPDDatabaseEntryFree((SLPDDatabaseEntry*)SLPListUnlink(&G_DatabaseList,(SLPListItem*)del));
            del = 0;
        }
    }
}


/*=========================================================================*/
int SLPDDatabaseReg(SLPSrvReg* srvreg,
                    int fresh)
/* Add a service registration to the database                              */
/*                                                                         */
/* srvreg   -   (IN) pointer to the SLPSrvReg to be added to the database  */
/*                                                                         */
/* fresh    -   (IN) pass in nonzero if the registration is fresh.         */
/*                                                                         */
/* Returns  -   Zero on success.  non-zero on error                        */
/*                                                                         */
/* NOTE:        All registrations are treated as fresh regardless of the   */
/*              setting of the fresh parameter                             */
/*=========================================================================*/
{
    SLPDDatabaseEntry* entry  = (SLPDDatabaseEntry*)G_DatabaseList.head;

    /* Check to see if there is already an identical entry */
    while(entry)
    {
        if(SLPCompareString(entry->urllen,
                            entry->url,
                            srvreg->urlentry.urllen,
                            srvreg->urlentry.url) == 0)
        {
            if(SLPIntersectStringList(entry->scopelistlen,
                                      entry->scopelist,
                                      srvreg->scopelistlen,
                                      srvreg->scopelist) > 0)
            {
                SLPListUnlink(&G_DatabaseList,(SLPListItem*)entry);
                break;
            }
        }

        entry = (SLPDDatabaseEntry*) entry->listitem.next;
    }

    /* if no identical entry are found, create a new one */
    if(entry == 0)
    {
        entry = SLPDDatabaseEntryAlloc();
        if(entry == 0)
        {
            /* Out of memory */
           goto FAILURE;
        }
    }

    /*----------------------------------------------------------------*/
    /* copy info from the message from the wire to the database entry */
    /*----------------------------------------------------------------*/

    /* scope */
    if(entry->scopelistlen >= srvreg->scopelistlen)
    {
        memcpy(entry->scopelist,srvreg->scopelist,srvreg->scopelistlen);
    }
    else
    {
        if(entry->scopelist) free(entry->scopelist);
        entry->scopelist = (char*)memdup(srvreg->scopelist,srvreg->scopelistlen);
        if(entry->scopelist == 0) goto FAILURE;
    }
    entry->scopelistlen = srvreg->scopelistlen;
    
    /* URL */
    if(entry->urllen >= srvreg->urlentry.urllen)
    {
        memcpy(entry->url,srvreg->urlentry.url,srvreg->urlentry.urllen);
    }
    else
    {
        if(entry->url) free(entry->url);
        entry->url = (char*)memdup(srvreg->urlentry.url,srvreg->urlentry.urllen);
        if(entry->url == 0) goto FAILURE;
    }
    entry->urllen = srvreg->urlentry.urllen;
    
    /* lifetime */
    entry->lifetime     = srvreg->urlentry.lifetime;
    
    /* SrvType */
    if(entry->srvtypelen >= srvreg->srvtypelen)
    {
        memcpy(entry->srvtype,srvreg->srvtype,srvreg->srvtypelen);
    }
    else
    {
        if(entry->srvtype) free(entry->srvtype);
        entry->srvtype = (char*)memdup(srvreg->srvtype,srvreg->srvtypelen);
        if(entry->srvtype == 0) goto FAILURE;
    }
    entry->srvtypelen = srvreg->srvtypelen;
    
    /* Attributes */
    if(srvreg->attrlistlen)
    {
        #ifdef USE_PREDICATES
        /* Tricky: perform an in place null termination of the attrlist */
        /*         Remember this squishes the authblock count           */ 
        ((char*)srvreg->attrlist)[srvreg->attrlistlen] = 0;
        if( SLPAttrFreshen(entry->attr, srvreg->attrlist) != SLP_OK)
        {
            goto FAILURE;
        }

        /* Serialize all attributes into entry->attrlist */
        if(entry->partiallist)
        {
            free(entry->partiallist);
            entry->partiallist = 0;
            entry->partiallistlen = 0;
        }
        if( SLPAttrSerialize(entry->attr,
                             "",
                             &entry->attrlist,
                             entry->attrlistlen,
                             &entry->attrlistlen,
                             SLP_FALSE) )
        {
            goto FAILURE;
        }
        #else
        if(entry->attrlistlen >= srvreg->attrlistlen)
        {
            memcpy(entry->attrlist,srvreg->attrlist,srvreg->attrlistlen);
        }
        else
        {
            if(entry->attrlist) free(entry->attrlist);
            entry->attrlist = memdup(srvreg->attrlist,srvreg->attrlistlen);
            if(entry->attrlist == 0) goto FAILURE;
        }
        entry->attrlistlen = srvreg->attrlistlen;
        #endif 
    }
    
    /* link the new (or modified) entry into the list */
    SLPListLinkHead(&G_DatabaseList,(SLPListItem*)entry);

    /* traceReg if necessary */
    SLPDLogTraceReg("Registered", entry);

    return 0;
    
FAILURE:
    if(entry)
    {
        SLPDDatabaseEntryFree(entry);
    }
   
    return -1;
}


/*=========================================================================*/
int SLPDDatabaseDeReg(SLPSrvDeReg* srvdereg)
/* Remove a service registration from the database                         */
/*                                                                         */
/* regfile  -   (IN) filename of the registration file to read into the    */
/*              database. Pass in NULL for no file.                        */
/*                                                                         */
/* Returns  -   Zero on success.  Non-zero if syntax error in registration */
/*              file.                                                      */
/*=========================================================================*/
{
    SLPDDatabaseEntry* entry = (SLPDDatabaseEntry*)G_DatabaseList.head;
    
    while(entry)
    {
        if(SLPCompareString(entry->urllen,
                            entry->url,
                            srvdereg->urlentry.urllen,
                            srvdereg->urlentry.url) == 0)
        {
            if(SLPIntersectStringList(entry->scopelistlen,
                                      entry->scopelist,
                                      srvdereg->scopelistlen,
                                      srvdereg->scopelist) > 0)
            {
                /* Log deregistration registration */
                SLPDLogTraceReg("Deregistered",entry);
                SLPDDatabaseEntryFree((SLPDDatabaseEntry*)SLPListUnlink(&G_DatabaseList,(SLPListItem*)entry));
                break;
            }
        }

        entry = (SLPDDatabaseEntry*) entry->listitem.next;
    }

    return 0;
}


/*=========================================================================*/
int SLPDDatabaseFindSrv(SLPSrvRqst* srvrqst, 
                        SLPDDatabaseSrvUrl* result,
                        int count)
/* Find services in the database                                           */
/*                                                                         */
/* srvrqst  (IN) the request to find.                                      */
/*                                                                         */
/* result   (OUT) pointer to an array of result structures that receives   */
/*                results                                                  */
/*                                                                         */
/* count    (IN)  number of elements in the result array                   */
/*                                                                         */
/* Returns  - The number of services found or < 0 on error.  If the number */
/*            of services found is exactly equal to the number of elements */
/*            in the array, the call may be repeated with a larger array.  */
/*=========================================================================*/
{
    SLPDDatabaseEntry*  entry;
    int                 found;
    
    #ifdef USE_PREDICATES
    /* Tricky: perform an in place null termination of the predicate string */
    /*         Remember this squishes the high byte of spilistlen which is  */
    /*         not a problem because it was already copied                  */
    if(srvrqst->predicate)
    {
        ((char*)srvrqst->predicate)[srvrqst->predicatelen] = 0;
    }
    #endif
    
    /*---------------*/
    /* Test services */
    /*---------------*/
    found = 0;
    entry = (SLPDDatabaseEntry*)G_DatabaseList.head;
    while(entry)
    {
        if(SLPCompareSrvType(srvrqst->srvtypelen,
                             srvrqst->srvtype,
                             entry->srvtypelen,
                             entry->srvtype) == 0)
        {
            if(SLPIntersectStringList(srvrqst->scopelistlen,
                                      srvrqst->scopelist,
                                      entry->scopelistlen,
                                      entry->scopelist))
            {
                #ifdef USE_PREDICATES
                if(srvrqst->predicate && 
                   entry->attr &&
                   SLPDPredicateTest(srvrqst->predicate,entry->attr) == 0)
                #endif
                {
                    result[found].lifetime = entry->lifetime;
                    result[found].urllen = entry->urllen;
                    result[found].url = entry->url;
                    found ++;
                    if(found >= count)
                    {
                        break;
                    }
                }
            }
        }

        entry = (SLPDDatabaseEntry*)entry->listitem.next;
    }
    
    return found;
}


/*=========================================================================*/
int SLPDDatabaseFindType(SLPSrvTypeRqst* srvtyperqst, 
                         SLPDDatabaseSrvType* result,
                         int count)
/* Find service types                                                      */
/*                                                                         */
/* srvtyperqst  (IN) the request to find.                                  */
/*                                                                         */
/* result   (OUT) pointer to an array of result structures that receives   */
/*                results                                                  */
/*                                                                         */
/* count    (IN)  number of elements in the result array                   */
/*                                                                         */
/* Returns  - The number of srvtypes found or <0 on error.  If the number  */
/*            of srvtypes found is exactly equal to the number of elements */
/*            in the array, the call may be repeated with a larger array.  */
/*=========================================================================*/
{
    SLPDDatabaseEntry*  entry;
    int                 found;
    int                 i;


    found = 0;
    entry = (SLPDDatabaseEntry*)G_DatabaseList.head;
    while(entry)
    {
        if(SLPCompareNamingAuth(entry->srvtypelen, entry->srvtype,
                                srvtyperqst->namingauthlen,
                                srvtyperqst->namingauth) == 0)
        {
            if(SLPIntersectStringList(srvtyperqst->scopelistlen,
                                      srvtyperqst->scopelist,
                                      entry->scopelistlen,
                                      entry->scopelist))
            {
                for(i = 0; i < found; i++)
                {
                    if(strncasecmp(result[i].type, entry->srvtype,
                                   entry->srvtypelen) == 0)
                    {
                        break;
                    }
                }
                if(i == found)
                {
                    result[found].type = entry->srvtype;
                    result[found].typelen = entry->srvtypelen;
                    found++;
                    if(found >= count)
                    {
                        break;
                    }
                }
            }
        }

        entry = (SLPDDatabaseEntry*)entry->listitem.next;
    }

    return found;
}


/*=========================================================================*/
int SLPDDatabaseFindAttr(SLPAttrRqst* attrrqst, 
                         SLPDDatabaseAttr* result)
/* Find attributes                                                         */
/*                                                                         */
/* srvtyperqst  (IN) the request to find.                                  */
/*                                                                         */
/* result   (OUT) pointer to a result structure that receives              */
/*                results                                                  */
/*                                                                         */
/* Returns  -   1 on success, zero if not found, negative on error         */
/*=========================================================================*/
{
    SLPDDatabaseEntry*  entry   = 0;
    int                 found   = 0;

    found = 0;
    entry = (SLPDDatabaseEntry*)G_DatabaseList.head;
    while(entry)
    {
        if(SLPCompareString(attrrqst->urllen,
                            attrrqst->url,
                            entry->urllen,
                            entry->url) == 0 ||
           SLPCompareSrvType(attrrqst->urllen,
                             attrrqst->url,
                             entry->srvtypelen,
                             entry->srvtype) == 0 )
        {
            if(SLPIntersectStringList(attrrqst->scopelistlen,
                                      attrrqst->scopelist,
                                      entry->scopelistlen,
                                      entry->scopelist))
            {
                #ifdef USE_PREDICATES
                if(attrrqst->taglistlen && entry->attr)
                {
                    /* serialize into entry->partiallist and return partiallist */
                    size_t count;
                    SLPError err;
                    
                    /* TRICKY: null terminate the taglist. This is squishes the spistrlen */
                    /*         which is not a problem because it was already copied       */
                    ((char*)attrrqst->taglist)[attrrqst->taglistlen] = 0;
                    err = SLPAttrSerialize(entry->attr,
                                           attrrqst->taglist,
                                           &entry->partiallist,
                                           entry->partiallistlen,
                                           &count,
                                           SLP_FALSE);
                    if(err == SLP_BUFFER_OVERFLOW)
                    {
                        /* free previously allocated memory */
                        free(entry->partiallist);
                        entry->partiallist = 0;
                        entry->partiallistlen = 0;

                        /* SLPAttrSerialize will allocate memory for us */
                        err = SLPAttrSerialize(entry->attr,
                                               attrrqst->taglist,
                                               &entry->partiallist,
                                               entry->partiallistlen,
                                               &entry->partiallistlen,
                                               SLP_FALSE);
                        entry->partiallistlen = count;
		            }

                    if(err == SLP_OK)
                    {
                        result->attrlistlen = entry->partiallistlen;
                        result->attrlist = entry->partiallist;
                        found = 1;
                        break;
                    }
                }
                else
                #endif
                {   result->attrlistlen = entry->attrlistlen;
                    result->attrlist = entry->attrlist;
                    found = 1;
                    break;
                }
            }
        }

        entry = (SLPDDatabaseEntry*)entry->listitem.next; 
    }

    return found;
}


/*=========================================================================*/
int SLPDDatabaseInit(const char* regfile)
/* Optionaly initialize the database with registrations from a regfile.    */
/*                                                                         */
/* regfile  (IN)    the regfile to register.                               */
/*                                                                         */
/* Returns  - zero on success or non-zero on error.                        */
/*=========================================================================*/
{
    FILE*               fd;
    SLPDDatabaseEntry*  entry;

    /*--------------------------------------*/
    /* Read static registration file if any */
    /*--------------------------------------*/
    if(regfile)
    {
        fd = fopen(regfile,"rb");
        if(fd)
        {

            SLPLog("Reading registration file %s\n",regfile);

            while(SLPDRegFileReadEntry(fd,&entry) != 0)
            {
                /* Log registration */
                SLPDLogTraceReg("Registered (static)",entry);

                SLPListLinkHead(&G_DatabaseList,(SLPListItem*)entry);
            }

            fclose(fd);
        }
    }

    return 0;
}

#ifdef DEBUG
/*=========================================================================*/
void SLPDDatabaseDeinit()
/* De-initialize the database.  Free all resources taken by registrations  */
/*=========================================================================*/
{
    while(G_DatabaseList.count)
    {
        SLPDDatabaseEntryFree((SLPDDatabaseEntry*)SLPListUnlink(&G_DatabaseList,
                                                                G_DatabaseList.head));
    }
}
#endif

/*=========================================================================*/
SLPDDatabaseEntry *SLPDDatabaseEntryAlloc()
/* Allocates and initializes a database entry.                             */
/*                                                                         */
/* Returns  - zero on success or non-zero on error.                        */
/*=========================================================================*/
{
    SLPDDatabaseEntry *entry;
    
    /* Allocate the entry. */
    entry = (SLPDDatabaseEntry *)malloc(sizeof(SLPDDatabaseEntry));
    if(entry == NULL)
    {
        return NULL;
    }
    memset(entry,0,sizeof(SLPDDatabaseEntry));

    /* Initialize the entry. */
    #ifdef USE_PREDICATES 
    if(SLPAttrAlloc("en", NULL, SLP_FALSE, &entry->attr))
    {
        SLPDDatabaseEntryFree(entry);
        entry = 0;
    }
    #endif  
    
    return entry;
}


/*=========================================================================*/
int SLPDDatabaseEnum(void** handle,
                     SLPDDatabaseEntry** entry)
/* Enumerate through all entries of the database                           */
/*                                                                         */
/* handle (IN/OUT) pointer to opaque data that is used to maintain         */
/*                 enumerate entries.  Pass in a pointer to NULL to start  */
/*                 enumeration.                                            */
/*                                                                         */
/* entry (OUT) pointer to an entry structure pointer that will point to    */
/*             the next entry on valid return                              */
/*                                                                         */
/* returns: >0 if end of enumeration, 0 on success, <0 on error            */
/*=========================================================================*/
{
    if(*handle == 0)
    {
        *entry = (SLPDDatabaseEntry*)G_DatabaseList.head; 
    }
    else
    {
        *entry = (SLPDDatabaseEntry*)*handle;
        *entry = (SLPDDatabaseEntry*)(*entry)->listitem.next;
    }

    *handle = (void*)*entry;

    if(*handle == 0)
    {
        return 1;
    }

    return 0;
}
