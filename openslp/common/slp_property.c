/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slplib_property.c                                          */
/*                                                                         */
/* Abstract:    Implementation for SLPGetProperty() and SLPSetProperty()   */
/*                                                                         */
/* Author(s):   Matthew Peterson                                           */
/*                                                                         */
/***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <slp_property.h>

/*=========================================================================*/
/* Global Variables                                                        */
/*=========================================================================*/
SLPProperty*    G_SLPPropertyListHead  = 0;


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

    curProperty = G_SLPPropertyListHead;
    
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

    existingProperty = Find("notfound");

    return existingProperty->propertyValue;
}


/*=========================================================================*/
int SLPPropertySet(const char *pcName,
                    const char *pcValue)
/*=========================================================================*/
{
    int             pcNameSize; 
    int             pcValueSize;
    SLPProperty*    newProperty; 
    
    newProperty = Find(pcName);
    pcNameSize = strlen(pcName) + 1;
    pcValueSize = strlen(pcValue) + 1;
    
    if(newProperty == 0)
    {
        /* property does not exist in the list */
        newProperty = (SLPProperty*)malloc(sizeof(SLPProperty) + pcNameSize + pcValueSize);
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
        ListLink((PListItem*)&G_SLPPropertyListHead,
                 (PListItem)newProperty);
    }
    else
    {    
        /* property already exists in the list */
        newProperty = (SLPProperty*)realloc(newProperty,sizeof(SLPProperty) + pcNameSize + pcValueSize);    
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
    }
    
    return 0;
}


/*-------------------------------------------------------------------------*/
int SetDefaultValues()
/*-------------------------------------------------------------------------*/
{
    int result = 0;

    result |= SLPPropertySet("net.slp.isBroadcastOnly","false");
    result |= SLPPropertySet("net.slp.passiveDADetection","false");
    result |= SLPPropertySet("net.slp.activeDADetection","false");
    result |= SLPPropertySet("net.slp.locale","en");
    result |= SLPPropertySet("net.slp.multicastTimeouts","");
    result |= SLPPropertySet("net.slp.DADiscoveryTimeouts","");
    result |= SLPPropertySet("net.slp.datagramTimeouts","");
    result |= SLPPropertySet("net.slp.randomWaitBound","1000");
    result |= SLPPropertySet("net.slp.interfaces","");
    result |= SLPPropertySet("net.slp.DAAddresses","");
    result |= SLPPropertySet("net.slp.securityEnabled","false");
    result |= SLPPropertySet("net.slp.unicastMaximumWait","15000");
    result |= SLPPropertySet("net.slp.multicastMaximumWait","15000");
    result |= SLPPropertySet("net.slp.multicastTTL","255");
    result |= SLPPropertySet("net.slp.MTU","1400");
    result |= SLPPropertySet("net.slp.useScopes","DEFAULT");
    result |= SLPPropertySet("net.slp.traceMsg","false");
    result |= SLPPropertySet("net.slp.traceReg","false");
    result |= SLPPropertySet("net.slp.traceDrop","false");
    result |= SLPPropertySet("net.slp.traceDATraffic","false");
    result |= SLPPropertySet("notfound","");

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
    FILE*   fd;
    char*   namestart;
    char*   nameend;
    char*   valuestart;
    char*   valueend;

    
    if(SetDefaultValues())
    {
        return -1;
    }
    
    alloced = malloc(4096);
    if(alloced == 0)
    {
        /* out of memory */
        errno = ENOMEM;
        return -1;
    }

    fd = fopen(conffile,"r");
    if(fd == 0)
    {
        goto CLEANUP;
    }   

    while(fgets(alloced,4096,fd))
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
        while(*valuestart <= 0x20) 
    	{
            valuestart++;
    	}
    
        valueend = valuestart;
    
        while(*valueend)
    	{
    	    valueend++;
    	}
    
        while(*valueend <= 0x20)
        {
            *valueend = 0;
            valueend --;
        }
    
        /* set the property */
        SLPPropertySet(namestart, valuestart);
    }   


    CLEANUP:

    if(alloced)
    {
        free(alloced);
    }

    return 0;
}

