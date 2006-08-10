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
   bool immutable;         /*!< The value may NOT be changed by config files */
   char * value;           /*!< The value of this property. Points into the name buffer. */
   char name[1];           /*!< The name/value of this property. The name is zero-terminated */
} SLPProperty;

/** The flag that tells if we've already read configuration files once. */
static bool s_PropertiesInitialized = false;

/** The property list - module static. */
static SLPList s_PropertyList = {0, 0, 0};

/** The (optional) application-specified property file - module static. */
static s_AppPropertyFile[MAX_PATH] = "";

/** The database lock - module static. */
static intptr_t s_PropDbLock = 0;

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
   ec |= SLPPropertySet("net.slp.isDA", "false", false);
   ec |= SLPPropertySet("net.slp.DAHeartBeat", "10800", false);
   ec |= SLPPropertySet("net.slp.DAAttributes", "", false);

   /* Section 2.1.2 Static Scope Configuration */
   ec |= SLPPropertySet("net.slp.useScopes", "DEFAULT", false);                                  /* RO */
   ec |= SLPPropertySet("net.slp.DAAddresses", "", false);                                       /* RO */

   /* Section 2.1.3 Tracing and Logging */
   ec |= SLPPropertySet("net.slp.traceDATraffic", "false", false);
   ec |= SLPPropertySet("net.slp.traceMsg", "false", false);
   ec |= SLPPropertySet("net.slp.traceDrop", "false", false);
   ec |= SLPPropertySet("net.slp.traceReg", "false", false);

   /* Section 2.1.4 Serialized Proxy Registrations */
   ec |= SLPPropertySet("net.slp.serializedRegURL", "", false);

   /* Section 2.1.5 Network Configuration Properties */
   ec |= SLPPropertySet("net.slp.isBroadcastOnly", "false", false);                              /* RO */
   ec |= SLPPropertySet("net.slp.passiveDADetection", "true", false);                            /* false */
   ec |= SLPPropertySet("net.slp.multicastTTL", "255", false);                                   /* 8 */
   ec |= SLPPropertySet("net.slp.DAActiveDiscoveryInterval", "900", false);                      /* 1 */
   ec |= SLPPropertySet("net.slp.multicastMaximumWait", "15000", false);                         /* 5000 */
   ec |= SLPPropertySet("net.slp.multicastTimeouts", "1000,1250,1500,2000,4000", false);         /* 500,750,1000,1500,2000,3000 */
   ec |= SLPPropertySet("net.slp.DADiscoveryTimeouts", "2000,2000,2000,2000,3000,4000", false);  /* 500,750,1000,1500,2000,3000 */
   ec |= SLPPropertySet("net.slp.datagramTimeouts", "1000,1250,1500,2000,4000", false);          /* I made up these numbers */
   ec |= SLPPropertySet("net.slp.randomWaitBound", "1000", false);
   ec |= SLPPropertySet("net.slp.MTU", "1400", false);
   ec |= SLPPropertySet("net.slp.interfaces", "", false);

   /* Section 2.1.6 SA Configuration */
   ec |= SLPPropertySet("net.slp.SAAttributes", "", false);

   /* Section 2.1.7 UA Configuration */
   ec |= SLPPropertySet("net.slp.locale", "en", false);
   ec |= SLPPropertySet("net.slp.maxResults", "-1", false);                                      /* 256 */
   ec |= SLPPropertySet("net.slp.typeHint", "", false);

   /* Section 2.1.8 Security */
   ec |= SLPPropertySet("net.slp.securityEnabled", "false", false);

   /* Additional properties that transcend RFC 2614 */
   ec |= SLPPropertySet("net.slp.watchRegistrationPID", "true", false);
   ec |= SLPPropertySet("net.slp.OpenSLPVersion", SLP_VERSION, false);                           /* RO - I'm guessing */
   ec |= SLPPropertySet("net.slp.unicastMaximumWait", "5000", false);
   ec |= SLPPropertySet("net.slp.unicastTimeouts", "500,750,1000,1500,2000,3000", false);
   ec |= SLPPropertySet("net.slp.DADiscoveryMaximumWait", "2000", false);
   ec |= SLPPropertySet("net.slp.activeDADetection", "true", false);
   ec |= SLPPropertySet("net.slp.checkSourceAddr", "true", false);
   ec |= SLPPropertySet("net.slp.broadcastAddr", "255.255.255.255", false);

   /* Additional properties that are specific to IPv6 */
   ec |= SLPPropertySet("net.slp.useIPv6", "false", false);

   return ec;
}

