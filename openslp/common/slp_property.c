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
 * @author     John Calcote (jcalcote@novell.com), Matthew Peterson
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeProp
 */

#include "slp_types.h"
#include "slp_property.h"
#include "slp_xmalloc.h"
#include "slp_linkedlist.h"
#include "slp_debug.h"

/** A property list entry structure.
 */
typedef struct _SLPProperty
{
   SLPListItem listitem;   /*!< Make the SLPProperty class list-able. */
   char * propertyName;    /*!< The name of this property. */
   char * propertyValue;   /*!< The value of this property. */
} SLPProperty;

/** The property list - module static 
 */
static SLPList SLPPropertyList = {0, 0, 0};

/** Return the default configuration file name.
 *
 * If there is no current configuration file - that is, if the property
 * module has never been initialized - then the default configuration
 * file name is returned.
 * 
 * @param[out] fname - The address of storage in which to return the name of
 *    the current configuration file.
 * 
 * @param[in] fnsize - the maximum size of the @p fname buffer.
 *
 * @internal
 */
static void GetDefaultConfigFile(char * fname, size_t fnsize)
{
   const char * curfile = SLPPropertyGet("net.slp.OpenSLPConfigFile");
   if (curfile)
      strncpy(fname, curfile, fnsize);
   else
   {
#ifdef _WIN32
      ExpandEnvironmentStrings((LPCTSTR)LIBSLP_CONFFILE, 
            (LPTSTR)fname, (DWORD)fnsize - 1);
#else
      strncpy(fname, LIBSLP_CONFFILE, fnsize - 1);
#endif
   }
   fname[fnsize - 1] = 0;
}

/** Sets all SLP default property values.
 *
 * @return Zero on success, or non-zero with errno set on error.
 * 
 * @internal
 */
static int SetDefaultValues(void)
{
   int ec = 0;                                

   /* Section 2.1.1 DA Configuration */
   ec |= SLPPropertySet("net.slp.isDA", "false");
   ec |= SLPPropertySet("net.slp.DAHeartBeat", "10800");
   ec |= SLPPropertySet("net.slp.DAAttributes", "");

   /* Section 2.1.2 Static Scope Configuration */
   ec |= SLPPropertySet("net.slp.useScopes", "DEFAULT");                                  /* RO */
   ec |= SLPPropertySet("net.slp.DAAddresses", "");                                       /* RO */

   /* Section 2.1.3 Tracing and Logging */
   ec |= SLPPropertySet("net.slp.traceDATraffic", "false");
   ec |= SLPPropertySet("net.slp.traceMsg", "false");
   ec |= SLPPropertySet("net.slp.traceDrop", "false");
   ec |= SLPPropertySet("net.slp.traceReg", "false");

   /* Section 2.1.4 Serialized Proxy Registrations */
   ec |= SLPPropertySet("net.slp.serializedRegURL", "");

   /* Section 2.1.5 Network Configuration Properties */
   ec |= SLPPropertySet("net.slp.isBroadcastOnly", "false");                              /* RO */
   ec |= SLPPropertySet("net.slp.passiveDADetection", "true");                            /* false */
   ec |= SLPPropertySet("net.slp.multicastTTL", "255");                                   /* 8 */
   ec |= SLPPropertySet("net.slp.DAActiveDiscoveryInterval", "900");                      /* 1 */
   ec |= SLPPropertySet("net.slp.multicastMaximumWait", "15000");                         /* 5000 */
   ec |= SLPPropertySet("net.slp.multicastTimeouts", "1000,1250,1500,2000,4000");         /* 500,750,1000,1500,2000,3000 */
   ec |= SLPPropertySet("net.slp.DADiscoveryTimeouts", "2000,2000,2000,2000,3000,4000");  /* 500,750,1000,1500,2000,3000 */
   ec |= SLPPropertySet("net.slp.datagramTimeouts", "1000,1250,1500,2000,4000");          /* I made up these numbers */
   ec |= SLPPropertySet("net.slp.randomWaitBound", "1000");
   ec |= SLPPropertySet("net.slp.MTU", "1400");
   ec |= SLPPropertySet("net.slp.interfaces", "");

   /* Section 2.1.6 SA Configuration */
   ec |= SLPPropertySet("net.slp.SAAttributes", "");

   /* Section 2.1.7 UA Configuration */
   ec |= SLPPropertySet("net.slp.locale", "en");
   ec |= SLPPropertySet("net.slp.maxResults", "-1");                                      /* 256 */
   ec |= SLPPropertySet("net.slp.typeHint", "");

   /* Section 2.1.8 Security */
   ec |= SLPPropertySet("net.slp.securityEnabled", "false");

   /* Additional properties that transcend RFC 2614 */
   ec |= SLPPropertySet("net.slp.watchRegistrationPID", "true");
   ec |= SLPPropertySet("net.slp.OpenSLPVersion", SLP_VERSION);                           /* RO - I'm guessing */
   ec |= SLPPropertySet("net.slp.unicastMaximumWait", "5000");
   ec |= SLPPropertySet("net.slp.unicastTimeouts", "500,750,1000,1500,2000,3000");
   ec |= SLPPropertySet("net.slp.DADiscoveryMaximumWait", "2000");
   ec |= SLPPropertySet("net.slp.activeDADetection", "true");
   ec |= SLPPropertySet("net.slp.checkSourceAddr", "true");
   ec |= SLPPropertySet("net.slp.broadcastAddr", "255.255.255.255");

   /* Additional properties that are specific to IPv6 */
   ec |= SLPPropertySet("net.slp.useIPv6", "false");

   return ec;
}

