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
 * The ServiceTypeRequest SLP Message.
 *
 * @author Patrick Callis
 */
class ServiceTypeRequest extends UAMessage {
    private String _namingAuthority;
    private String _scopes;

    ServiceTypeRequest(String namingAuthority, String scopes,
		       Locale locale) {
	super(locale, SRVTYPERQST);
	_namingAuthority = namingAuthority;
	_scopes = scopes;
    }

    protected int calcSize() {
	int size = 2   // length of pr list
	    + getResponders().getBytes().length  // pr list
	    + 2        // length of naming auth
	    + _namingAuthority.getBytes().length  // naming auth
	    + 2        // length of scopes
	    + _scopes.getBytes().length; // scopes

        if ("*".compareTo (_namingAuthority) == 0)
        {
            // System.err.println ("* detected for naming authority reduce size 1 byte");
            size --;
        }

	return size;
    }

    protected void writeBody(DataOutput out) throws IOException {
	byte[] temp = getResponders().getBytes();
	out.writeShort(temp.length); // length of pr list
	out.write(temp);             // pr list

        // '*' means all naming authorities
        if ("*".compareTo (_namingAuthority) == 0)
        {
            // System.err.println ("* detected for naming. Setting size len to 0xffff");
	    out.writeShort(0xffff); // length of naming auth
        }
        else {
	    temp = _namingAuthority.getBytes();
	    out.writeShort(temp.length); // length of naming auth
	    out.write(temp);             // naming auth
        }

	temp = _scopes.getBytes();
	out.writeShort(temp.length); // length of scopes
	out.write(temp);  // scopes
    }

    public String toString() {
	return "ServiceTypeRequest: " +
	    "Naming Authority: " + _namingAuthority;
    }
}
