/*
 * Copyright 2003 Solers Corporation.  All rights reserved.
 *
 * Modification and use of this SLP API software and
 * associated documentation ("Software") is permitted provided that the
 * conditions specified in the license.txt file included within this
 * distribution are met.
 *
 */

package com.solers.slp;

import com.solers.slp.AttributeRequest;
import com.solers.slp.Locator;

import java.util.*;

/**
 * Implementation of SLP Locator interface.
 *
 * @author Patrick Callis
 * @see com.solers.slp.Locator
 */
class LocatorImpl implements Locator {
    private Locale _locale;
    private NetworkManager _net;

    LocatorImpl(Locale locale) {
	_locale = locale;
	_net = NetworkManager.getInstance();
    }

    public Locale getLocale() {
	return _locale;
    }

    public ServiceLocationEnumeration
	findServiceTypes(String namingAuthority, Vector scopes)
	throws ServiceLocationException {
	ServiceTypeRequest str = new ServiceTypeRequest(namingAuthority,
							scopeString(scopes),
							_locale);
	return new ServiceLocationEnumerationImpl(_net, str);
    }

    public ServiceLocationEnumeration
	findServices(ServiceType type, Vector scopes,
		     String searchFilter)
	throws ServiceLocationException {
	ServiceRequest sr = new ServiceRequest(type, scopeString(scopes), searchFilter,
					       _locale);
	return new ServiceLocationEnumerationImpl(_net, sr);
    }

    public ServiceLocationEnumeration
	findAttributes(ServiceURL URL, Vector scopes,
		       Vector attributeIds)
	throws ServiceLocationException {
	AttributeRequest ar = new AttributeRequest(URL.toString(),
						   scopeString(scopes), attributeIds,
						   _locale);
	return new ServiceLocationEnumerationImpl(_net, ar);
    }

    public ServiceLocationEnumeration
	findAttributes(ServiceType type, Vector scopes,
			Vector attributeIds)
	throws ServiceLocationException {

	AttributeRequest ar = new AttributeRequest(type.toString(), scopeString(scopes),
						   attributeIds, _locale);
	return new ServiceLocationEnumerationImpl(_net, ar);
    }

    private static String scopeString(Vector scopes) {
	StringBuffer scopeBuffer = new StringBuffer();
	Iterator i = scopes.iterator();
	while(i.hasNext()) {
	    scopeBuffer.append((String)i.next());
	    if(i.hasNext()) {
		scopeBuffer.append(',');
	    }
	}
	return scopeBuffer.toString();
    }
}