/** Reads the property file into the property table.
 *
 * Reads and sets properties from the specified configuration file.
 *
 * @param[in] conffile - The path of the config file to be read.
 *
 * @return Zero on success, or a non-zero value on error. Properties will 
 *    be set to default on error.
 * 
 * @internal
 */
static int ReadPropertyFile(const char * conffile)
{
   FILE * fp;
   char * alloced;

   /* load all default values first - over ride later with file entries */
   if (SetDefaultValues() != 0)
      return -1;

#define CONFFILE_RDBUFSZ 4096

   /* allocate a buffer and read the configuration file */
   alloced = xmalloc(CONFFILE_RDBUFSZ);
   if (alloced == 0) 
   {
      errno = ENOMEM;
      return -1;
   }

   /* open configuration file for read - missing file is not an error */
   fp = fopen(conffile, "r");
   if (fp != 0)
   {
      /* set the current configuration file name property */
      SLPPropertySet("net.slp.OpenSLPConfigFile", conffile);

      /* read a line at a time - max 4k characters per line */
      while (fgets(alloced, CONFFILE_RDBUFSZ, fp))
      {
         char * line = alloced;
         char * namestart;
         char * nameend;
         char * valuestart;
         char * valueend; 

         /* trim leading white space */
         while (*line && *line <= 0x20)
            line++;

         if (*line == 0)
            continue;

         /* skip all comments: # or ; to EOL */
         if (*line == '#' || *line == ';')
            continue;

         /* parse out the property name */
         namestart = line;

         /* ignore lines not containing '=' */
         nameend = strchr(namestart, '=');
         if (nameend == 0)
            continue;

         /* capture value start for later */
         valuestart = nameend + 1;

         /* trim trailing white space from name */
         *nameend-- = 0;
         while (*nameend <= 0x20)
            *nameend-- = 0;

         /* parse out the property value - trim leading white space */
         while (*valuestart && *valuestart <= 0x20)
            valuestart++;

         /* find end of value, trim trailing white space */
         valueend = valuestart + strlen(valuestart);
         while (valueend != valuestart && *valueend <= 0x20)
            *valueend-- = 0;

         /* set the property */
         if (valuestart && *valuestart)
            SLPPropertySet(namestart, valuestart);
      }
      fclose(fp);
   }
   xfree(alloced);

   return 0;
}

/** Locate a property entry by name.
 *
 * Locates an entry in the property list. If the list has not yet been
 * initialized, reads the property file first in a thread-safe manner.
 * 
 * @param[in] name - The property name.
 *
 * @return A pointer to the requested property entry, or NULL if the 
 *    requested property entry was not found.
 *
 * @internal
 */
