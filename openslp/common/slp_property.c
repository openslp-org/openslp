/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slplib_property.c                                          */
/*                                                                         */
/* Abstract:    Implementation for SLPGetProperty() and SLPSetProperty()   */
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "slp_property.h"
#include "slp_xmalloc.h"

/*=========================================================================*/
/* Global Variables                                                        */
/*=========================================================================*/
SLPList G_SLPPropertyList = {0,0,0};


/*-------------------------------------------------------------------------*/
SLPProperty* Find(const char* pcName)
/* Finds the property in the property list with the specified name.        */
/*                                                                         */
/* pcName   pointer to the property name                                   */
/*                                                                         */
/* Returns: pointer to the requested property or null if the requested     */
/*          property was not found.                                        */
/*-------------------------------------------------------------------------*/
{
    SLPProperty*  curProperty;

    curProperty = (SLPProperty*)G_SLPPropertyList.head;
    while(curProperty != 0)
    {
        if(strcmp(curProperty->propertyName,pcName) == 0)
        {
            break;
        }
        curProperty = (SLPProperty*)curProperty->listitem.next;
    }

    return curProperty;
}



/*=========================================================================*/
const char* SLPPropertyGet(const char* pcName)
/*=========================================================================*/
{
    SLPProperty* existingProperty = Find(pcName);
    if(existingProperty)
    {
        return existingProperty->propertyValue;
    }

    return 0;
}


/*=========================================================================*/
int SLPPropertySet(const char *pcName,
                   const char *pcValue)
/*=========================================================================*/
{
    int             pcNameSize; 
    int             pcValueSize;
    SLPProperty*    newProperty; 

    if(pcValue == 0)
    {
       /* Bail for right now */
       return 0;
    }
    
    newProperty = Find(pcName);
    pcNameSize = strlen(pcName) + 1;
    pcValueSize = strlen(pcValue) + 1;

    if(newProperty == 0)
    {
        /* property does not exist in the list */
        newProperty = (SLPProperty*)xmalloc(sizeof(SLPProperty) + pcNameSize + pcValueSize);
        if(newProperty == 0)
        {
            /* out of memory */
            errno = ENOMEM;
            return -1;
        }

        /* set the pointers in the SLPProperty structure to point to areas of    */
        /* the previously allocated block of memory                              */
        newProperty->propertyName   = ((char*)newProperty) + sizeof(SLPProperty); 
        newProperty->propertyValue  = newProperty->propertyName + pcNameSize;

        /* copy the passed in name and value */
        memcpy(newProperty->propertyName,pcName,pcNameSize);
        memcpy(newProperty->propertyValue,pcValue,pcValueSize);

        /* Link the new property into the list */
        SLPListLinkHead(&G_SLPPropertyList,(SLPListItem*)newProperty);
    }
    else
    {
        SLPListUnlink(&G_SLPPropertyList,(SLPListItem*)newProperty);

        /* property already exists in the list */
        newProperty = (SLPProperty*)xrealloc(newProperty,sizeof(SLPProperty) + pcNameSize + pcValueSize);    
        if(newProperty == 0)
        {
            /* out of memory */
            errno = ENOMEM;
            return -1;
        }

        /* set the pointers in the SLPProperty structure to point to areas of    */
        /* the previously allocated block of memory                              */
        newProperty->propertyName   = ((char*)newProperty) + sizeof(SLPProperty); 
        newProperty->propertyValue  = newProperty->propertyName + pcNameSize;

        /* copy the passed in name and value */
        memcpy(newProperty->propertyName,pcName,pcNameSize);
        memcpy(newProperty->propertyValue,pcValue,pcValueSize);

        SLPListLinkHead(&G_SLPPropertyList,(SLPListItem*)newProperty);
    }

    return 0;
}


/*-------------------------------------------------------------------------*/
int SetDefaultValues()
/*-------------------------------------------------------------------------*/
{
    int result = 0;                                

    result |= SLPPropertySet("net.slp.isBroadcastOnly","false");
    result |= SLPPropertySet("net.slp.multicastTimeouts","500,750,1000,1500,2000,3000");
    result |= SLPPropertySet("net.slp.multicastMaximumWait","5000");
    result |= SLPPropertySet("net.slp.unicastTimeouts","500,750,1000,1500,2000,3000");
    result |= SLPPropertySet("net.slp.unicastMaximumWait","5000");
    result |= SLPPropertySet("net.slp.datagramTimeouts","");
    result |= SLPPropertySet("net.slp.maxResults","256");
    result |= SLPPropertySet("net.slp.DADiscoveryTimeouts","500,750,1000,1500,2000,3000");
    result |= SLPPropertySet("net.slp.DADiscoveryMaximumWait","2000");
    result |= SLPPropertySet("net.slp.DAActiveDiscoveryInterval","1");
    result |= SLPPropertySet("net.slp.DAAddresses","");
    result |= SLPPropertySet("net.slp.activeDADetection","true");
    result |= SLPPropertySet("net.slp.passiveDADetection","true");
    result |= SLPPropertySet("net.slp.useScopes","default");
    result |= SLPPropertySet("net.slp.locale","en");
    result |= SLPPropertySet("net.slp.randomWaitBound","5000");
    result |= SLPPropertySet("net.slp.interfaces","");
    result |= SLPPropertySet("net.slp.securityEnabled","false");
    result |= SLPPropertySet("net.slp.multicastTTL","8");
    result |= SLPPropertySet("net.slp.MTU","1400");
    result |= SLPPropertySet("net.slp.traceMsg","false");
    result |= SLPPropertySet("net.slp.traceReg","false");
    result |= SLPPropertySet("net.slp.traceDrop","false");
    result |= SLPPropertySet("net.slp.traceDATraffic","false");
    result |= SLPPropertySet("net.slp.isDA","false");
    result |= SLPPropertySet("net.slp.securityEnabled","false");
    result |= SLPPropertySet("net.slp.checkSourceAddr","true");
#ifdef WIN32
    result |= SLPPropertySet("net.slp.OpenSLPVersion", SLP_VERSION);
#else /* UNIX */
    result |= SLPPropertySet("net.slp.OpenSLPVersion", VERSION);
#endif

    return result;
}