/** Reads a specified configuration file into non-immutable properties.
 * 
 * @param[in] conffile - The name of the file to be read.
 * 
 * @return A Boolean value of true if the file could be opened and read, 
 *    or false if the file did not exist, or could not be opened.
 * 
 * @internal
 */
static bool ReadFileProperties(char const * conffile)
{
   FILE * fp;
   char * alloced;
   bool retval = false;

#define CONFFILE_RDBUFSZ 4096

   /* allocate a buffer and read the configuration file */
   if ((alloced = xmalloc(CONFFILE_RDBUFSZ)) == 0)
      return false;

   /* open configuration file for read - missing file returns false */
   if ((fp = fopen(conffile, "r")) != 0)
   {
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
   
         /* set the property (as mutable) */
         if (valuestart && *valuestart)
            SLPPropertySet(namestart, valuestart, false);
      }
      fclose(fp);
      retval = true;
   }
   xfree(alloced);

   return retval;
}

/** Reads the specified property configuration files.
 *
 * Clears all current values from the property table, and then reads and sets 
 * properties from the specified configuration files. All properties read from 
 * configuration files are considered mutable by the application (except for
 * the values of the specified configuration files.
 *
 * @param[in] gconffile - The path of the optional global OpenSLP 
 *    configuration file to be read.
 * @param[in] econffile - The path of the optional environment-specified 
 *    configuration file to be read.
 * @param[in] aconffile - The path of the optional application-specified
 *    configuration file to be read.
 *
 * @return Zero on success, or a non-zero value on error. Properties will 
 *    be set to default on error, or if not set by one or more of the
 *    configuration files.
 * 
 * @internal
 */
static int ReadPropertyFiles(char const * gconffile, char const * econffile, 
      char const * aconffile)
{
   /* load all default values first - override later with file entries */
   if (SetDefaultValues() != 0)
      return -1;

   /* read global, and then app configuration files */
   if (gconffile && *gconffile)
      if (ReadFileProperties(gconffile))
         SLPPropertySet("net.slp.OpenSLPConfigFile", gconffile, true);

   /* read environment specified configuration file */
   if (econffile && *econffile)
      if (ReadFileProperties(econffile))
         SLPPropertySet("net.slp.EnvConfigFile", econffile, true);

   /* if set, read application-specified configuration file */
   if (aconffile && *aconffile)
      if (ReadFileProperties(aconffile))
         SLPPropertySet("net.slp.AppConfigFile", aconffile, true);

   return 0;
}

/** Locate a property entry by name.
 *
 * Locates and returns an entry in the property list by property name.
 * 
 * @param[in] name - The property name.
 *
 * @return A pointer to the requested property entry, or NULL if the 
 *    requested property entry was not found.
 *
 * @remarks Assumes the database lock.
 * 
 * @internal
 */
static SLPProperty * Find(char const * name)
{
   SLPProperty * property = (SLPProperty *)s_PropertyList.head;

   /* property names are case sensitive, so use strcmp */
   while (property && strcmp(property->name, name))
      property = (SLPProperty *)property->listitem.next;

   return property;
}

/** Return an allocated copy of a property by name.
 * 
 * @param[in] name - The name of the property to return.
 * 
 * @return A pointer to an allocated buffer containing a copy of the property
 *    named by @p name.
 * 
 * @remarks The caller is responsible for releasing the memory returned by 
 * calling xfree on it.
 */
char * SLPPropertyXDup(const char * name)
{
   char * retval = 0;
   SLPProperty * property;

   /* parameter sanity check */
   SLP_ASSERT(name);
   if (!name)
      return 0;

   SLPAcquireSpinLock(&s_PropDbLock);

   if ((property = Find(name)) != 0)
      retval = xstrdup(property->value);

   SLPReleaseSpinLock(&s_PropDbLock);

   return retval;
}