static SLPProperty * Find(const char * name)
{
   SLPProperty * curProperty = (SLPProperty *)SLPPropertyList.head;

   while (curProperty != 0)
   {
      /* property names are case sensitive */
      if (strcmp(curProperty->propertyName, name) == 0)
         break;
      curProperty = (SLPProperty *)curProperty->listitem.next;
   }
   return curProperty;
}

/** Return a property by name.
 *
 * @param[in] name - The name of the property to return.
 *
 * @return A pointer to the value of the property named by @p name.
 */
const char * SLPPropertyGet(const char * name)
{
   SLPProperty * existingProperty = Find(name);

   if (existingProperty)
      return existingProperty->propertyValue;

   return 0;
}

/** Set a new value for a property by name.
 * 
 * If the value is NULL or empty, then simply erase the existing value and
 * return.
 *
 * @param[in] name - The name of the desired property.
 * @param[in] value - The new value to which @p name should be set or
 *    NULL if the existing value should be removed.
 *
 * @return Zero on success; -1 on error, with errno set.
 */
int SLPPropertySet(const char * name, const char * value)
{
   size_t namesz, valuesz; 
   SLPProperty * newprop; 
   SLPProperty * oldprop;

   /* property names must not be null or empty */
   SLP_ASSERT(name && *name);
   if (!name || !*name)
      return -1;

   if (value == 0)
      return 0;   /* Bail for right now */

   /* allocate property entry for this new value */
   namesz = strlen(name) + 1;
   valuesz = strlen(value) + 1;
   newprop = (SLPProperty*)xmalloc(sizeof(SLPProperty) + namesz + valuesz);
   if (newprop == 0)
   {
      errno = ENOMEM;   /* out of memory */
      return -1;
   }

   /* set internal pointers to trailing buffer space, copy values */
   newprop->propertyName = (char *)newprop + sizeof(SLPProperty); 
   memcpy(newprop->propertyName, name, namesz);
   newprop->propertyValue = newprop->propertyName + namesz;
   memcpy(newprop->propertyValue, value, valuesz);

   /* locate existing property by name, if any */
   oldprop = Find(name);
   if (oldprop != 0)
   {
      /* remove the old property and delete it */
      SLPListUnlink(&SLPPropertyList, (SLPListItem *)oldprop);
      xfree(oldprop);
   }

   /* link the new property into the list */
   SLPListLinkHead(&SLPPropertyList, (SLPListItem *)newprop);

   return 0;
}

/** Converts a string boolean to a binary boolean.
 *
 * Returns the specified property value as a binary boolean value.
 *
 * @param[in] value - The value string of a boolean property.
 * 
 * @return true if @p value refers to a FALSE boolean string value;
 *    false if @p value refers to a TRUE boolean string value.
 * 
 * @remarks Ensures that @p value is not NULL before attempting to 
 *    evaluate it. If @p value is NULL, returns false.
 */
bool SLPPropertyAsBoolean(const char * value)
{
   if (value && (*value == 't' || *value == 'T' 
         || *value == 'y' || *value == 'Y' || *value == '1'))
      return true;
   return false;
}

/** Converts a string integer to a binary integer.
 *
 * Returns the specified property value as a binary integer value.
 *
 * @param[in] value - The value string of an integer property.
 *
 * @return An integer value of the string value associated with 
 *    @p value.
 *
 * @remarks Ensures that @p value is not NULL before attempting to 
 *    evaluate it. If @p value is NULL, returns 0.
 */
int SLPPropertyAsInteger(const char * value)
{
   return value? atoi(value): 0;
}

/** Converts a string integer vector to a binary integer vector.
 *
 * Returns the specified property value as a binary integer value.
 *
 * @param[in] value - The value string of an integer vector property.
 * @param[out] ivector - The address of storage for a vector of integers.
 * @param[in] ivectorsz - The amount of storage in @p ivector.
 *
 * @return The number of integer values returned in @p ivector.
 *
 * @remarks The array in pre-initialized to zero so that all 
 *    un-initialized entries are zero on return.
 *
 * @remarks Ensures that @p value is not NULL before attempting to 
 *    evaluate it. If @p value is NULL, returns 0.
 */
