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

import com.solers.slp.*;
import com.solers.slp.Advertiser;
import com.solers.slp.AdvertiserImpl;
import com.solers.slp.Locator;
import com.solers.slp.LocatorImpl;
import com.solers.slp.ServiceLocationException;

import java.util.*;
import java.io.File;
import java.io.IOException;

/**
 * Implementation of the SLP ServiceLocationManager class defined in RFC 2614.
 *
 * @author Patrick Callis
 */
public class ServiceLocationManager {
    private static SLPConfiguration  _conf;

    private ServiceLocationManager() {
	// No one should construct one of these.
    }

    /**
     * Initializes the manager's configuration using the
     * System properties.
     */
    public static void init() {
	init(System.getProperties());
    }

    /**
     * Initializes the manager's configuration with a set
     * of properties.
     * @param p the properties to initialize with.
     */
    public static void init(Properties p) {
	_conf = new SLPConfiguration(p);
    }

    /**
     * Initializes the manager's configuration from a file.
     * The file must be in the format specified by RFC 2614.
     * @param f RFC 2614 SLP configuration file.
     */
    public static void init(File f) throws IOException {
	_conf = new SLPConfiguration(f);
    }

    static SLPConfiguration getConfiguration() {
	if(_conf == null) {
	    init();
	}

	return _conf;
    }

    public static int getRefreshInterval() throws ServiceLocationException {
	return 0;
    }

    public static Vector findScopes() throws ServiceLocationException {
	String s = getConfiguration().getScopes();
	StringTokenizer st = new StringTokenizer(s, ",");
	Vector v = new Vector();
	while(st.hasMoreElements()) {
	    v.add(st.nextToken());
	}
	return v;
    }

    public static Locator getLocator(Locale locale) throws ServiceLocationException {

	return new LocatorImpl(locale);
    }

    public static Advertiser getAdvertiser(Locale locale)
	throws ServiceLocationException {

	return new AdvertiserImpl(locale);
    }

    /**
     * Returns the number of seconds since Jan 1, 1970
     */
    static int getTimestamp() {
	long systemTime = System.currentTimeMillis();
	systemTime /= 1000;
	return (int)systemTime;
    }
}
