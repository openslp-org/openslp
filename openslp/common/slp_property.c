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
#include "slp_thread.h"
#include "slp_debug.h"
#include "slp_socket.h"

#define ENV_CONFFILE_VARNAME "OpenSLPConfig"

/** A property list entry structure.
 */
typedef struct _SLPProperty
{
   SLPListItem listitem;   /*!< Make the SLPProperty class list-able. */
   unsigned attrs;         /*!< Property attributes. */
   char * value;           /*!< The value of this property. Points into the name buffer. */
   char name[1];           /*!< The name/value of this property. The name is zero-terminated */
} SLPProperty;

/** The flag that tells if we've already read configuration files once. */
static bool s_PropertiesInitialized = false;

/** The property list - module static. */
static SLPList s_PropertyList = {0, 0, 0};

/** The (optional) application-specified property file - module static. */
static char s_AppPropertyFile[MAX_PATH] = "";

/** The (optional) environment-specified property file - module static. */
static char s_EnvPropertyFile[MAX_PATH] = "";

/** The (optional) global property file - module static. */
static char s_GlobalPropertyFile[MAX_PATH] = "";

/** The database lock - module static. */
static SLPMutexHandle s_PropDbLock;

/** The global MTU configuration property value. */
static int s_GlobalPropertyMTU = 1400;

/** The internl property which holds global RCVBUF size */
static int s_GlobalPropertyInternalRcvBufSize;

/** The internal property which holds global SNDBUF size */
static int s_GlobalPropertyInternalSndBufSize;

/** Sets all SLP default property values.
 *
 * @return Zero on success, or non-zero with errno set on error.
 * 
 * @internal
 */
static int SetDefaultValues(void)
{
   /* The table of default property values - comments following some values 
    * indicate deviations from the default values specified in RFC 2614.
    */
   static struct { char * name, * value; unsigned attrs; } defaults[] =
   {
   /* Section 2.1.1 DA Configuration */
      {"net.slp.isDA", "false", 0},
      {"net.slp.DAHeartBeat", "10800", 0},
      {"net.slp.DAAttributes", "", 0},

   /* Section 2.1.2 Static Scope Configuration */
      {"net.slp.useScopes", "DEFAULT", 0},
      {"net.slp.DAAddresses", "", 0},

   /* Section 2.1.3 Tracing and Logging */
      {"net.slp.traceDATraffic", "false", 0},
      {"net.slp.traceMsg", "false", 0},
      {"net.slp.traceDrop", "false", 0},
      {"net.slp.traceReg", "false", 0},
      {"net.slp.appendLog", "true", 0},

   /* Section 2.1.4 Serialized Proxy Registrations */
      {"net.slp.serializedRegURL", "", 0},

   /* Section 2.1.5 Network Configuration Properties */
      {"net.slp.isBroadcastOnly", "false", 0},
      {"net.slp.passiveDADetection", "true", 0},                           /* false */
      {"net.slp.multicastTTL", "255", 0},                                  /* 8 */
      {"net.slp.DAActiveDiscoveryInterval", "900", 0},                     /* 1 */
      {"net.slp.multicastMaximumWait", "15000"},                           /* 5000 */
      {"net.slp.multicastTimeouts", "500,750,1000,1500,2000,3000", 0},        /* 500,750,1000,1500,2000,3000 */  
      {"net.slp.DADiscoveryTimeouts", "500,750,1000,1500,2000,3000", 0},   /* 500,750,1000,1500,2000,3000 */ 
      {"net.slp.datagramTimeouts", "500,750,1000,1500,2000,3000", 0},         /* I made up these numbers */     
      {"net.slp.randomWaitBound", "1000", 0},
      {"net.slp.MTU", "1400", 0},
      {"net.slp.interfaces", "", 0},

   /* Section 2.1.6 SA Configuration */
      {"net.slp.SAAttributes", "", 0},

   /* Section 2.1.7 UA Configuration */
      {"net.slp.locale", "en", 0},
      {"net.slp.maxResults", "-1", 0},                                     /* 256 */
      {"net.slp.typeHint", "", 0},
	  {"net.slp.preferSLPv1", "false", 0},

   /* Section 2.1.8 Security */
      {"net.slp.securityEnabled", "false", 0},

   /* Additional properties that transcend RFC 2614 */
      {"net.slp.watchRegistrationPID", "true", 0},
      {"net.slp.OpenSLPVersion", SLP_VERSION, 0},
      {"net.slp.unicastMaximumWait", "5000", 0},
      {"net.slp.unicastTimeouts", "500,750,1000,1500,2000,3000", 0},
      {"net.slp.DADiscoveryMaximumWait", "5000", 0},
      {"net.slp.activeDADetection", "true", 0},
      {"net.slp.checkSourceAddr", "true", 0},
      {"net.slp.broadcastAddr", "255.255.255.255", 0},
      {"net.slp.port", "427", 0},
      {"net.slp.useDHCP", "true", 0},

   /* Additional properties that are specific to IPv6 */
      {"net.slp.useIPv6", "false", 0},
      {"net.slp.useIPv4", "true", 0},
   };

   int i;

   for (i = 0; i < sizeof(defaults)/sizeof(*defaults); i++)
      if (SLPPropertySet(defaults[i].name, defaults[i].value, 
            defaults[i].attrs) != 0)
         return -1;

   return 0;
}

