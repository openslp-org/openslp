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
import com.solers.slp.ServiceLocationException;
import com.solers.slp.ServiceLocationManager;
import com.solers.slp.ServiceType;

import java.io.*;
import java.util.StringTokenizer;

/**
 * Implementation of the SLP ServiceURL class defined in RFC 2614.
 *
 * @author Patrick Callis
 */
public class ServiceURL implements Serializable {
    public static final int NO_PORT = 0;
    public static final int LIFETIME_NONE=0;
    public static final int LIFETIME_DEFAULT = 10800;
    public static final int LIFETIME_MAXIMUM = 65535;
    public static final int LIFETIME_PERMANENT = -1;

    private String _url = null;;
    private int _lifetime = 0;
    private ServiceType _type = null;
    private String _host = null;
    private String _transport = null;
    private int _port = 0;
    private String _path = null;
    private AuthenticationBlock[] _authBlocks;

    public ServiceURL(String URL, int lifetime)
    throws ServiceLocationException
    {
	_url = URL;
	_lifetime = lifetime;
	_authBlocks = new AuthenticationBlock[0];

        try {
	    parse();
        }
        catch (Exception ex) {
	    throw new ServiceLocationException("service url is malformed: ["
                + _url + "]. ",
                ServiceLocationException.PARSE_ERROR);
        }
    }

    ServiceURL() {
    }

    private void parse() {
	int transportSlash1 = _url.indexOf(":/");
	_type = new ServiceType(_url.substring(0, transportSlash1++));

	int transportSlash2 = _url.indexOf("/", transportSlash1 + 1);
	_transport = _url.substring(transportSlash1, transportSlash2-1);

	int hostEnd = -1;
	if(_transport.equals("")) {
	    hostEnd = _url.indexOf(":", transportSlash2+1);
	}

	int pathStart;
	if(hostEnd == -1) {
	    _port = NO_PORT;
	    pathStart = hostEnd = _url.indexOf("/", transportSlash2+1);
	}
	else {
	    pathStart = _url.indexOf("/", hostEnd+1);
	    if(pathStart == -1) {
		_port = Integer.parseInt(_url.substring(hostEnd+1));
	    }
	    else {
		_port = Integer.parseInt(_url.substring(hostEnd+1, pathStart));
	    }
	}

	if(hostEnd == -1) {
	    _host = _url.substring(transportSlash2+1);
	}
	else {
	    _host = _url.substring(transportSlash2+1, hostEnd);
	}

	if(pathStart == -1) {
	    _path = "";
	}
	else {
	    _path = _url.substring(pathStart);
	}
    }

    void sign() throws ServiceLocationException {
	SLPConfiguration conf = ServiceLocationManager.getConfiguration();
	if(conf.getSecurityEnabled()) {
	    try {
		StringTokenizer st = new StringTokenizer(conf.getSPI(), ",");
		_authBlocks = new AuthenticationBlock[st.countTokens()];
		int i=0;
		while(st.hasMoreElements()) {
		    String spi = st.nextToken();
		    int timestamp = ServiceLocationManager.getTimestamp();
		    timestamp += _lifetime;

		    byte[] data = getAuthData(spi, timestamp);
		    _authBlocks[i++] = new AuthenticationBlock((short)2, spi, timestamp,
							       data, null);
		}
	    }
	    catch(IOException e) {
		throw new ServiceLocationException("Could not sign URL",
					  ServiceLocationException.AUTHENTICATION_FAILED);
	    }
	}
    }

    boolean verify() {
	SLPConfiguration conf = ServiceLocationManager.getConfiguration();
	if(conf.getSecurityEnabled()) {
	    for(int i=0;i<_authBlocks.length;i++) {
		try {
		    byte[] data = getAuthData(_authBlocks[i].getSPI(),
					      _authBlocks[i].getTimestamp());
		    if(_authBlocks[i].verify(data)) {
			return true;
		    }
		}
		catch(Exception e) {
		}
	    }
	    return false;
	}
	else {
	    return true;
	}
    }

    private byte[] getAuthData(String spi, int timestamp) throws IOException {
	ByteArrayOutputStream bos = new ByteArrayOutputStream();
	DataOutputStream dos = new DataOutputStream(bos);

	byte[] temp = spi.getBytes();
	dos.writeShort(temp.length);
	dos.write(temp);
	temp = toString().getBytes();
	dos.writeShort(temp.length);
	dos.write(temp);
	dos.writeInt(timestamp);
	return bos.toByteArray();
    }

    public ServiceType getServiceType() {
	return _type;
    }

    public final void setServiceType(ServiceType type) {
	_type = type;
    }

    public String getTransport() {
	return _transport;
    }

    public String getHost() {
	return _host;
    }

    public int getPort() {
	return _port;
    }

    public String getURLPath() {
	return _path;
    }

    public int getLifetime() {
	return _lifetime;
    }

    public boolean equals(Object obj) {
	if(obj instanceof ServiceURL) {
	    ServiceURL u = (ServiceURL)obj;
	    return (_type.equals(u._type) &&
		    _host.equals(u._host) &&
		    _port == u._port &&
		    _transport.equals(u._transport) &&
		    _path.equals(u._path));
	}
	else {
	    return false;
	}
    }

    public String toString() {
	return _type.toString() + ":/" + _transport + "/" + _host +
	    (_port != NO_PORT ? (":" + _port) : "") + _path;
    }

    public int hashCode() {
	// TODO
	return 0;
    }

    void writeExternal(DataOutput out) throws IOException {
	out.write(0);
	out.writeShort((short)_lifetime);

	byte[] urlBytes = this.toString().getBytes();
	out.writeShort((short)urlBytes.length);
	out.write(urlBytes);
	out.write(_authBlocks.length);

	for(int i=0;i<_authBlocks.length;i++) {
	    _authBlocks[i].writeBlock(out);
	}
    }

    void readExternal(DataInput in) throws IOException {
	in.readByte();
	_lifetime = in.readShort() & 0xFFFF;
	int len = in.readShort() & 0xFFFF;
	byte[] urlArr = new byte[len];
	in.readFully(urlArr);
	_url = new String(urlArr);

	_authBlocks = new AuthenticationBlock[in.readByte() & 0xFF];
	for(int i=0;i<_authBlocks.length;i++) {
	    _authBlocks[i] = new AuthenticationBlock();
	    _authBlocks[i].readBlock(in);
	}

	parse();
    }

    int calcSize() {
	int size = 1 // Reserved
	    + 2  // lifetime
	    + 2  // url length
	    + this.toString().getBytes().length
	    + 1;  // # of URL auth

	for(int i=0;i<_authBlocks.length;i++) {
	    size += _authBlocks[i].calcSize();
	}
	return size;
    }
}
