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

/** @mainpage OpenSLP - An API for Service Location
 * 
 * The OpenSLP project was started by Caldera in an attempt to provide a 
 * clean solid implementation of RFC 2614 "An API for Service Location". 
 * This API is based on RFC 2608 "Service Location Protocol, Version 2".
 * 
 * Since its inception, OpenSLP has been compiled and used effectively on 
 * many platforms, including: most Linux distributions, Solaris, Windows,
 * HPUX, Mac Classic and OSX, BeOS, FreeBSD, and many others. The primary
 * reason for this prolification is that there is no standardized, secure,
 * bandwidth-efficient, robust, service advertising protocol available.
 * 
 * In the last few years, other advertising protocols have been invented.
 * Some of them have become quite popular. Apple's Bonjour, for instance,
 * is really doing well in the zero-conf world. Directories based on LDAP
 * such as Novell's eDirectory, OpenLDAP, and iPlanet have been configured
 * to provide some service location infrastructure. The fact remains however,
 * that no significant solution has ever been created to solve the problems
 * that SLP was designed to solve.
 * 
 * The current version of OpenSLP supports SLPv2 security features, 
 * asynchronous multi-threaded execution, IP version 6 support, and many 
 * other features that were missing or not entirely implemented in earlier 
 * releases of the project.
 * 
 * -- John Calcote, Sr. Software Engineer, Novell, Inc. (jcalcote@novell.com)
 */

/** Main header file for the SLP API exactly as described by RFC 2614.
 *
 * This is the only file that needs to be included in order make all 
 * SLP API declarations.
 *
 * @file       slp.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpUAInterface
 */

#ifndef SLP_H_INCLUDED
#define SLP_H_INCLUDED

/*!@defgroup SlpUAInterface OpenSLP API 
 * @ingroup LibSLPCode
 * @{
 */