/**
 * Initializes the MTU configuration property value. If the user specified
 * value is more than the value that is allowed by the kernel, MTU value is
 * adjusted to the actual value set by the kernel. If the default values of
 * SO_SNDBUF and SO_RCVBUF are greater than the global MTU value, SO_SNDBUF
 * and SO_RCVBUF are not set explicitly.
 *
 * @internal
 */
static void InitializeMTUPropertyValue()
{
#ifndef _WIN32
   int mtuChanged = 0;
   int family;
   sockfd_t sock;
   int value = 0;
   socklen_t valSize = sizeof(int);
#endif
   s_GlobalPropertyInternalRcvBufSize = s_GlobalPropertyInternalSndBufSize = 0;
   s_GlobalPropertyMTU = SLPPropertyAsInteger("net.slp.MTU");

#ifndef _WIN32
   family = SLPPropertyAsBoolean("net.slp.useIPv4") ? AF_INET : AF_INET6;

   if ((sock = socket(family, SOCK_DGRAM, 0)) != SLP_INVALID_SOCKET)
   {
      if (getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &value, &valSize) != -1)
      {
         if (value < s_GlobalPropertyMTU)
         {
            setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
                &s_GlobalPropertyMTU, sizeof(int));
            s_GlobalPropertyInternalRcvBufSize = s_GlobalPropertyMTU;
         }
      }

      if (getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &value, &valSize) != -1)
      {
         if (value < s_GlobalPropertyMTU)
         {
            setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
                &s_GlobalPropertyMTU, sizeof(int));
            s_GlobalPropertyInternalSndBufSize = s_GlobalPropertyMTU;
         }
      }

      // If the actual value set by the kernel is less than the MTU value,
      // adjust here.
      if (s_GlobalPropertyInternalRcvBufSize &&
          getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &value, &valSize) != -1)
      {
         if (value < s_GlobalPropertyMTU)
         {
            s_GlobalPropertyInternalRcvBufSize = value;
         }
      }

      if (s_GlobalPropertyInternalSndBufSize && 
          getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &value, &valSize) != -1)
      {
         if (value < s_GlobalPropertyMTU)
         {
            s_GlobalPropertyInternalSndBufSize = value;
         }
      }

      close(sock);

      // If both values are set adjust the s_GlobalPropertyMTU value.
      if (s_GlobalPropertyInternalRcvBufSize 
         && s_GlobalPropertyInternalSndBufSize)
      {
          s_GlobalPropertyMTU = s_GlobalPropertyInternalRcvBufSize;
          if (s_GlobalPropertyMTU < s_GlobalPropertyInternalSndBufSize)
          {
             s_GlobalPropertyMTU = s_GlobalPropertyInternalSndBufSize;
          }
          mtuChanged = 1;
      }
   }

   if (mtuChanged)
   {
      char tmp[13];
      snprintf(tmp, 13, "%d", s_GlobalPropertyMTU);
      SLPPropertySet("net.slp.MTU", tmp, 0);
   }
