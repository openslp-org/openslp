# File:      slp.pyx
# Abstract:  Python wrapper for OpenSLP
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

def slpstr(data):
    """slpstr(data) converts data into a string like the str() builtin
    function str() except that the conversion is in a format suitable for
    the SLP API. A list will be flattened by joining it with a comma. A
    hash will become a comma separated (key=val) pairs"""
    if type(data) == type(""):
        datastr = data
    elif type(data) == type([]):
        datastr = ",".join(data)
    elif type(data) == type({}):
        l = []
        for key, val in data.items():
            l.append("(%s=%s)" % (key, val))
        datastr = ",".join(l)
    else:
        datastr = str(data)
    return datastr

class SLPException:
    """Exception raised when an SLP error is returned by the C API """
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
    
    def __init__(self, errcode):
        self.errcode = errcode
        
cdef void MySLPRegReport(SLPHandle hslp, SLPError errcode, void *cookie):
    """Callback for register() and deregister() methods in SLPConn."""
    if errcode != SLP_OK:
        raise SLPException(errcode)

cdef SLPBoolean MySLPSrvTypeCallback(SLPHandle slph, char *srvtype,
                                     SLPError errcode, void *cookie):
    """Callback for SLPConn.findsrvtypes"""
    srvtypelist = <object>cookie
    if errcode == SLP_OK:
        # srvtype may be a comma separated list
        srvtypes = srvtype.split(",")
        srvtypelist.extend(srvtypes)
    elif errcode == SLP_LAST_CALL:
        pass
    else:
        raise SLPException(errcode)

    return SLP_TRUE

cdef SLPBoolean MySLPSrvURLCallback (SLPHandle hslp, char *srvurl,
                                     unsigned short lifetime,
                                     SLPError errcode, void *cookie):
    """Callback for SLPConn.findsrvs"""
    srvlist = <object>cookie
    if errcode == SLP_OK:
        srvlist.append((srvurl, lifetime))
    elif errcode == SLP_LAST_CALL:
        pass
    else:
        raise SLPException(errcode)

    return SLP_TRUE

cdef SLPBoolean MySLPAttrCallback(SLPHandle hslp, char* attrstr, 
                                  SLPError errcode, void* cookie):
    """Callback for SLPConn.findattrs"""
    attrs = <object>cookie
    if errcode == SLP_OK:
        for attr in attrstr.split(","):
            key, val = attr[1:-1].split("=")
            attrs[key] = val
    elif errcode == SLP_LAST_CALL:
        pass
    else:
        raise SLPException(errcode)
    return SLP_TRUE

cdef SLPBoolean PythonSrvTypeCallback(SLPHandle slph, char *srvtype,
                                      SLPError errcode, void *cookie):
    """Callback for the SLPConn.findsrvtypes API with a callback. Not
    used currently"""
    callback = <object>cookie
    cdef SLPBoolean ret
    ret = callback(srvtype, errcode)
    return ret

def open(lang = "", async = 0):
    return SLPConn(lang, async)
    
cdef class SLPConn:
    """SLP Connecton Handle"""
    
    cdef SLPHandle slph
    def __init__(self, lang = "", async = 0):
        err = SLPOpen(lang, async, &self.slph)
        if err != SLP_OK:
            raise SLPException(err)

    def __del__(self):
        if self.slph != <void *>0:
            SLPClose(self.slph)

    def close(self):
        """Close the SLP Handle"""
        SLPClose(self.slph)
        self.slph = <void *>0

    def register(self, object srvurl, lifetime, attrs = "", fresh = 1):
        """register an SLP service"""
        attrstr = slpstr(attrs)
        err = SLPReg(self.slph, srvurl, lifetime, "", attrstr,
                     fresh, MySLPRegReport, <void *>0)
        if err != SLP_OK:
            raise SLPException(err)
                 
    def deregister(self, object srvurl):
        """deregister an SLP service"""
        err = SLPDereg(self.slph, srvurl, MySLPRegReport, <void *>0)
        if err != SLP_OK:
            raise SLPException(err)
        
    def delattrs(self, object srvurl, attrs = ""):
        """delete attributes from a SLP service URL"""
        attrstr = slpstr(attrs)
        err = SLPDelAttrs(self.slph, srvurl, attrstr,
                          MySLPRegReport, <void *>0)
        if err != SLP_OK:
            raise SLPException(err)

    def findsrvtypes(self, scopelist = "default", na = ""):
        """find SLP service types"""
        srvtypes = []
        scopestr = slpstr(scopelist)
        err = SLPFindSrvTypes(self.slph, na, scopestr,
                              MySLPSrvTypeCallback, <void *>srvtypes)
        if err != SLP_OK:
            raise SLPException(err)
        
        return srvtypes

    def findsrvs(self, srvtype, searchfilter = "", scopelist = "default"):
        """find SLP services matching the service type and searchfilter"""
        srvlist = []
        scopestr = slpstr(scopelist)
        err = SLPFindSrvs(self.slph, srvtype, scopestr, searchfilter,
                          MySLPSrvURLCallback, <void *>srvlist)
        if err != SLP_OK:
            raise SLPException(err)
        return srvlist

    def findattrs(self, urlorsrvtype, attrids = "", scopelist = "default"):
        """find attributes for the given SLP service URL or service type"""
        scopestr = slpstr(scopelist)
        attridstr = slpstr(attrids)
        attrs = {}
        err = SLPFindAttrs(self.slph, urlorsrvtype, scopestr,
                           attridstr, MySLPAttrCallback,
                           <void *>attrs)
        if err != SLP_OK:
            raise SLPException(err)
        return attrs

    def getrefinterval(self):
        """get refresh interval"""
        return SLPGetRefreshInterval()

    def findscopes(self):
        """find supported SLP scopes"""
        cdef char *cscopes
        err = SLPFindScopes(self.slph, &cscopes)
        if err != SLP_OK:
            raise SLPException(err)
        return cscopes.split(",")

def parsesrvurl(object srvurl):
    """Parse given service URL"""
    cdef SLPSrvURL *pSrvURL
    err = SLPParseSrvURL(srvurl, &pSrvURL)
    if err != SLP_OK:
        raise SLPException(err)
    parsedurl = (pSrvURL.s_pcSrvType, pSrvURL.s_pcHost, pSrvURL.s_iPort,
                 pSrvURL.s_pcNetFamily, pSrvURL.s_pcSrvPart)
    SLPFree(pSrvURL)
    return parsedurl

def escape(s, isTag):
    """Escape given string"""
    cdef char *outs
    err = SLPEscape(s, &outs, isTag)
    if err != SLP_OK:
        raise SLPException(err)
    ret = outs
    SLPFree(outs)
    return ret

def unescape(s, isTag):
    """Unescape given string"""
    cdef char *outs
    err = SLPUnescape(s, &outs, isTag)
    if err != SLP_OK:
        raise SLPException(err)
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