int SLPPropertyAsIntegerVector(const char * value, 
      int * ivector, int ivectorsz)
{
   int i;
   char * slider1;
   char * slider2;
   char * temp;
   char * end;

   memset(ivector, 0, sizeof(int) * ivectorsz);
   if (!value) 
      return 0;

   temp = xstrdup(value);
   if (temp == 0)
      return 0;

   end = temp + strlen(value);
   slider1 = slider2 = temp;

   for(i = 0; i < ivectorsz && slider2 < end; i++)
   {
      while (*slider2 && *slider2 != ',') 
         slider2++;
      *slider2 = 0;
      ivector[i] = SLPPropertyAsInteger(slider1);
      slider2++;
      slider1 = slider2;
   }
   xfree(temp);
   return i;
}

/** Initialize (or reintialize) the property table.
 *
 * Initialize the property module from configuration options specified in
 * @p conffile. If @p conffile is NULL, then read the values from the 
 * default file name and location. If property module has already been 
 * initialized, call SLPPropertyCleanup to release all existing resources
 * and re-initialize from the specified or default property file.
 * 
 * @param[in] conffile - The name of the configuration file to read. If
 *    this parameter is NULL, then use the default configuration file path
 *    and name.
 *
 * @return Zero on success, or a non-zero value on error.
 * 
 * @remarks This routine is NOT reentrant, so steps should be taken by the 
 *    caller to ensure that this routine is not called by multiple threads
 *    simultaneously. This routine is designed to be called by the client
 *    library and the daemon. The daemon calls it at startup and at SIGHUP.
 *    The client library calls it the first time any library property call
 *    is made by the library consumer.
 */
int SLPPropertyInit(const char * conffile)
{
   char fname[MAX_PATH]; 

   SLPPropertyCleanup();
   if (conffile == 0)
   {
      GetDefaultConfigFile(fname, sizeof(fname));
      conffile = fname;
   }
   return ReadPropertyFile(conffile);
}

/** Release all resources held by the property module.
 * 
 * Free all associated list memory, and reinitialize the global list head
 * pointer to NULL.
 */
void SLPPropertyCleanup(void)
{
   SLPProperty * property;
   SLPProperty * del;

   property = (SLPProperty *)SLPPropertyList.head;
   while (property)
   {
      del = property;
      property = (SLPProperty *)property->listitem.next;
      xfree(del);
   }
   memset(&SLPPropertyList, 0, sizeof(SLPPropertyList));
}

/*===========================================================================
 *  TESTING CODE : compile with the following command lines:
 *
 *  $ gcc -g -DSLP_PROPERTY_TEST -DLIBSLP_CONFFILE=\"test.conf\" 
 *       -DSLP_VERSION=\"2.0\" -DDEBUG slp_property.c slp_xmalloc.c 
 *       slp_linkedlist.c slp_debug.c
 *
 *  C:\> cl -Zi -DSLP_PROPERTY_TEST -DLIBSLP_CONFFILE=\"test.conf\"
 *       -DSLP_VERSION=\"2.0\" -DDEBUG slp_property.c slp_xmalloc.c 
 *       slp_linkedlist.c slp_debug.c
 */
#ifdef SLP_PROPERTY_TEST 

