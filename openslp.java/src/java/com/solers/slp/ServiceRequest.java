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

import com.solers.slp.ServiceLocationManager;

import java.io.*;
import java.util.*;

/**
 * The ServiceRequest SLP Message.
 *
 * @author Patrick Callis
 */
class ServiceRequest extends UAMessage {
    private ServiceType _type;
    private String _scopes;
    private String _searchFilter;
    private String _spi;

    ServiceRequest(ServiceType type, String scopes, String searchFilter,
		   Locale locale) {
	super(locale, SRVRQST);
	_type = type;
	_scopes = scopes;
	_searchFilter = searchFilter;
	SLPConfiguration conf = ServiceLocationManager.getConfiguration();
	_spi = conf.getSecurityEnabled() ? conf.getSPI() : "";
    }

    protected int calcSize() {
	int size = 2   // length of pr list
	    + getResponders().getBytes().length  // pr list
	    + 2        // length of type
	    + _type.toString().getBytes().length  // type
	    + 2        // length of scopes
	    + _scopes.getBytes().length // scopes
	    + 2        // length of filter
	    + _searchFilter.getBytes().length  // filter
	    + 2       // length of spi string
	    + _spi.getBytes().length; // spi string
	return size;
    }

    protected void writeBody(DataOutput out) throws IOException {
	byte[] temp = getResponders().getBytes();
	out.writeShort(temp.length); // length of pr list
	out.write(temp);             // pr list

	temp = _type.toString().getBytes();
	out.writeShort(temp.length); // length of type
	out.write(temp);             // type

	temp = _scopes.getBytes();
	out.writeShort(temp.length); // length of scopes
	out.write(temp);  // scopes

	temp = _searchFilter.getBytes();
	out.writeShort(temp.length); // length of filter
	out.write(temp);             // filter

	temp = _spi.getBytes();
	out.writeShort(temp.length);    // length of spi string
	out.write(temp);
    }

    public String toString() {
	return "ServiceRequest: " +
	    "Type: " + _type +
	    " Filter: " + _searchFilter;
    }
}
