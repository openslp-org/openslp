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

import java.io.*;
import java.util.*;

/**
 * The AttributeRequest SLP Message.
 *
 * @author Patrick Callis
 */
class AttributeRequest extends UAMessage {

    private String _scopes;
    private String _attributeIds;
    private String _url;
    private Locale _locale;
    private String _spi;

    AttributeRequest(String url, String scopes, Vector attributeIds,
		     Locale locale) {
	super(locale, ATTRRQST);
	_locale = locale;
	_url = url;
	_scopes = scopes;

	StringBuffer buffer = new StringBuffer();
	Iterator i2 = attributeIds.iterator();
	while(i2.hasNext()) {
	    buffer.append((String)i2.next());
	    if(i2.hasNext()) {
		buffer.append(",");
	    }
	}
	_attributeIds = buffer.toString();

	SLPConfiguration conf = ServiceLocationManager.getConfiguration();
	_spi = conf.getSecurityEnabled() ? conf.getSPI() : "";
    }

    protected int calcSize() {
	int size = 2   // length of pr list
	    + getResponders().getBytes().length  // pr list
	    + 2        // length of url
	    + _url.getBytes().length  // url
	    + 2        // length of scope list
	    + _scopes.getBytes().length  // scope list
	    + 2        // length of tag list
	    + _attributeIds.getBytes().length // tag list
	    + 2        // length of spi
	    + _spi.getBytes().length; // spi string
	return size;
    }

    protected void writeBody(DataOutput out) throws IOException {
	byte[] temp = getResponders().getBytes();
	out.writeShort(temp.length); // length of pr list
	out.write(temp); // pr list

	temp = _url.getBytes();
	out.writeShort(temp.length); // length of url
	out.write(temp); // url

	temp = _scopes.getBytes();
	out.writeShort(temp.length); // length of scopes
	out.write(temp); // scopes

	temp = _attributeIds.getBytes();
	out.writeShort(temp.length); // length of attr ids
	out.write(temp); // attr ids

	temp = _spi.getBytes();
	out.writeShort(temp.length); //length of spi
	out.write(temp); // spi string
    }

    public String toString() {
	return "AttributeRequest: " +
	    "URL: " + _url +
	    " AttributeIds: " + _attributeIds;
    }
}
