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

import java.util.*;
import java.io.*;

/**
 * The ServiceDeregistration SLP Message.
 *
 * @author Patrick Callis
 */
class ServiceDeregistration extends SLPMessage {

    private ServiceURL _url;
    private String _scopes;

    ServiceDeregistration(ServiceURL url, Locale locale, String scopes) {
	super(locale, SRVDEREG);
	_url = url;
	_scopes = scopes;
    }

    protected void writeBody(DataOutput out) throws java.io.IOException {
	byte[] temp = _scopes.getBytes();
	out.writeShort((short)temp.length); // scopes length
	out.write(temp); // scopes

	_url.writeExternal(out); // url entry

	out.writeShort((short)0); // length of tag list
    }

    protected int calcSize() {
	int size = 2  // Length of scope list
	    + _scopes.getBytes().length   // scopes
	    + _url.calcSize()  // URL-Entry
	    + 2;   // Length of tag list

	return size;
    }

    public String toString() {
	return "ServiceDeregistration: " +
	    "URL: " + _url;
    }
}
