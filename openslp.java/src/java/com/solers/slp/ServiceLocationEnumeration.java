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

import java.util.Enumeration;

/**
 * The SLP ServiceLocationEnumeration interface defined in RFC 2614.
 */
public interface ServiceLocationEnumeration extends Enumeration {

    /**
     * Returns the next result from an SLP request.
     */
    public abstract Object next() throws ServiceLocationException;

    /**
     * Frees all resources allocated by the enumeration.
     */
    public abstract void destroy();
}