/** Return a property by name.
 * 
 * This is the internal property access routine. If the @p buffer and @p bufszp
 * parameters are used, then this routine will return a copy of the internal
 * value string. Otherwise it returns a pointer to the internal value string.
 *
 * @param[in] name - The name of the property to return.
 * @param[out] buffer - The address of storage for the requested property 
 *    value. This parameter is optional, and may be specified as NULL.
 * @param[in/out] bufszp - On entry, contains the size of @p buffer. On exit,
 *    returns the number of bytes used or required. If @p buffer is too small
 *    then this parameter returns the number of bytes required to return all
 *    of the value. If too large, then this parameter returns the number of 
 *    bytes used in @p buffer.
 * 
 * @return A pointer to the value of the property named by @p name. If the
 *    @p buffer and @p bufszp parameters are specified, returns a pointer to
 *    @p buffer, otherwise a pointer to the internal value buffer is returned.
 * 
 * @remarks If an internal value string pointer is returned, then OpenSLP 
 * is absolved of all responsibility regarding concurrent access to the 
 * internal property database.
 * 
 * @remarks The correct way to call this routine with a @p buffer parameter
 * is to size the buffer as appropriate, or size it to zero. This routine will
 * return the required size in *bufszp. Then call it again with a @p buffer
 * parameter of the returned size. If @p bufszp is returned containing any
 * value less than or equal to the originally specified size, then the caller
 * knows that the entire value was returned in @p buffer.
 */
char const * SLPPropertyGet(char const * name, char * buffer, size_t * bufszp)
{
   SLPProperty * property;
   char const * retval = buffer;
   size_t bufsz = bufszp? *bufszp: 0;

   /* parameter sanity check */
   SLP_ASSERT(name && (bufsz || !buffer));
   if (!name || buffer && !bufsz)
      return 0;

   if (bufszp) *bufszp = 0;

   SLPAcquireSpinLock(&s_PropDbLock);

   if ((property = Find(name)) != 0)
   {
      char const * value = property->value;
      if (buffer)
      {
         size_t valsz = strlen(value);
         *bufszp = valsz;
         if (valsz > bufsz)
            valsz = bufsz;
         memcpy(buffer, value, valsz - 1);
         buffer[valsz - 1] = 0;
      }
      else
         retval = value;
   }

   SLPReleaseSpinLock(&s_PropDbLock);

   return retval;
}

/** Set a new value for a property by name.
 * 
 * If the value is NULL or empty, then simply erase the existing value and
 * return.
 *
 * @param[in] name - The name of the desired property.
 * @param[in] value - The new value to which @p name should be set or
 *    NULL if the existing value should be removed.
 * @param[in] immutable - the property may NOTE be changed by values 
 *    read from configuration files.
 *
 * @return Zero on success; -1 on error, with errno set.
 * 
 * @remarks The @p immutable parameter is actually overloaded. If it's true
 * then the value will be set regardless of the current state of the property.
 * The reason for this is that only the application can specify an immutable
 * value of true, so it should have full power to change values, even if they
 * are already marked immutable. Sorry about the logic - I know it's horrible.
 */
int SLPPropertySet(char const * name, char const * value, bool immutable)
{
   size_t namesz, valuesz;
   SLPProperty * oldprop;
   SLPProperty * newprop = 0;
   bool opimmutable = false;

   /* property names must not be null or empty */
   SLP_ASSERT(name && *name);
   if (!name || !*name)
      return -1;

   if (value)
   {
      /* allocate property entry for this new value */
      namesz = strlen(name) + 1;
      valuesz = strlen(value) + 1;
      if ((newprop = (SLPProperty*)xmalloc(
            sizeof(SLPProperty) - 1 + namesz + valuesz)) == 0)
      {
         errno = ENOMEM;
         return -1;
      }

      /* set internal pointers to trailing buffer space, copy values */
      newprop->immutable = immutable;
      memcpy(newprop->name, name, namesz);
      newprop->value = newprop->name + namesz;
      memcpy(newprop->value, value, valuesz);
   }

   SLPAcquireSpinLock(&s_PropDbLock);

   /* locate and remove old property if exists and is NOT immutable */
   if ((oldprop = Find(name))!= 0)
      if ((opimmutable = oldprop->immutable) == false || immutable == true)
         SLPListUnlink(&s_PropertyList, (SLPListItem *)oldprop);

   /* link in new property, if specified and old property is NOT immutable */
   if (newprop && (!opimmutable || immutable))
      SLPListLinkHead(&s_PropertyList, (SLPListItem *)newprop);

   SLPReleaseSpinLock(&s_PropDbLock);

   if (opimmutable && !immutable)
      oldprop = newprop;

   xfree(oldprop);   /* free one or the other - never both */

   return (opimmutable && !immutable)? -1: 0;
}

