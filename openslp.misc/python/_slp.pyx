# File:      slp.pyx
# Abstract:  Pyrex wrapper for OpenSLP
# Requires:  OpenSLP installation, Pyrex
#                                                   
# Author(s): Ganesan Rajagopal <rganesan@debian.org>
# 
# Copyright (C) 2003 The OpenSLP Project
#
# This program is released under same license as the OpenSLP project (BSD
# style without the advertising clause). Alternatively, you can use it under
# the Python License.

cdef extern from "slp.h":
    ctypedef enum SLPBoolean:
        SLP_FALSE
        SLP_TRUE
    ctypedef enum SLPError:
        SLP_LAST_CALL = 1
        SLP_OK = 0
        SLP_LANGUAGE_NOT_SUPPORTED = -1
        SLP_PARSE_ERROR = -2
        SLP_INVALID_REGISTRATION = -3
        SLP_SCOPE_NOT_SUPPORTED = -4
        SLP_AUTHENTICATION_ABSENT = -6
        SLP_AUTHENTICATION_FAILED = -7
        SLP_INVALID_UPDATE = -13
        SLP_REFRESH_REJECTED = -15
        SLP_NOT_IMPLEMENTED = -17
        SLP_BUFFER_OVERFLOW = -18
        SLP_NETWORK_TIMED_OUT = -19
        SLP_NETWORK_INIT_FAILED = -20
        SLP_MEMORY_ALLOC_FAILED = -21
        SLP_PARAMETER_BAD = -22
        SLP_NETWORK_ERROR = -23
        SLP_INTERNAL_SYSTEM_ERROR = -24
        SLP_HANDLE_IN_USE = -25
        SLP_TYPE_ERROR = -26
    cdef struct srvurl:
        char *s_pcSrvType
        char *s_pcHost
        int s_iPort
        char *s_pcNetFamily
        char *s_pcSrvPart
    ctypedef srvurl SLPSrvURL
    ctypedef void *SLPHandle
    
    ctypedef void SLPRegReport(SLPHandle hSLP, SLPError errCode,
                               void *pvCookie)
    ctypedef SLPBoolean SLPSrvTypeCallback(
        SLPHandle hSLP, char *pcSrvTypes, SLPError errCode, void *pvCookie)
    ctypedef SLPBoolean SLPSrvURLCallback(
        SLPHandle hSLP, char *pcSrvURL, unsigned short sLifetime,
        SLPError errCode, void *pvCookie)
    ctypedef SLPBoolean SLPAttrCallback(
        SLPHandle hSLP, char *pcAttrList, SLPError errCode, void *pvCookie)

    SLPError SLPOpen(char *lang, SLPBoolean isasync, SLPHandle *phslp)
    void SLPClose(SLPHandle hSLP)
    SLPError SLPReg(SLPHandle hSLP, char *pcSrvURL, unsigned short usLifetime,
                    char *pcSrvType, char *pcAttrs, SLPBoolean fresh,
                    SLPRegReport callback, void *pvCookie)
    SLPError SLPDereg(SLPHandle hSLP, char *pcSrvURL, SLPRegReport callback,
                      void *pvCookie)
    SLPError SLPDelAttrs(SLPHandle hSLP, char *pcSrvURL, char  *pcAttrs,
                         SLPRegReport callback, void *pvCookie)
    SLPError SLPFindSrvTypes(SLPHandle hslp, char *namingauthority,
                             char *scopelist, SLPSrvTypeCallback callback,
                             void* cookie)
    SLPError SLPFindSrvs(SLPHandle  hSLP, char *pcServiceType,
                         char *pcScopeList, char *pcSearchFilter,
                         SLPSrvURLCallback callback, void *pvCookie)
    SLPError SLPFindAttrs(SLPHandle hSLP, char *pcURLOrServiceType,
                          char *pcScopeList, char *pcAttrIds,
                          SLPAttrCallback callback, void *pvCookie)
    unsigned short SLPGetRefreshInterval()
    SLPError SLPFindScopes(SLPHandle hSLP, char **ppcScopeList)
    SLPError SLPParseSrvURL(char *pcSrvURL, SLPSrvURL** ppSrvURL)
    SLPError SLPEscape(char *pcInbuf, char **ppcOutBuf, SLPBoolean isTag)
    SLPError SLPUnescape(char* pcInbuf, char **ppcOutBuf, SLPBoolean isTag)
    void SLPFree(void *pvMem)
    char *SLPGetProperty(char* pcName)
    void SLPSetProperty(char *pcName, char *pcValue)
    SLPError SLPParseAttrs(char *pcAttrList, char *pcAttrId,
                           char **ppcAttrVal)

# C callbacks get a python tuple as the cookie parameter. The first element
# of the tuple is a python function and the second element is a cookie to
# be passed to it (as the last parameter). 

# Callback for register(), deregister() and delattrs() methods
cdef void errcb(SLPHandle hslp, SLPError errcode, void *cookie):
    callback, realcookie = <object>cookie
    callback(errcode, realcookie)