/*=========================================================================*/
int SLPPropertyReadFile(const char* conffile)
/* Reads and sets properties from the specified configuration file         */
/*                                                                         */
/* conffile     (IN) the path of the config file to read.                  */
/*                                                                         */
/* Returns  -   zero on success. non-zero on error.  Properties will be set*/
/*              to default on error.                                       */
/*=========================================================================*/
{
    char*   line;
    char*   alloced;
    FILE*   fp;
    char*   namestart;
    char*   nameend;
    char*   valuestart;
    char*   valueend; 

    if(SetDefaultValues())
    {
        return -1;
    }

    alloced = xmalloc(4096);
    if(alloced == 0)
    {
        /* out of memory */
        errno = ENOMEM;
        return -1;
    }

    fp = fopen(conffile,"r");
    if(!fp)
    {
        goto CLEANUP;
    }

    /* Set the property that keeps track of conffile */
    SLPPropertySet("net.slp.OpenSLPConfigFile",conffile);

    while(fgets(alloced,4096,fp))
    {
        line = alloced;

        /* trim whitespace */
        while(*line && *line <= 0x20)
        {
            line++;
        }

        if(*line == 0)
        {
            continue;
        }

        /* skip commented lines */
        if(*line == '#' || *line == ';')
        {
            continue;
        }

        /* parse out the property name*/
        namestart = line;
        nameend = line;
        nameend = strchr(nameend,'=');

        if(nameend == 0)
        {
            continue;
        }
        valuestart = nameend + 1;  /* start of value for later*/

        while(*nameend <= 0x20 || *nameend == '=')
        {
            *nameend = 0;
            nameend --;
        }

        /* parse out the property value */
        while(*valuestart && *valuestart <= 0x20)
        {
            valuestart++;
        }

        valueend = valuestart;

        /* Seek to the end of the value */
        while(*valueend)
        {
            valueend++;
        }
        
        /* Remove any whitespace that might be present */
        while(valueend != valuestart && *valueend <= 0x20)
        {
            *valueend = 0;
            valueend --;
        }

        /* set the property */
        if(valuestart && *valuestart)
        {
            SLPPropertySet(namestart, valuestart);
        }
    }   


    CLEANUP:
    if(fp)
    {
        fclose(fp);
    }

    if(alloced)
    {
        xfree(alloced);
    }

    return 0;
}

/*=========================================================================*/
int SLPPropertyAsBoolean(const char* property)
/*=========================================================================*/
{
    if(property)
    {
        if(*property == 't' ||
           *property == 'T' ||
           *property == 'y' ||
           *property == 'Y' ||
           *property == '1')
        {
            return 1;
        }
    }

    return 0;
}

/*=========================================================================*/
int SLPPropertyAsInteger(const char* property)
/*=========================================================================*/
{
    return atoi(property);
}


/*=========================================================================*/
int SLPPropertyAsIntegerVector(const char* property, 
                               int* vector, 
                               int vectorsize)
/*=========================================================================*/
{
    int         i;
    char*       slider1;
    char*       slider2;
    char*       temp;
    char*       end;

    memset(vector,0,sizeof(int)*vectorsize);
    temp = xstrdup(property);
    if(temp == 0)
    {
        return 0;
    }

    end = temp + strlen(property);
    slider1 = slider2 = temp;

    for(i=0;i<vectorsize;i++)
    {
        while(*slider2 && *slider2 != ',') slider2++;
        *slider2 = 0;
        vector[i] = SLPPropertyAsInteger(slider1);
        slider2++;
        if(slider2 >= end)
        {
            break;
        }
        slider1 = slider2;
    }

    xfree(temp);

    return i;
}

#ifdef DEBUG

/*=========================================================================*/
void SLPPropertyFreeAll()
/*=========================================================================*/
{
    SLPProperty* property;
    SLPProperty* del;


    property = (SLPProperty*)G_SLPPropertyList.head;
    while(property)
    {
        del = property;
        property = (SLPProperty*)property->listitem.next;
        xfree(del);
    }

    memset(&G_SLPPropertyList,0,sizeof(G_SLPPropertyList));
}

#endif