/** Converts a property name into a binary boolean value.
 *
 * Returns the value of the specified property name as a binary boolean value.
 *
 * @param[in] name - The property name of the value to be returned as boolean.
 * 
 * @return true if @p name refers to a FALSE boolean string value;
 *    false if @p name refers to a TRUE boolean string value.
 * 
 * @remarks Ensures that @p name is not a non-existent property, and that it 
 *    contains a non-NULL value. If @p name is non-existent, or refers to a
 *    null value, returns false.
 */
bool SLPPropertyAsBoolean(char const * name)
{
   bool retval = false;

   SLPAcquireSpinLock(&s_PropDbLock);

   SLPProperty * property = Find(name);
   if (property)
   {
      char const * value = property->value;
      if (*value == 't' || *value == 'T' 
            || *value == 'y' || *value == 'Y' 
            || *value == '1')
         retval = true;
   }

   SLPReleaseSpinLock(&s_PropDbLock);

   return retval;
}

/** Converts a property name into a binary integer value.
 *
 * Returns the specified property value as a binary integer value.
 *
 * @param[in] name - The name of the property whose value should be returned
 *    as an integer.
 *
 * @return An integer value of the string value associated with 
 *    @p value.
 *
 * @remarks Ensures that @p name is a true property with a string value before
 *    before attempting to evaluate it. If @p name is a non-existant property,
 *    or has a null value, returns 0.
 */
int SLPPropertyAsInteger(char const * name)
{
   int ivalue = 0;

   SLPAcquireSpinLock(&s_PropDbLock);

   SLPProperty * property = Find(name);
   if (property)
      ivalue = atoi(property->value);

   SLPReleaseSpinLock(&s_PropDbLock);

   return ivalue;
}

/** Converts a named integer vector property to a binary integer vector.
 *
 * Returns the value of the specified property as a binary integer vector.
 *
 * @param[in] name - The property name of an integer vector property.
 * @param[out] ivector - The address of storage for a vector of integers.
 * @param[in] ivectorsz - The amount of storage in @p ivector.
 *
 * @return The number of integer values returned in @p ivector, or zero in
 *    case of any sort of read or conversion error.
 *
 * @remarks The array is pre-initialized to zero so that all 
 *    un-initialized entries are zero on return.
 *
 * @remarks Ensures that @p name is not NULL and that it refers to an existing
 *    property with a non-null value before attempting to evaluate it. If 
 *    @p name is null or empty, returns 0.
 */
int SLPPropertyAsIntegerVector(char const * name, 
      int * ivector, int ivectorsz)
{
   int i = 0;

   SLPAcquireSpinLock(&s_PropDbLock);

   SLPProperty * property = Find(name);
   if (property)
   {
      char const * value = property->value;
      char * end = value + strlen(value);
      char * slider1, * slider2;

      /* clear caller's vector */
      memset(ivector, 0, sizeof(int) * ivectorsz);

      slider1 = slider2 = value;
      for (i = 0; i < ivectorsz && slider2 < end; i++)
      {
         while (*slider2 && *slider2 != ',') 
            slider2++;

         /* atoi stops converting at first non-numeric character */
         ivector[i] = atoi(slider1);
         slider2++;
         slider1 = slider2;
      }
   }

   SLPReleaseSpinLock(&s_PropDbLock);

   return i;
}

/** Configure the property sub-system to read an app conf file on init.
 * 
 * Configure the property sub-system to read an application-specific property
 * file at the time the global property file is read. The optional application 
 * configuration file settings override any settings obtained from the global 
 * configuration file.
 * 
 * @param[in] aconffile - The full path name of an application-specific 
 *    configuration file.
 * 
 * @return Zero on success, or a non-zero error code if the property 
 *    sub-system has already been initialized.
 */
int SLPPropertySetAppConfFile(const char * aconffile)
{
   if (s_PropertiesInitialized)
      return -1;

   if (aconffile)
      fnamecpy(aconffile, s_AppPropertyFile, sizeof(s_AppPropertyFile));

   return 0;
}

