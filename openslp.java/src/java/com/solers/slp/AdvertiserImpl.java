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

import com.solers.slp.Advertiser;

import java.util.Locale;
import java.util.Vector;

/**
 * Implementation of SLP Advertiser interface.
 *
 * @author Patrick Callis
 * @see com.solers.slp.Advertiser
 */
class AdvertiserImpl implements Advertiser {

    private Locale _locale;
    private SLPConfiguration _conf;
    private NetworkManager _net;

    AdvertiserImpl(Locale locale) {
	_locale = locale;
	_conf = ServiceLocationManager.getConfiguration();
	_net = NetworkManager.getInstance();
    }

    public Locale getLocale() {
	return _locale;
    }

    public void register(ServiceURL url, Vector attributes)
	throws ServiceLocationException {
	ServiceRegistration sr = new ServiceRegistration(url, attributes,
							 _locale, _conf.getScopes());
	sr.sign();
	_net.saMessage(sr);
    }

    public void deregister(ServiceURL url)
	throws ServiceLocationException {
	ServiceDeregistration sd = new ServiceDeregistration(url, _locale,
							     _conf.getScopes());
	_net.saMessage(sd);
    }

    public void addAttributes(ServiceURL url, Vector attributes)
	throws ServiceLocationException {
	throw new ServiceLocationException("Incremental Registrations are not supported",
					   ServiceLocationException.NOT_IMPLEMENTED);
    }

    public void deleteAttributes(ServiceURL url, Vector attributeIds)
	throws ServiceLocationException {
	throw new ServiceLocationException("Incremental Deregistrations are not supported",
					   ServiceLocationException.NOT_IMPLEMENTED);
    }

}
