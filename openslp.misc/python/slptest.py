#!/usr/bin/python2.2

import slp

url = "service:test2://10.0.0.1/some/junk/path"
attrs = { 'attr1': 'val1', 'attr2' : 'val2'}
parsedurl = slp.parsesrvurl(url)
print url, '=', parsedurl

slph = slp.open()
scopes = slph.findscopes()
print "scopes =", scopes

slph.register(url, 65535, attrs)

srvtypes = slph.findsrvtypes()
print "srvtypes =", srvtypes

for type in srvtypes:
    print type + ":"
    services = slph.findsrvs(type, "")
    for srv in services:
        attrs = slph.findattrs(srv[0])
        print srv, attrs

srv = slph.findsrvs("service:test2", "(attr1=val1)")[0]
print srv
print slph.findattrs(srv[0], ["attr1"])
        
slph.close()
