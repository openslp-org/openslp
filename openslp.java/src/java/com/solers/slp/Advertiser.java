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
 * The SLP Advertiser interface defined in RFC 2614.
 */
public interface Advertiser {
    /**
     * Returns the current locale of this Advertiser.
     * @return a Locale
     */
    public abstract Locale getLocale();

    /**
     * Registers a ServiceURL with the slp daemon.
     * @param URL the ServiceURL to register.
     * @param attributes the attributes associated with this
     *                   URL, or an empty Vector.
     * @exception com.solers.slp.ServiceLocationException
     */
    public abstract void register(ServiceURL URL, Vector attributes)
	throws ServiceLocationException;

    /**
     * Deregisters a particular URL.
     * @param URL the ServiceURL to deregister.
     * @exception com.solers.slp.ServiceLocationException
     */
    public abstract void deregister(ServiceURL URL)
	throws ServiceLocationException;

    /**
     * This operation is not supported.
     * @exception com.solers.slp.ServiceLocationException always.
     */
    public abstract void addAttributes(ServiceURL URL, Vector attributes)
	throws ServiceLocationException;

    /**
     * This operation is not supported.
     * @exception com.solers.slp.ServiceLocationException always.
     */
    public abstract void deleteAttributes(ServiceURL URL, Vector attributeIds)
	throws ServiceLocationException;
}