# Callback for findsrvtypes() and findattrs() methods
cdef SLPBoolean strcb(SLPHandle slph, char *string, SLPError errcode,
                      void *cookie): 
    cdef SLPBoolean ret
    callback, realcookie = <object>cookie
    if string != NULL:                  # when errcode != SLP_OK
        pystring = string
    ret = callback(pystring, errcode, realcookie)
    return ret

# Callback for findsrvs() menthod
cdef SLPBoolean srvcb(SLPHandle hslp, char *srvurl, unsigned short lifetime,
                      SLPError errcode, void *cookie):
    cdef SLPBoolean ret
    callback, realcookie = <object>cookie
    pysrvurl = None
    if srvurl != NULL:                  # when errcode != SLP_OK
        pysrvurl = srvurl
    ret = callback(pysrvurl, lifetime, errcode, realcookie)
    return ret

# Wrapper for OpenSLP APIs
cdef class SLPConn:
    cdef SLPHandle slph
    cdef object cookie

    # defaultcookie will be passed as the cookie parameter to callback
    # functions if no explicit parameter is specified in the call
    def __init__(self, char *lang, int async, object defaultcookie = None):
        self.cookie = defaultcookie
        err = SLPOpen(lang, async, &self.slph)
        if err != SLP_OK:
            raise EnvironmentError(err, "")

    def __del__(self):
        if self.slph != NULL:
            SLPClose(self.slph)

    # Close the SLP Handle
    def close(self):
        SLPClose(self.slph)
        self.slph = NULL

    # register an SLP service
    def register(self, char *srvurl, unsigned lifetime, char *attrs,
                 object callback, object cookie = None):
        pycb = (callback, cookie or self.cookie)
        return SLPReg(self.slph, srvurl, <unsigned short>lifetime, "",
                      attrs, 1, errcb, <void *>pycb)
                 
    # deregister an SLP service
    def deregister(self, char *srvurl, object callback,
                   object cookie = None):
        pycb = (callback, cookie or self.cookie)
        return SLPDereg(self.slph, srvurl, errcb, <void *>pycb)
        
    # delete attributes from a SLP service URL
    def delattrs(self, char *srvurl, char *attrs, object callback,
                 object cookie = None):
        pycb = (callback, cookie or self.cookie)
        return SLPDelAttrs(self.slph, srvurl, attrs, errcb, <void *>pycb)

    # find SLP service types
    def findsrvtypes(self, char *na, char *scopelist, object callback,
                     object cookie = None):
        pycb = (callback, cookie or self.cookie)
        return SLPFindSrvTypes(self.slph, na, scopelist, strcb, <void *>pycb)

    # find SLP services matching the service type and searchfilter
    def findsrvs(self, char *srvtype, char *scopelist, char *searchfilter,
                 object callback, object cookie = None):
        pycb = (callback, cookie or self.cookie)
        return SLPFindSrvs(self.slph, srvtype, scopelist, searchfilter,
                           srvcb, <void *>pycb)

    # find attributes for the given SLP service URL
    def findattrs(self, char *url, char *scopelist, char *attrids,
                  object callback, object cookie = None):
        pycb = (callback, cookie or self.cookie)
        return SLPFindAttrs(self.slph, url, scopelist, attrids, strcb,
                            <void *>pycb)

    # find supported SLP scopes
    def findscopes(self):
        cdef char *scopes
        err = SLPFindScopes(self.slph, &scopes)
        if err != SLP_OK:
            raise EnvironmentError(err, "")
        return scopes

def getrefreshinterval():
    """get refresh interval"""
    return SLPGetRefreshInterval()

def parsesrvurl(char *srvurl):
    """Parse given service URL"""
    cdef SLPSrvURL *pSrvURL
    err = SLPParseSrvURL(srvurl, &pSrvURL)
    if err != SLP_OK:
        raise EnvironmentError(err, "")
    parsedurl = (pSrvURL.s_pcSrvType, pSrvURL.s_pcHost, pSrvURL.s_iPort,
                 pSrvURL.s_pcNetFamily, pSrvURL.s_pcSrvPart)
    SLPFree(pSrvURL)
    return parsedurl

def escape(char *s, int isTag):
    """Escape given string"""
    cdef char *outs
    err = SLPEscape(s, &outs, isTag)
    if err != SLP_OK:
        raise EnvironmentError(err, "")
    ret = outs
    SLPFree(outs)
    return ret

def unescape(char *s, int isTag):
    """Unescape given string"""
    cdef char *outs
    err = SLPUnescape(s, &outs, isTag)
    if err != SLP_OK:
        raise EnvironmentError(err, "")
    ret = outs
    SLPFree(outs)
    return ret

def getprop(char *name):
    """Get a SLP property"""
    return SLPGetProperty(name)

def setprop(char *name, char *value):
    """Set an SLP property"""
    SLPSetProperty(name, value)

# Local Variables: 
# mode: python 
# End: 
