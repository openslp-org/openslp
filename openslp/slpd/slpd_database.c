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

/*=========================================================================*/
SLPDDatabaseEntry*    G_DatabaseHead = 0;
/*=========================================================================*/

/*-------------------------------------------------------------------------*/
void* memdup(const void* src, int srclen)
/*-------------------------------------------------------------------------*/
{
    char* result;
    result = (char*)malloc(srclen);
    if(result)
    {
        memcpy(result,src,srclen);
    }

    return result;
}

/*-------------------------------------------------------------------------*/
SLPDDatabaseEntry* LinkEntry(SLPDDatabaseEntry* entry)
/*-------------------------------------------------------------------------*/

{
    entry->previous = 0;
    entry->next = G_DatabaseHead;
    if(G_DatabaseHead)
    {
        G_DatabaseHead->previous = entry;
    }
    
    G_DatabaseHead = entry;

    return entry;
}

        
/*-------------------------------------------------------------------------*/
SLPDDatabaseEntry* UnlinkEntry(SLPDDatabaseEntry* entry)
/*-------------------------------------------------------------------------*/
{
    if(entry->previous)
    {
        entry->previous->next = entry->next;
    }

    if(entry->next)
    {
        entry->next->previous = entry->previous;
    }

    if(entry == G_DatabaseHead)
    {
        G_DatabaseHead = entry->next;
    }

    return entry;
}

/*-------------------------------------------------------------------------*/
void FreeEntry(SLPDDatabaseEntry* entry)
/*-------------------------------------------------------------------------*/
{
    free(entry->scopelist);
    free(entry->srvtype);
    free(entry->url);
    free(entry->attrlist);
    free(entry);
}

/*-------------------------------------------------------------------------*/
void RemoveEntry(int urllen, 
                 const char* url, 
                 int scopelistlen,
                 const char* scopelist)
/*-------------------------------------------------------------------------*/
{
    SLPDDatabaseEntry* entry = G_DatabaseHead;

    while(entry)
    {
        if(SLPStringCompare(entry->urllen,
                            entry->url,
                            urllen,
                            url) == 0)
        {
            if(SLPStringListIntersect(entry->scopelistlen,
                                      entry->scopelist,
                                      scopelistlen,
                                      scopelist) > 0)
            {
                UnlinkEntry(entry);                
                FreeEntry(entry);

                break;
            } 
        }             
        
        entry = entry->next;
    }
}

/*-------------------------------------------------------------------------*/
void RemoveAllEntries()
/*-------------------------------------------------------------------------*/
{
    SLPDDatabaseEntry*  entry   = G_DatabaseHead;
    SLPDDatabaseEntry*  del     = 0;

    /*--------------------------*/
    /* Remove all registrations */
    /*--------------------------*/
    while(entry)
    {
        del = entry;
        entry = entry->next;
        UnlinkEntry(del); 
    }
    G_DatabaseHead = entry;
}


/*=========================================================================*/
void SLPDDatabaseAge(int seconds)                                          
/* Age the database entries                                                */
/*                                                                         */
/* seconds  (IN) the number of seconds to age each entry by                */
/*                                                                         */
/* Returns  - None                                                         */
/*=========================================================================*/
{
    SLPDDatabaseEntry* entry = G_DatabaseHead;
    SLPDDatabaseEntry* del;

    while(entry)
    {
        entry->lifetime = entry->lifetime - seconds;
        if(entry->lifetime <= 0)
        {
            del = entry;
            entry=entry->next;
            UnlinkEntry(del); 
            continue;
        }
        entry=entry->next;
    }
}


/*=========================================================================*/
int SLPDDatabaseReg(SLPSrvReg* srvreg,
                    int fresh,
                    int pid,
                    int uid)