/** Initialize (or reintialize) the property table.
 *
 * Initialize the property module from configuration options specified in
 * @p conffile. If @p conffile is NULL, then read the values from the 
 * default file name and location. If property module has already been 
 * initialized, call SLPPropertyCleanup to release all existing resources
 * and re-initialize from the specified or default property file.
 * 
 * @param[in] gconffile - The name of the global configuration file to read.
 *    If this parameter is NULL, then use the default global configuration 
 *    file path and name.
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
int SLPPropertyInit(const char * gconffile)
{
   char * ecfptr = getenv(ENV_CONFFILE_VARNAME);
   char ecfbuf[MAX_PATH + 1];
   char * econffile = 0;

   if (ecfptr)
      fnamecpy(ecfptr, ecfbuf, sizeof(ecfbuf));

   SLPPropertyCleanup();   /* remove all existing properties */

   s_PropertiesInitialized = true;

   return ReadPropertyFiles(gconffile, econffile, s_AppPropertyFile);
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

   property = (SLPProperty *)s_PropertyList.head;
   while (property)
   {
      del = property;
      property = (SLPProperty *)property->listitem.next;
      xfree(del);
   }
   memset(&s_PropertyList, 0, sizeof(s_PropertyList));

   s_PropertiesInitialized = false;
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
 *
 * NOTE: No database lock required in this single-threaded test routine.
 */
#ifdef SLP_PROPERTY_TEST 

int main(int argc, char * argv[])
{
   int ec, nval, ival;
   bool bval;
   int ivec[10];
   FILE * fp;
   char const * pval;

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
   pval = SLPPropertyGet("net.slp.isDA", 0, 0);
   if (pval == 0 || strcmp(pval, "false") != 0)
   {
      printf("FAILURE: net.slp.isDA (1).\n");
      return -1;
   }
   pval = SLPPropertyGet("net.slp.DAHeartBeat", 0, 0);
   if (pval == 0 || strcmp(pval, "10800") != 0)
   {
      printf("FAILURE: net.slp.DAHeartBeat (1).\n");
      return -1;
   }
   pval = SLPPropertyGet("net.slp.DAAttributes", 0, 0);
   if (pval == 0 || *pval != 0)
   {
      printf("FAILURE: net.slp.DAAttributes (1).\n");
      return -1;
   }
   pval = SLPPropertyGet("net.slp.multicastTimeouts", 0, 0);
   if (pval == 0 || strcmp(pval, "1000,1250,1500,2000,4000") != 0)
   {
      printf("FAILURE: net.slp.multicastTimeouts (1).\n");
      return -1;
   }
   ival = SLPPropertyAsInteger("net.slp.DAHeartBeat");
   if (ival != 10800)
   {
      printf("FAILURE: net.slp.DAHeartBeat (2).\n");
      return -1;
   }
   bval = SLPPropertyAsBoolean("net.slp.isDA");
   if (bval != false)
   {
      printf("FAILURE: net.slp.isDA (2).\n");
      return -1;
   }
   nval = SLPPropertyAsIntegerVector("net.slp.multicastTimeouts", ivec, 10);
   if (nval != 5 || ivec[0] != 1000 || ivec[1] != 1250 || ivec[2] != 1500
         || ivec[3] != 2000 || ivec[4] != 4000)
   {
      printf("FAILURE: SLPPropertyAsIntegerVector (2).\n");
      return -1;
   }

   /* test generated config file read */
   ec = SLPPropertyInit("slp_property_test.conf");
   if (ec != 0)
   {
      printf("FAILURE: SLPPropertyInit (3).\n");
      return ec;
   }
   ival = SLPPropertyAsInteger("net.slp.DAHeartBeat");
   if (ival != 10801)
   {
      printf("FAILURE: SLPPropertyAsInteger (3).\n");
      return -1;
   }
   bval = SLPPropertyAsBoolean("net.slp.isDA");
   if (bval == false)
   {
      printf("FAILURE: SLPPropertyAsBoolean (3).\n");
      return -1;
   }
   nval = SLPPropertyAsIntegerVector("net.slp.multicastTimeouts", ivec, 10);
   if (nval != 5 || ivec[0] != 1001 || ivec[1] != 1251 || ivec[2] != 1501
         || ivec[3] != 2001 || ivec[4] != 4001)
   {
      printf("FAILURE: SLPPropertyAsIntegerVector (3).\n");
      return -1;
   }
   ival = SLPPropertyAsInteger("net.slp.fake");
   if (ival != 0)
   {
      printf("FAILURE: SLPPropertyAsInteger (4).\n");
      return -1;
   }
   bval = SLPPropertyAsBoolean("net.slp.fake");
   if (bval != false)
   {
      printf("FAILURE: SLPPropertyAsBoolean (4).\n");
      return -1;
   }
   nval = SLPPropertyAsIntegerVector("net.slp.fake", ivec, 10);
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