#ifdef __cplusplus
extern "C"
{
#endif

/** Public interface attributes
 *
 * Note that for Win32 MSVC builds, you must define LIBSLP_STATIC in 
 * your compilation environment if you intend to link against the static 
 * version of the Win32 SLP library. The default action is to link against 
 * the dynamic import library. This macro changes the signature of the 
 * library imports in your code, so not using it properly will cause 
 * "missing symbol" linker errors.
 */
#if defined(_WIN32)
# if defined(_MSC_VER)
#  if defined(LIBSLP_STATIC)     /* User must define to link statically. */
#   define SLPEXP
#  elif defined(LIBSLP_EXPORTS)
#   define SLPEXP    __declspec(dllexport)
#  else
#   define SLPEXP    __declspec(dllimport)
#  endif
# else   /* ! _MSC_VER */
#  define SLPEXP
# endif
# define SLPCALLBACK __cdecl
# define SLPAPI      __cdecl
#else    /* ! _WIN32 */
# define SLPCALLBACK
# define SLPEXP
# define SLPAPI
#endif

/** URL Lifetimes (4.1.1)     NOTE: Section numbers refer to [RFC 2614].
 *
 * The SLPURLLifetime enum type contains URL lifetime values, in
 * seconds, that are frequently used. SLP_LIFETIME_DEFAULT is 3 hours,
 * while SLP_LIFETIME_MAXIMUM is about 18 hours and corresponds to the
 * maximum size of the lifetime field in SLP messages.
 */
typedef enum {
   SLP_LIFETIME_DEFAULT = 10800,    /*!< 3 hours    */
   SLP_LIFETIME_MAXIMUM = 65535     /*!< 18.2 hours */
} SLPURLLifetime;

/** Error Semantics - Error type (3.9)
 */
typedef enum {

   SLP_LAST_CALL                    = 1,
   /*!< Passed to callback functions when the API library has no more data 
    * for them and therefore no further calls will be made to the callback 
    * on the currently outstanding operation. The callback can use this to 
    * signal the main body of the client code that no more data will be 
    * forthcoming on the operation, so that the main body of the client code 
    * can break out of data collection loops. On the last call of a callback 
    * during both a synchronous and asynchronous call, the error code 
    * parameter has value SLP_LAST_CALL, and the other parameters are all 
    * NULL. If no results are returned by an API operation, then only one 
    * call is made, with the error parameter set to SLP_LAST_CALL.
    */

   SLP_OK                           = 0,
   /*!< Indicates that the no error occurred during the operation. */

   SLP_LANGUAGE_NOT_SUPPORTED       = -1,
   /*!< No DA or SA has service advertisement or attribute information in 
    * the language requested, but at least one DA or SA indicated, via the 
    * LANGUAGE_NOT_SUPPORTED error code, that it might have information for 
    * that service in another language.
    */

   SLP_PARSE_ERROR                  = -2,
   /*!< The SLP message was rejected by a remote SLP agent. The API returns 
    * this error only when no information was retrieved, and at least one SA 
    * or DA indicated a protocol error. The data supplied through the API 
    * may be malformed or a may have been damaged in transit.
    */

   SLP_INVALID_REGISTRATION         = -3,
   /*!< The API may return this error if an attempt to register a service 
    * was rejected by all DAs because of a malformed URL or attributes. SLP 
    * does not return the error if at least one DA accepted the registration.
    */

   SLP_SCOPE_NOT_SUPPORTED          = -4,
   /*!< The API returns this error if the SA has been configured with 
    * net.slp.useScopes value-list of scopes and the SA request did not 
    * specify one or more of these allowable scopes, and no others. It may 
    * be returned by a DA or SA if the scope included in a request is not 
    * supported by the DA or SA.
    */

   SLP_AUTHENTICATION_ABSENT        = -6,
   /*!< If the SLP framework supports authentication, this error arises 
    * when the UA or SA failed to send an authenticator for requests or 
    * registrations in a protected scope.
    */

   SLP_AUTHENTICATION_FAILED        = -7,
   /*!< If the SLP framework supports authentication, this error arises when 
    * a authentication on an SLP message failed.
    */

   SLP_INVALID_UPDATE               = -13,
   /*!< An update for a non-existing registration was issued, or the update 
    * includes a service type or scope different than that in the initial 
    * registration, etc.
    */

   SLP_REFRESH_REJECTED             = -15,
   /*!< The SA attempted to refresh a registration more frequently than
    * the minimum refresh interval. The SA should call the appropriate API 
    * function to obtain the minimum refresh interval to use.
    */

   SLP_NOT_IMPLEMENTED              = -17,
   /*!< If an unimplemented feature is used, this error is returned. */

   SLP_BUFFER_OVERFLOW              = -18,
   /*!< An outgoing request overflowed the maximum network MTU size. The 
    * request should be reduced in size or broken into pieces and tried 
    * again.
    */

   SLP_NETWORK_TIMED_OUT            = -19,
   /*!< When no reply can be obtained in the time specified by the configured 
    * timeout interval for a unicast request, this error is returned.
    */

   SLP_NETWORK_INIT_FAILED          = -20,
   /*!< If the network cannot initialize properly, this error is returned. */

   SLP_MEMORY_ALLOC_FAILED          = -21,
   /*!< If the API fails to allocate memory, the operation is aborted and 
    * returns this.
    */

   SLP_PARAMETER_BAD                = -22,
   /*!< If a parameter passed into an interface is bad, this error is 
    * returned.
    */

   SLP_NETWORK_ERROR                = -23,
   /*!< The failure of networking during normal operations causes this error 
    * to be returned.
    */

   SLP_INTERNAL_SYSTEM_ERROR        = -24,
   /*!< A basic failure of the API causes this error to be returned. This 
    * occurs when a system call or library fails. The operation could not 
    * recover.
    */

   SLP_HANDLE_IN_USE                = -25,
   /*!< In the C API, callback functions are not permitted to recursively 
    * call into the API on the same SLPHandle, either directly or indirectly.  
    * If an attempt is made to do so, this error is returned from the called 
    * API function.
    */

   SLP_TYPE_ERROR                   = -26
   /*!< If the API supports type checking of registrations against service 
    * type templates, this error can arise if the attributes in a 
    * registration do not match the service type template for the service.
    */

} SLPError;

/** SLPBoolean (4.1.3)
 *
 * The SLPBoolean enum is used as a boolean flag.
 */
typedef enum {
   SLP_FALSE = 0,
   SLP_TRUE  = 1
} SLPBoolean;

/** SLPSrvURL (4.2.1)
 *
 * The SLPSrvURL structure is filled in by the SLPParseSrvURL() function
 * with information parsed from a character buffer containing a service
 * URL. The fields correspond to different parts of the URL. Note that the
 * structure is in conformance with the standard Berkeley sockets struct
 * servent, with the exception that the pointer to an array of characters
 * for aliases (s_aliases field) is replaced by the pointer to host name
 * (s_pcHost field).
 */
typedef struct srvurl {

   char * s_pcSrvType;
   /*!< A pointer to a character string containing the service type name, 
    * including naming authority. The service type name includes the 
    * "service:" if the URL is of the "service:" scheme. [RFC 2608]
    */

   char * s_pcHost;
   /*!< A pointer to a character string containing the host identification 
    * information.
    */

   int s_iPort;
   /*!< The port number, or zero if none. The port is only available if the 
    * transport is IP.
    */

   char * s_pcNetFamily;
   /*!< A pointer to a character string containing the network address 
    * family identifier. Possible values are "ipx" for the IPX family, "at" 
    * for the Appletalk family, and "" (i.e. the empty string) for the IP 
    * address family. OpenSLP extends the RFC here to add "v6" for IPv6
    * addresses (since "" indicates ip).
    */

   char * s_pcSrvPart;
   /*!< The remainder of the URL, after the host identification. */

} SLPSrvURL;

/** SLPHandle (4.2.2)
 *
 * The SLPHandle type is returned by SLPOpen() and is a parameter to all
 * SLP functions. It serves as a handle for all resources allocated on
 * behalf of the process by the SLP library.  The type is opaque, since
 * the exact nature differs depending on the implementation.
 */
typedef void * SLPHandle;

/** SLPRegReport (4.3.1)
 *
 * The SLPRegReport callback type is the type of the callback function
 * to the SLPReg(), SLPDereg(), and SLPDelAttrs() functions.
 *
 * @param[in] hSLP - The SLPHandle used to initiate the operation.
 *
 * @param[in] errCode - An error code indicating if an error occurred 
 *    during the operation.
 *
 * @param[in] pvCookie - Memory passed down from the client code that 
 *    called the original API function, starting the operation. May be 
 *    NULL.
 */
typedef void SLPCALLBACK SLPRegReport(
      SLPHandle   hSLP, 
      SLPError    errCode, 
      void *      pvCookie);

/** SLPSrvTypeCallback (4.3.2)
 *
 * The SLPSrvTypeCallback type is the type of the callback function
 * parameter to SLPFindSrvTypes() function.  If the hSLP handle
 * parameter was opened asynchronously, the results returned through the
 * callback MAY be uncollated.  If the hSLP handle parameter was opened
 * synchronously, then the returned results must be collated and
 * duplicates eliminated.
 *
 * @param[in] hSLP - The SLPHandle used to initiate the operation.
 *
 * @param[in] pcSrvTypes - A character buffer containing a comma 
 *    separated, null terminated list of service types.
 *
 * @param[in] errCode - An error code indicating if an error occurred 
 *    during the operation. The callback should check this error code 
 *    before processing the parameters. If the error code is other than
 *    SLP_OK, then the API library may choose to terminate the outstanding 
 *    operation.
 *
 * @param[in] pvCookie - Memory passed down from the client code that 
 *    called the original API function, starting the operation. May be 
 *    NULL.
 *
 * @return The client code should return SLP_TRUE if more data is 
 *    desired, otherwise SLP_FALSE.
 */
typedef SLPBoolean SLPCALLBACK SLPSrvTypeCallback(
      SLPHandle      hSLP,
      const char *   pcSrvTypes, 
      SLPError       errCode, 
      void *         pvCookie);

/** SLPSrvURLCallback (4.3.3)
 *
 * The SLPSrvURLCallback type is the type of the callback function
 * parameter to SLPFindSrvs() function.  If the hSLP handle parameter
 * was opened asynchronously, the results returned through the callback
 * MAY be uncollated.  If the hSLP handle parameter was opened
 * synchronously, then the returned results must be collated and
 * duplicates eliminated.
 *
 * @param[in] hSLP - The SLPHandle used to initiate the operation.
 *
 * @param[in] pcSrvURL - A character buffer containing the returned 
 *    service URL. May be NULL if errCode not SLP_OK.
 *
 * @param[in] sLifetime - An unsigned short giving the life time of the 
 *    service advertisement, in seconds. The value must be an unsigned
 *    integer less than or equal to SLP_LIFETIME_MAXIMUM.
 *
 * @param[in] errCode - An error code indicating if an error occurred 
 *    during the operation. The callback should check this error code 
 *    before processing the parameters. If the error code is other than
 *    SLP_OK, then the API library may choose to terminate the outstanding 
 *    operation. SLP_LAST_CALL is returned when no more services are 
 *    available and the callback will not be called again.
 *
 * @param[in] pvCookie - Memory passed down from the client code that 
 *    called the original API function, starting the operation. May be 
 *    NULL.
 *
 * @return The client code should return SLP_TRUE if more data is 
 *    desired, otherwise SLP_FALSE.
 */
typedef SLPBoolean SLPCALLBACK SLPSrvURLCallback(
      SLPHandle      hSLP,
      const char *   pcSrvURL, 
      unsigned short sLifetime, 
      SLPError       errCode,
      void *         pvCookie);

/** SLPAttrCallback (4.3.4)
 *
 * The SLPAttrCallback type is the type of the callback function
 * parameter to SLPFindAttrs() function.
 *
 * @par
 * The behavior of the callback differs depending on whether the
 * attribute request was by URL or by service type.  If the
 * SLPFindAttrs() operation was originally called with a URL, the
 * callback is called once regardless of whether the handle was opened
 * asynchronously or synchronously.  The pcAttrList parameter contains
 * the requested attributes as a comma separated list (or is empty if no
 * attributes matched the original tag list).
 *
 * @par
 * If the SLPFindAttrs() operation was originally called with a service
 * type, the value of pcAttrList and calling behavior depend on whether
 * the handle was opened asynchronously or synchronously.  If the handle
 * was opened asynchronously, the callback is called every time the API
 * library has results from a remote agent.  The pcAttrList parameter
 * MAY be uncollated between calls.  It contains a comma separated list
 * with the results from the agent that immediately returned results.
 * If the handle was opened synchronously, the results must be collated
 * from all returning agents and the callback is called once, with the
 * pcAttrList parameter set to the collated result.
 *
 * @param[in] hSLP - The SLPHandle used to initiate the operation.
 *
 * @param[in] pcAttrList - A character buffer containing a comma 
 *    separated, null terminated list of attribute id/value assignments, 
 *    in SLP wire format; i.e. "(attr-id=attr-value-list)" [RFC 2608].
 *
 * @param[in] errCode - An error code indicating if an error occurred 
 *    during the operation. The callback should check this error code 
 *    before processing the parameters. If the error code is other than
 *    SLP_OK, then the API library may choose to terminate the 
 *    outstanding operation.
 *
 * @param[in] pvCookie - Memory passed down from the client code that 
 *    called the original API function, starting the operation. May be 
 *    NULL.
 *
 * @return The client code should return SLP_TRUE if more data is 
 *    desired, otherwise SLP_FALSE.
 */
typedef SLPBoolean SLPCALLBACK SLPAttrCallback(
      SLPHandle      hSLP,
      const char *   pcAttrList, 
      SLPError       errCode, 
      void *         pvCookie); 

/*=========================================================================
 * SLPOpen (4.4.1)
 */
SLPEXP SLPError SLPAPI SLPOpen(
      const char *   pcLang, 
      SLPBoolean     isAsync,
      SLPHandle *    phSLP);

/*=========================================================================
 * SLPClose (4.4.2)
 */
SLPEXP void SLPAPI SLPClose(SLPHandle hSLP);

/*=========================================================================
 * SLPReg (4.5.1)
 */
SLPEXP SLPError SLPAPI SLPReg(
      SLPHandle      hSLP, 
      const char *   pcSrvURL,
      unsigned short usLifetime, 
      const char *   pcSrvType,
      const char *   pcAttrs, 
      SLPBoolean     fresh, 
      SLPRegReport   callback,
      void *         pvCookie); 

/*=========================================================================
 * SLPDereg (4.5.2)
 */
SLPEXP SLPError SLPAPI SLPDereg(
      SLPHandle      hSLP, 
      const char *   pcSrvURL,
      SLPRegReport   callback, 
      void *         pvCookie);

/*=========================================================================
 * SLPDelAttrs (4.5.3)
 */
SLPEXP SLPError SLPAPI SLPDelAttrs(
      SLPHandle      hSLP, 
      const char *   pcURL,
      const char *   pcAttrs, 
      SLPRegReport   callback, 
      void *         pvCookie);

/*=========================================================================
 * SLPFindSrvTypes (4.5.4)
 */
SLPEXP SLPError SLPAPI SLPFindSrvTypes(
      SLPHandle            hSLP, 
      const char *         pcNamingAuthority, 
      const char *         pcScopeList, 
      SLPSrvTypeCallback   callback, 
      void *               pvCookie);

/*=========================================================================
 * SLPFindSrvs (4.5.5)
 */
SLPEXP SLPError SLPAPI SLPFindSrvs(
      SLPHandle         hSLP, 
      const char *      pcServiceType,
      const char *      pcScopeList, 
      const char *      pcSearchFilter, 
      SLPSrvURLCallback callback, 
      void *            pvCookie);

/*=========================================================================
 * SLPFindAttrs (4.5.6)
 */
SLPEXP SLPError SLPAPI SLPFindAttrs(
      SLPHandle         hSLP, 
      const char *      pcURLOrServiceType, 
      const char *      pcScopeList,
      const char *      pcAttrIds, 
      SLPAttrCallback   callback, 
      void *            pvCookie);

/*=========================================================================
 * SLPGetRefreshInterval (4.6.1)
 */
SLPEXP unsigned short SLPAPI SLPGetRefreshInterval(void);

/*=========================================================================
 * SLPFindScopes (4.6.2)
 */
SLPEXP SLPError SLPAPI SLPFindScopes(
      SLPHandle   hSLP, 
      char **     ppcScopeList);

/*=========================================================================
 * SLPParseSrvURL (4.6.3)
 */
SLPEXP SLPError SLPAPI SLPParseSrvURL(
      const char *   pcSrvURL,
      SLPSrvURL **   ppSrvURL);

/*=========================================================================
 * SLPEscape (4.6.4)
 */
SLPEXP SLPError SLPAPI SLPEscape(
      const char *   pcInbuf, 
      char **        ppcOutBuf,
      SLPBoolean     isTag); 

/*=========================================================================
 * SLPUnescape (4.6.5)
 */
SLPEXP SLPError SLPAPI SLPUnescape(
      const char *   pcInbuf, 
      char **        ppcOutBuf,
      SLPBoolean     isTag);

/*=========================================================================
 * SLPFree (4.6.6)
 */
SLPEXP void SLPAPI SLPFree(void * pvMem);

/*=========================================================================
 * SLPGetProperty (4.6.7)
 */
SLPEXP const char * SLPAPI SLPGetProperty(const char * pcName);

/*=========================================================================
 * SLPSetProperty (4.6.8) - only partially implemented - can successfully
 *    be called as long as SLPGetProperty has not yet been called.
 */
SLPEXP void SLPAPI SLPSetProperty(
      const char *   pcName, 
      const char *   pcValue);

/*=========================================================================
 * SLPSetAppPropertyFile (post RFC 2614)
 */
SLPEXP SLPError SLPAPI SLPSetAppPropertyFile(
      const char *   pathname);

/*=========================================================================
 * SLPParseAttrs (post RFC 2614)
 */
SLPEXP SLPError SLPAPI SLPParseAttrs(
      const char *   pcAttrList, 
      const char *   pcAttrId, 
      char **        ppcAttrVal);

/*=========================================================================
 * SLPAssociateIFList (post RFC 2614)
 */
SLPEXP SLPError SLPAPI SLPAssociateIFList(
      SLPHandle      hSLP, 
      const char *   McastIFList);

/*=========================================================================
 * SLPAssociateIP (post RFC 2614)
 */
SLPEXP SLPError SLPAPI SLPAssociateIP(
      SLPHandle      hSLP, 
      const char *   unicast_ip);

#if __cplusplus
}
#endif

/*! @} */

#endif  /* SLP_H_INCLUDED */

/*=========================================================================*/