#endif
}

/** Reads a specified configuration file into non-userset properties.
 * 
 * Reads all values from a specified configuration file into the in-memory
 * database
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
            SLPPropertySet(namestart, valuestart, 0);
      }
      fclose(fp);
      retval = true;
   }
   xfree(alloced);

   return retval;
}

/** Reads the property configuration files.
 *
 * Clears all current values from the property table, and then reads and 
 * sets properties from the configuration files. All properties read from 
 * configuration files are considered mutable by the application (except for
 * the values of the specified configuration files.
 *
 * @return Zero on success, or a non-zero value on error. Properties will 
 *    be set to default on error, or if not set by one or more of the
 *    configuration files.
 * 
 * @internal
 */
static int ReadPropertyFiles(void)
{
   /* load all default values first - override later with file entries */
   if (SetDefaultValues() != 0)
      return -1;

   /* read global, and then app configuration files */
   if (*s_GlobalPropertyFile)
      if (ReadFileProperties(s_GlobalPropertyFile))
         SLPPropertySet("net.slp.OpenSLPConfigFile", 
               s_GlobalPropertyFile, SLP_PA_READONLY);

   /* read environment specified configuration file */
   if (*s_EnvPropertyFile)
      if (ReadFileProperties(s_EnvPropertyFile))
         SLPPropertySet("net.slp.EnvConfigFile", 
               s_EnvPropertyFile, SLP_PA_READONLY);

   /* if set, read application-specified configuration file */
   if (*s_AppPropertyFile)
      if (ReadFileProperties(s_AppPropertyFile))
         SLPPropertySet("net.slp.AppConfigFile", 
               s_AppPropertyFile, SLP_PA_READONLY);

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

/** Return MTU configuration property value
 * @return Returns MTU value
 *
 * @remarks MTU configuration property value is read from the configuration
 *    file if specified and initialized only once. This special function is
 *    added to access MTU configuration property value in both client and
 *    server code to avoid performance issues.
 */
int SLPPropertyGetMTU()
{
    return s_GlobalPropertyMTU;
}

/** Gets the SNDBUF and RCVBUF sizes.
 *
 * @param[in] sndBufSize - A poniter to the integer to which global SNDBUF
 *                          value is assigned
 * @param[in] sndBufSize - A poniter to the integer to which global RCVBUF
 *                          value is assigned
 */
void SLPPropertyInternalGetSndRcvBufSize(int *sndBufSize, int *rcvBufSize)
{
   SLP_ASSERT(sndBufSize);
   SLP_ASSERT(rcvBufSize);
   *sndBufSize = s_GlobalPropertyInternalSndBufSize;
   *rcvBufSize = s_GlobalPropertyInternalRcvBufSize;
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

   SLPMutexAcquire(s_PropDbLock);

   if ((property = Find(name)) != 0)
      retval = xstrdup(property->value);

   SLPMutexRelease(s_PropDbLock);

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
   if (!name || (buffer && !bufsz))
      return 0;

   if (bufszp) *bufszp = 0;

   SLPMutexAcquire(s_PropDbLock);

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

   SLPMutexRelease(s_PropDbLock);

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
 * @param[in] attrs - The attributes of this property - zero means no
 *    attributes are assigned, other values include SLP_PA_USERSET and 
 *    SLP_PA_READONLY.
 *
 * @return Zero on success; -1 on error, with errno set.
 * 
 * @remarks The @p attrs parameter contains a set of bit flags indicating
 * various attributes of the property. These attributes control write 
 * permissions mostly. SLP_PA_USERSET means that an attribute may not
 * be changed by reading a configuration file, except in a complete 
 * re-initialization scenario. SLP_PA_READONLY sounds like the same thing, 
 * but it's not. The difference is that once set, properties with the 
 * SLP_PA_READONLY attribute may NEVER be reset (again, except in a complete 
 * re-initialization scenario), while properties with the SLP_PA_USERSET 
 * attribute may only be reset by passing this same flag in @p attrs, 
 * indicating that the caller is actually a user, and so has the right
 * to reset the property value.
 */
int SLPPropertySet(char const * name, char const * value, unsigned attrs)
{
   size_t namesz, valuesz;
   SLPProperty * oldprop;
   SLPProperty * newprop = 0;    /* we may be just removing the old */
   bool update = true;           /* reset if old property exists */

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
      newprop->attrs = attrs;
      memcpy(newprop->name, name, namesz);
      newprop->value = newprop->name + namesz;
      memcpy(newprop->value, value, valuesz);
   }

   SLPMutexAcquire(s_PropDbLock);

   /* locate and possibly remove old property */
   if ((oldprop = Find(name))!= 0)
   {
      /* update ONLY if old is clean, or new and old are user-settable . */
      update = !oldprop->attrs 
            || (oldprop->attrs == SLP_PA_USERSET && attrs == SLP_PA_USERSET);
      if (update)
         SLPListUnlink(&s_PropertyList, (SLPListItem *)oldprop);
   }

   /* link in new property, if specified and old property was removed */
   if (newprop && update)
      SLPListLinkHead(&s_PropertyList, (SLPListItem *)newprop);

   SLPMutexRelease(s_PropDbLock);

   /* if old property was not removed, delete the new one instead */
   xfree(update? oldprop: newprop);

   return update? 0: ((errno = EACCES), -1);
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
   SLPProperty * property;

   SLPMutexAcquire(s_PropDbLock);

   if ((property = Find(name)) != 0)
   {
      char const * value = property->value;
      if (*value == 't' || *value == 'T' 
            || *value == 'y' || *value == 'Y' 
            || *value == '1')
         retval = true;
   }

   SLPMutexRelease(s_PropDbLock);

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
   SLPProperty * property;

   SLPMutexAcquire(s_PropDbLock);

   if ((property = Find(name)) != 0)
      ivalue = atoi(property->value);

   SLPMutexRelease(s_PropDbLock);

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
   SLPProperty * property;

   SLPMutexAcquire(s_PropDbLock);

   if ((property = Find(name)) != 0)
   {
      char const * value = property->value;
      char const * end = value + strlen(value);
      char const * slider1;
      char const * slider2;

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

   SLPMutexRelease(s_PropDbLock);

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
   {
      strnenv(s_AppPropertyFile, aconffile, sizeof(s_AppPropertyFile));
      s_AppPropertyFile[sizeof(s_AppPropertyFile)-1] = 0;
   }
   return 0;
}

/** Release all resources held by the property module.
 * 
 * Free all associated list memory, and reinitialize the global list head
 * pointer to NULL.
 * 
 * @internal
 */
static void SLPPropertyCleanup(void)
{
   SLPProperty * property;
   SLPProperty * del;

   SLPMutexAcquire(s_PropDbLock);

   property = (SLPProperty *)s_PropertyList.head;
   while (property)
   {
      del = property;
      property = (SLPProperty *)property->listitem.next;
      xfree(del);
   }
   memset(&s_PropertyList, 0, sizeof(s_PropertyList));

   SLPMutexRelease(s_PropDbLock);
}

/** Initialize (or reintialize) the property table.
 *
 * Cleanup and reinitialize the property module from configuration files.
 * Since this can happen anytime, we do the entire operation within the lock
 * to keep changing state from messing up user threads. SLP mutexes are
 * reentrant, so we can call mutex acquire from within the lock.
 * 
 * @return Zero on success, or a non-zero value on error.
 * 
 * @remarks The daemon calls this routine at SIGHUP. Thread-safe.
 */
int SLPPropertyReinit(void)
{
   int ret;
   SLPMutexAcquire(s_PropDbLock);
   SLPPropertyCleanup();
   ret = ReadPropertyFiles();
   InitializeMTUPropertyValue();
   SLPMutexRelease(s_PropDbLock);
   return ret;
}

/** Initialize (or reintialize) the property table.
 *
 * Store the global init file pathname and initialize the property module 
 * from configuration options specified in @p gconffile. 
 * 
 * @param[in] gconffile - The name of the global configuration file to read.
 *
 * @return Zero on success, or a non-zero value on error.
 * 
 * @remarks This routine is NOT reentrant, so steps should be taken by the 
 * caller to ensure that this routine is not called by multiple threads
 * simultaneously. This routine is designed to be called once by the 
 * client library and the daemon, and before other threads begin accessing
 * other property sub-system methods.
 */
int SLPPropertyInit(const char * gconffile)
{
   int sts;
   char const * econffile = getenv(ENV_CONFFILE_VARNAME);

   if (econffile)
   {
      strnenv(s_EnvPropertyFile, econffile, sizeof(s_EnvPropertyFile));
      s_EnvPropertyFile[sizeof(s_EnvPropertyFile)-1] = 0;
   }
   if (gconffile)
   {
      strnenv(s_GlobalPropertyFile, gconffile, sizeof(s_GlobalPropertyFile));
      s_GlobalPropertyFile[sizeof(s_GlobalPropertyFile)-1] = 0;
   }
   if ((s_PropDbLock = SLPMutexCreate()) == 0)
      return -1;

   if ((sts = SLPPropertyReinit()) != 0)
      SLPMutexDestroy(s_PropDbLock);
   else
      s_PropertiesInitialized = true;

   return sts;
}

/** Release all globally held resources held by the property module.
 * 
 * Free all associated property database memory, and destroy the database 
 * mutex.
 * 
 * @remarks This routine is NOT reentrant, so steps should be taken by the
 * caller to ensure that it is not called by more than one thread at a time.
 * It should also not be called while other threads are accessing the property
 * database through any of the other property sub-system access methods.
 */
void SLPPropertyExit(void)
{
   SLPPropertyCleanup();
   SLPMutexDestroy(s_PropDbLock);
   s_PropertiesInitialized = false;
}

/*===========================================================================
 *  TESTING CODE : compile with the following command lines:
 *
 *  $ gcc -g -O0 -DSLP_PROPERTY_TEST -DDEBUG -DHAVE_CONFIG_H -lpthread 
 *       -I .. slp_property.c slp_xmalloc.c slp_linkedlist.c slp_debug.c 
 *       slp_thread.c -o slp-prop-test
 *
 *  C:\> cl -Zi -DSLP_PROPERTY_TEST -DSLP_VERSION=\"2.0\" -DDEBUG 
 *       -D_CRT_SECURE_NO_DEPRECATE slp_property.c slp_xmalloc.c 
 *       slp_thread.c slp_debug.c slp_linkedlist.c
 */
#ifdef SLP_PROPERTY_TEST 

# define FAIL (printf("FAIL: %s at line %d.\n", __FILE__, __LINE__), (-1))
# define PASS (printf("PASS: Success!\n"), (0))

# define TEST_G_CFG_FILENAME "slp_property_test.global.conf"
# define TEST_A_CFG_FILENAME "slp_property_test.app.cfg"

# ifdef _WIN32
#  define unlink _unlink
# endif

int main(int argc, char * argv[])
{
   int ec, nval, ival;
   bool bval;
   int ivec[10];
   FILE * fp;
   char const * pval;

   /* create a global configuration file */
   fp = fopen(TEST_G_CFG_FILENAME, "w+");
   if (!fp)
      return FAIL;
   fputs("\n", fp);
   fputs(" \n", fp);
   fputs("# This is a comment.\n", fp);
   fputs(" # This is another comment.\n", fp);
   fputs(" \t\f# This is the last comment.\n", fp);
   fputs("\t\t   \f\tStrange Text with no equals sign\n", fp);
   fputs("net.slp.isDA=true\n", fp);         /* default value is false */
   fputs("net.slp.DAHeartBeat = 10801\n", fp);
   fputs("net.slp.DAAttributes=\n", fp);
   fputs("net.slp.useScopes =DEFAULT\n", fp);
   fputs("net.slp.DAAddresses\t\t\t=       \n", fp);
   fputs("net.slp.traceDATraffic = true\n", fp);
   fputs("net.slp.multicastTimeouts=1001,1251,1501,2001,4001\n", fp);
   fclose(fp);

   /* create an application configuration file */
   fp = fopen(TEST_A_CFG_FILENAME, "w+");
   if (!fp)
      return FAIL;
   fputs("net.slp.DAHeartBeat = 10802\n", fp);
   fclose(fp);

   /* specify app configuration file */
   ec = SLPPropertySetAppConfFile(TEST_A_CFG_FILENAME);
   if (ec != 0)
      return FAIL;

   /* specify global configuration file - initialize */
   ec = SLPPropertyInit(TEST_G_CFG_FILENAME);
   if (ec != 0)
      return FAIL;

   /* set a mutable value */
   ec = SLPPropertySet("net.slp.traceDATraffic", "false", 0);
   if (ec != 0)
      return FAIL;

   /* set a user-only settable value */
   ec = SLPPropertySet("net.slp.isDA", "false", SLP_PA_USERSET);
   if (ec != 0)
      return FAIL;

   pval = SLPPropertyGet("net.slp.traceDATraffic", 0, 0);
   if (pval == 0 || strcmp(pval, "false") != 0)
      return FAIL;

   ival = SLPPropertyAsInteger("net.slp.DAHeartBeat");
   if (ival != 10802)
      return FAIL;

   bval = SLPPropertyAsBoolean("net.slp.isDA");
   if (bval != false)
      return FAIL;

   nval = SLPPropertyAsIntegerVector("net.slp.multicastTimeouts", ivec, 10);
   if (nval != 5 || ivec[0] != 1001 || ivec[1] != 1251 || ivec[2] != 1501
         || ivec[3] != 2001 || ivec[4] != 4001)
      return FAIL;

   ival = SLPPropertyAsInteger("net.slp.fake");
   if (ival != 0)
      return FAIL;

   bval = SLPPropertyAsBoolean("net.slp.fake");
   if (bval != false)
      return FAIL;

   nval = SLPPropertyAsIntegerVector("net.slp.fake", ivec, 10);
   if (nval != 0)
      return FAIL;

   pval = SLPPropertyGet("net.slp.OpenSLPConfigFile", 0, 0);
   if (pval == 0 || strcmp(pval, TEST_G_CFG_FILENAME) != 0)
      return FAIL;

   /* reset a user-only settable value - indicate non-user is setting */
   ec = SLPPropertySet("net.slp.isDA", "true", 0);
   if (ec == 0)
      return FAIL;

   /* reset a user-only settable value - indicate user is setting */
   ec = SLPPropertySet("net.slp.isDA", "true", SLP_PA_USERSET);
   if (ec != 0)
      return FAIL;

   SLPPropertyExit();

   unlink(TEST_A_CFG_FILENAME);
   unlink(TEST_G_CFG_FILENAME);

   return PASS;
}

#endif

/*=========================================================================*/
