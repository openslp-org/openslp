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

import java.util.Vector;
import java.util.Locale;

/**
 * The SLP Locator interface defined in RFC 2614.
 */
public interface Locator {
    /**
     * Returns the current locale of this Locator.
     * @return a Locale
     */
    public abstract Locale getLocale();

    /**
     * Finds all service types that have a registered URL.
     * @param namingAuthority the naming authority to search
     *                        in, or empty string if all.
     * @return ServiceLocationEnumeration of ServiceType objects.
     * @exception com.solers.slp.ServiceLocationException
     */
    public abstract ServiceLocationEnumeration
	findServiceTypes(String namingAuthority,
			 Vector scopes)
	throws ServiceLocationException;

    /**
     * Finds all services of a particular type.
     * @param type the ServiceType to search for.
     * @param scopes list of scopes to look in.
     * @param searchFilter an SLP search filter string.
     * @return ServiceLocationEnumeration of ServiceURL objects.
     * @exception com.solers.slp.ServiceLocationException
     */
    public abstract ServiceLocationEnumeration
	findServices(ServiceType type,
		     Vector scopes,
		     String searchFilter)
	throws ServiceLocationException;

    /**
     * Returns the attributes registered with a ServiceURL.
     * @param URL the ServiceURL to query.
     * @param scopes the scopes to look in.
     * @param attributeIds list of ids to return, or empty
     *                     string if all.
     * @return ServiceLocationEnumeration of ServiceLocationAttribute
     *         objects.
     * @exception com.solers.slp.ServiceLocationException
     */
    public abstract ServiceLocationEnumeration
	findAttributes(ServiceURL URL,
		       Vector scopes,
		       Vector attributeIds)
	throws ServiceLocationException;

    /**
     * Returns the attributes registered with a ServiceType.
     * @param type the ServiceType to query.
     * @param scopes the scopes to look in.
     * @param attributeIds list of ids to return, or empty
     *                     string if all.
     * @return ServiceLocationEnumeration of ServiceLocationAttribute
     *         objects.
     * @exception com.solers.slp.ServiceLocationException
     */
    public abstract ServiceLocationEnumeration
	findAttributes(ServiceType type,
			Vector scopes,
			Vector attributeIds)
	throws ServiceLocationException;
}
