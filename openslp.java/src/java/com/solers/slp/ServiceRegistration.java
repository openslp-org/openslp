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

import com.solers.slp.AuthenticationBlock;
import com.solers.slp.ServiceLocationAttribute;
import com.solers.slp.ServiceLocationException;
import com.solers.slp.ServiceLocationManager;

import java.util.*;
import java.io.*;

/**
 * The ServiceRegistration SLP Message.
 *
 * @author Patrick Callis
 */
class ServiceRegistration extends SLPMessage {

    private ServiceURL _url;
    private Vector _attributes;
    private String _scopes;
    private AuthenticationBlock[] _attrAuths;

    ServiceRegistration(ServiceURL url, Vector attributes, Locale locale,
			String scopes) {
	super(locale, SRVREG);
	_url = url;
	_attributes = attributes;
	_scopes = scopes;
	_attrAuths = new AuthenticationBlock[0];
    }

    void sign() throws ServiceLocationException {
	_url.sign();
	SLPConfiguration conf = ServiceLocationManager.getConfiguration();
	if(conf.getSecurityEnabled() && _attributes.size() > 0) {
	    try {
		StringTokenizer st = new StringTokenizer(conf.getSPI(), ",");
		_attrAuths = new AuthenticationBlock[st.countTokens()];
		int i=0;
		while(st.hasMoreElements()) {
		    String spi = st.nextToken();
		    int timestamp = ServiceLocationManager.getTimestamp();
		    timestamp += _url.getLifetime();
		    byte[] data = getAuthData(spi, timestamp);
		    _attrAuths[i++] = new AuthenticationBlock((short)2, spi, timestamp,
							       data, null);
		}
	    }
	    catch(IOException e) {
		throw new ServiceLocationException("Could not sign URL",
					  ServiceLocationException.AUTHENTICATION_FAILED);
	    }
	}
    }

    private byte[] getAuthData(String spi, int timestamp) throws IOException {
	ByteArrayOutputStream bos = new ByteArrayOutputStream();
	DataOutputStream dos = new DataOutputStream(bos);

	byte[] temp = spi.getBytes();
	dos.writeShort(temp.length);
	dos.write(temp);

	dos.writeShort((short)ServiceLocationAttribute.calcAttrListSize(_attributes));
	Iterator i = _attributes.iterator();
	while(i.hasNext()) {
	    ((ServiceLocationAttribute)i.next()).writeAttribute(dos);
	    if(i.hasNext()) {
		dos.write(",".getBytes());
	    }
	}

	dos.writeInt(timestamp);
	return bos.toByteArray();
    }

    protected void writeBody(DataOutput out) throws java.io.IOException {
	_url.writeExternal(out); // URL entry
	byte[] temp = _url.getServiceType().toString().getBytes();
	out.writeShort((short)temp.length); // length of service type
	out.write(temp); // service type

	temp = _scopes.getBytes();
	out.writeShort((short)temp.length); // length of scopes
	out.write(temp);

	// Attributes
	out.writeShort((short)ServiceLocationAttribute.calcAttrListSize(_attributes));
	Iterator i = _attributes.iterator();
	while(i.hasNext()) {
	    ((ServiceLocationAttribute)i.next()).writeAttribute(out);
	    if(i.hasNext()) {
		out.write(",".getBytes());
	    }
	}

	// # of attr auths
	out.write((byte)_attrAuths.length);
	for(int j=0;j<_attrAuths.length;j++) {
	    _attrAuths[j].writeBlock(out);
	}
    }

    protected int calcSize() {
	int size = _url.calcSize()  // URL-Entry
	    + 2   // Length of service type
	    + _url.getServiceType().toString().length()
	    + 2   // Length of scope list
	    + _scopes.getBytes().length   // scopes
	    + 2   // length of attr list
	    + ServiceLocationAttribute.calcAttrListSize(_attributes)
	    + 1;  // # of attr auths

	for(int i=0;i<_attrAuths.length;i++) {
	    size += _attrAuths[i].calcSize();
	}

	return size;
    }

    public String toString() {
	String s = "ServiceRegistration: "+
	    "URL: " + _url + " Attributes: ";

	Iterator i = _attributes.iterator();
	while(i.hasNext()) {
	    s += i.next() + " ";
	}
	return s;
    }
}