/* Add a service registration to the database                              */
/*                                                                         */
/* srvreg   -   (IN) pointer to the SLPSrvReg to be added to the database  */
/*                                                                         */
/* fresh    -   (IN) pass in nonzero if the registration is fresh.         */
/*                                                                         */
/* pid      -   (IN) process id of the process that registered the service */
/*                                                                         */
/* uid      -   (IN) user id of the user that registered the service       */
/*                                                                         */
/* Returns  -   Zero on success.  non-zero on error                        */
/*                                                                         */
/* NOTE:        All registrations are treated as fresh regardless of the   */
/*              setting of the fresh parameter                             */
/*=========================================================================*/
{
    int                  result = 0;
    SLPDDatabaseEntry*   entry  = 0;

    RemoveEntry(srvreg->urlentry.urllen,
                srvreg->urlentry.url,
                srvreg->scopelistlen,
                srvreg->scopelist);

    entry = (SLPDDatabaseEntry*)malloc(sizeof(SLPDDatabaseEntry));
    if(entry == 0)
    {
        return -1;
    }
    memset(entry,0,sizeof(SLPDDatabaseEntry));
    
    entry->pid              = pid;
    entry->uid              = uid;
    entry->scopelistlen     = srvreg->scopelistlen;
    entry->scopelist        = (char*)memdup(srvreg->scopelist,srvreg->scopelistlen);
    entry->lifetime  = srvreg->urlentry.lifetime;
    entry->urllen    = srvreg->urlentry.urllen;
    entry->url       = (char*)memdup(srvreg->urlentry.url, srvreg->urlentry.urllen);
    entry->srvtypelen  = srvreg->srvtypelen;
    entry->srvtype     = (char*)memdup(srvreg->srvtype,srvreg->srvtypelen);
    entry->attrlistlen = srvreg->attrlistlen;
    entry->attrlist    = (char*)memdup(srvreg->attrlist,srvreg->attrlistlen);
    
    
    if(entry->scopelist == 0 ||
       entry->url == 0 ||
       entry->srvtype == 0 ||
       entry->attrlist == 0)
    {
        FreeEntry(entry);
        return -1;
    }
    
    LinkEntry(entry);
    
    return result;
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
    RemoveEntry(srvdereg->urlentry.urllen,
                srvdereg->urlentry.url,
                srvdereg->scopelistlen,
                srvdereg->scopelist);

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
/* Returns  - The number of services found or <0 on error.  If the number  */
/*            of services found is exactly equal to the number of elements */
/*            in the array, the call may be repeated with a larger array.  */
/*=========================================================================*/
{
    SLPDDatabaseEntry*  entry;
    int                 found;

    entry = G_DatabaseHead;
    found = 0;
    while(entry)
    {
        if(SLPSrvTypeCompare(srvrqst->srvtypelen,
                             srvrqst->srvtype,
                             entry->srvtypelen,
                             entry->srvtype) == 0)
        {
            /*---------------------------------------*/
            /* TODO: Add predicate query stuff later */
            /*---------------------------------------*/
            
            if(SLPStringListIntersect(srvrqst->scopelistlen,
                                      srvrqst->scopelist,
                                      entry->scopelistlen,
                                      entry->scopelist))
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

        entry = entry->next;
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
/* Returns  - The number of services found or <0 on error.  If the number  */
/*            of services found is exactly equal to the number of elements */
/*            in the array, the call may be repeated with a larger array.  */
/*=========================================================================*/
{
    return 0;
}


/*=========================================================================*/
int SLPDDatabaseFindAttr(SLPAttrRqst* attrrqst, 
                         SLPDDatabaseAttr* result,
                         int count)
/* Find attributes                                                         */
/*                                                                         */
/* srvtyperqst  (IN) the request to find.                                  */
/*                                                                         */
/* result   (OUT) pointer to a result structure that receives              */
/*                results                                                  */
/*                                                                         */
/* count    (IN)  number of elements in the result array                   */
/*                                                                         */
/* Returns  -   >0 on success. 0 if the url of the attrrqst could not be   */
/*              cound and <0 on error.                                     */
/*=========================================================================*/
{
    SLPDDatabaseEntry*  entry   = 0;
    int                 found   = 0;

    /* TODO: Do we ever want to handle passing back all of the attributes  */
    /*       for service types?                                            */

    entry = G_DatabaseHead;
    found = 0;
    while(entry)
    {
        if(SLPStringCompare(attrrqst->urllen,
                            attrrqst->url,
                            entry->urllen,
                            entry->url) == 0 ||
           SLPSrvTypeCompare(attrrqst->urllen,
                             attrrqst->url,
                             entry->srvtypelen,
                             entry->srvtype) == 0 )
        {
            if(SLPStringListIntersect(attrrqst->scopelistlen,
                                      attrrqst->scopelist,
                                      entry->scopelistlen,
                                      entry->scopelist))
            {
                result[found].attrlen = entry->attrlistlen;
                result[found].attr = entry->attrlist;
                found++;
                break;
            }       
        }

        entry = entry->next;

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
    int                 mypid = getpid();
    int                 myuid = getuid();
    SLPDDatabaseEntry*  entry;

    /* Remove all entries in the database if any */
    RemoveAllEntries();

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
                entry->pid              = mypid;
                entry->uid              = myuid;
                LinkEntry(entry);
            }

            fclose(fd);
        }
    }
    
    return 0;
}


