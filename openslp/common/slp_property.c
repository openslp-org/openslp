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

/** Implementation for SLPGetProperty and SLPSetProperty
 *
 * These routines represent the internal implementation of the property 
 * management routines. Some really creative string management needs to be
 * done to properly implement the API, as designed in RFC 2614 because the
 * SLPGetProperty routine returns reference (pointer) into the property
 * value storage. Since this is the case, SLPSetProperty can't just delete
 * old values because an outstanding reference might exist so such a value.
 *
 * @par
 * One possible solution to this problem might be to cache all old values 
 * in a table that may only be freed at the time the program terminates,
 * but this could get expensive, memory-wise. We may try this approach 
 * anyway - for the few times properties will need to be set at run-time,
 * the cost may be worth it.
 *
 * @file       slp_property.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifndef _WIN32
# ifdef HAVE_CONFIG_H
#  include "config.h"
#  define SLP_VERSION VERSION
# endif
#endif

#include "slp_property.h"
#include "slp_xmalloc.h"

/* Global Variables */ 
/** @todo Make property list non-global. */
SLPList G_SLPPropertyList = {0,0,0};

/** Locate a property by name.
 *
 * @param[in] pcName - The property name.
 *
 * @return A pointer to the requested property, or null if the requested
 *    property was not found.
 *
 * @internal
 */
SLPProperty * Find(const char * pcName)
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

/** Return a property by name.
 *
 * @param[in] pcName - The name of the property to return.
 *
 * @return A pointer to the value of the property named by @p pcName.
 */
const char * SLPPropertyGet(const char * pcName)
{
    SLPProperty* existingProperty = Find(pcName);
    if(existingProperty)
    {
        return existingProperty->propertyValue;
    }

    return 0;
}

/** Set a new value for a property by name.
 *
 * @param[in] pcName - The name of the desired property.
 * @param[in] pcValue - The new value to which @p pcName should be set.
 *
 * @return Zero on success; -1 on error, with errno set.
 */
int SLPPropertySet(const char *pcName,
                   const char *pcValue)
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

/** Sets all SLP default property values.
 *
 * @return Zero on success, or -1 with errno set on error.
 */
int SetDefaultValues()
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
    result |= SLPPropertySet("net.slp.watchRegistrationPID","true");
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
    result |= SLPPropertySet("net.slp.DAHeartBeat","10800");

    result |= SLPPropertySet("net.slp.securityEnabled","false");
    result |= SLPPropertySet("net.slp.checkSourceAddr","true");
    result |= SLPPropertySet("net.slp.OpenSLPVersion", SLP_VERSION);
    result |= SLPPropertySet("net.slp.useIPV4", "true");
    result |= SLPPropertySet("net.slp.useIPV6", "false");
    result |= SLPPropertySet("net.slp.broadcastAddr", "255.255.255.255");

    return result;
}

/** Reads the property file into the property table.
 *
 * Reads and sets properties from the specified configuration file.
 *
 * @param[in] conffile - The path of the config file to be read.
 *
 * @return Zero on success, or a non-zero value on error. Properties will 
 *    be set to default on error.
 */
int SLPPropertyReadFile(const char* conffile)
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

/** Converts a string boolean to a binary boolean.
 *
 * Returns the specified property value as a binary boolean value.
 *
 * @param[in] property - The name of a boolean property to return as 
 *    a boolean value.
 *
 * @return Zero if @p property refers to a FALSE boolean string value;
 *    Non-zero if @p property refers to a TRUE boolean string value.
 */
int SLPPropertyAsBoolean(const char* property)
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

/** Converts a string integer to a binary integer.
 *
 * Returns the specified property value as a binary integer value.
 *
 * @param[in] property - The name of an integer property to return as
 *    an integer value.
 *
 * @return An integer value of the string value associated with 
 *    @p property.
 */
int SLPPropertyAsInteger(const char* property)
{
    return atoi(property);
}

/** Converts a string integer vector to a binary integer vector.
 *
 * Returns the specified property value as a binary integer value.
 *
 * @param[in] property - The name of an integer vector property to return
 *    as a binary integer vector.
 * @param[out] vector - The address of storage for a vector of integers.
 * @param[in] vectorsize - The amount of storage in @p vector.
 *
 * @return Zero on success, or a non-zero value on error.
 *
 * @remarks The array in pre-initialized to zero so that all 
 *    un-initialized entries are zero on return.
 *
 * @todo Convert this routine to return the number of entries set on 
 *    success, rather than simply zero.
 */
int SLPPropertyAsIntegerVector(const char* property, 
                               int* vector, 
                               int vectorsize)
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
/** Free all property memory (DEBUG only).
 *
 * This routine is debug only because the amount of memory used by the 
 * property code is limited at runtime by a finite number of properties.
 */
void SLPPropertyFreeAll(void)
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

/*=========================================================================*/