int main(int argc, char * argv[])
{
   int ec, nval, ival;
   bool bval;
   int ivec[10];
   FILE * fp;
   const char * pval;

   /* create a configuration file */
   fp = fopen("slp_property_test.conf", "w+");
   if (!fp)
      return -1;
   fputs("\n", fp);
   fputs(" \n", fp);
   fputs("# This is a comment.\n", fp);
   fputs(" # This is another comment.\n", fp);
   fputs(" \t\f# This is the last comment.\n", fp);
   fputs("\t\t   \f\tStrange Text with no equals sign\n", fp);
   fputs("net.slp.isDA=true\n", fp); /* default false */
   fputs("net.slp.DAHeartBeat = 10801\n", fp);
   fputs("net.slp.DAAttributes=\n", fp);
   fputs("net.slp.useScopes =DEFAULT\n", fp);
   fputs("net.slp.DAAddresses\t\t\t=       \n", fp);
   fputs("net.slp.traceDATraffic = true\n", fp);
   fputs("net.slp.multicastTimeouts=1001,1251,1501,2001,4001\n", fp);
   fclose(fp);

   /* test default config file read */
   ec = SLPPropertyInit(0);
   if (ec != 0)
   {
      printf("FAILURE: SLPPropertyInit (1).\n");
      return ec;
   }
   pval = SLPPropertyGet("net.slp.isDA");
   if (pval == 0 || strcmp(pval, "false") != 0)
   {
      printf("FAILURE: net.slp.isDA (1).\n");
      return -1;
   }
   pval = SLPPropertyGet("net.slp.DAHeartBeat");
   if (pval == 0 || strcmp(pval, "10800") != 0)
   {
      printf("FAILURE: net.slp.DAHeartBeat (1).\n");
      return -1;
   }
   pval = SLPPropertyGet("net.slp.DAAttributes");
   if (pval == 0 || *pval != 0)
   {
      printf("FAILURE: net.slp.DAAttributes (1).\n");
      return -1;
   }
   pval = SLPPropertyGet("net.slp.multicastTimeouts");
   if (pval == 0 || strcmp(pval, "1000,1250,1500,2000,4000") != 0)
   {
      printf("FAILURE: net.slp.multicastTimeouts (1).\n");
      return -1;
   }
   ival = SLPPropertyAsInteger(SLPPropertyGet("net.slp.DAHeartBeat"));
   if (ival != 10800)
   {
      printf("FAILURE: net.slp.DAHeartBeat (2).\n");
      return -1;
   }
   bval = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.isDA"));
   if (bval != false)
   {
      printf("FAILURE: net.slp.isDA (2).\n");
      return -1;
   }
   nval = SLPPropertyAsIntegerVector(SLPPropertyGet("net.slp.multicastTimeouts"), ivec, 10);
   if (nval != 5 || ivec[0] != 1000 || ivec[1] != 1250 || ivec[2] != 1500
         || ivec[3] != 2000 || ivec[4] != 4000)
   {
      printf("FAILURE: net.slp.multicastTimeouts (2).\n");
      return -1;
   }

   /* test generated config file read */
   ec = SLPPropertyInit("slp_property_test.conf");
   if (ec != 0)
   {
      printf("FAILURE: SLPPropertyInit (3).\n");
      return ec;
   }
   ival = SLPPropertyAsInteger(SLPPropertyGet("net.slp.DAHeartBeat"));
   if (ival != 10801)
   {
      printf("FAILURE: SLPPropertyAsInteger (3).\n");
      return -1;
   }
   bval = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.isDA"));
   if (bval == false)
   {
      printf("FAILURE: SLPPropertyAsBoolean (3).\n");
      return -1;
   }
   nval = SLPPropertyAsIntegerVector(SLPPropertyGet("net.slp.multicastTimeouts"), ivec, 10);
   if (nval != 5 || ivec[0] != 1001 || ivec[1] != 1251 || ivec[2] != 1501
         || ivec[3] != 2001 || ivec[4] != 4001)
   {
      printf("FAILURE: SLPPropertyAsIntegerVector (3).\n");
      return -1;
   }
   ival = SLPPropertyAsInteger(SLPPropertyGet("net.slp.fake"));
   if (ival != 0)
   {
      printf("FAILURE: SLPPropertyAsInteger (4).\n");
      return -1;
   }
   bval = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.fake"));
   if (bval != false)
   {
      printf("FAILURE: SLPPropertyAsBoolean (4).\n");
      return -1;
   }
   nval = SLPPropertyAsIntegerVector(SLPPropertyGet("net.slp.fake"), ivec, 10);
   if (nval != 0)
   {
      printf("FAILURE: SLPPropertyAsIntegerVector (4).\n");
      return -1;
   }
   SLPPropertyCleanup();

   remove("slp_property_test.conf");

   printf("Success!\n");

   return 0;
}
#endif

/*=========================================================================*/
