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
import java.net.*;
import org.apache.log4j.Category;
import com.solers.slp.AuthenticationBlock;
import com.solers.slp.NetworkManager;
import com.solers.slp.ServiceLocationAttribute;
import com.solers.slp.ServiceLocationEnumeration;

/**
 * Implements the ServiceLocationEnumeration interface defined in
 * RFC 2614.
 *
 * @author Patrick Callis
 */
class ServiceLocationEnumerationImpl
    implements ServiceLocationEnumeration {

    /** Default Logging category */
    private Category cat =
	Category.getInstance(ServiceLocationEnumerationImpl.class.getName());

    private SLPConfiguration _conf;

    private NetworkManager _net;
    private UAMessage _message;

    /** Used for writing UA messages */
    private ByteArrayOutputStream uaBos;
    /** Wrapper for ByteArrayOutputStream */
    private DataOutputStream uaDos;
    /** Datagram socket for UA messages */
    private DatagramSocket _uaSock;

    /** Array of timeout values read from configuration */
    private int[] _retry;
    /** Max time to wait for response to UA message */
    private int _maxWait;

    private Vector _received = new Vector();
    private Vector _delivered = new Vector();

    /**
     * Used to remember if we've asked for responses yet.
     * If this is a unicast request, don't wait for more
     * than one response.
     */
    private boolean _requested = false;

    /** Holds the errorCode from the last response */
    private short _errorCode = 0;

    ServiceLocationEnumerationImpl(NetworkManager net, UAMessage message)
	throws ServiceLocationException {
	_conf = ServiceLocationManager.getConfiguration();

	_retry = _conf.getMcastTimeouts();
	_maxWait = _conf.getMcastMaxWait();

	_net = net;
	_message = message;

	uaBos = new ByteArrayOutputStream(_conf.getMTU());
	uaDos = new DataOutputStream(uaBos);

	_message.setXid(_net.nextXid());

	try {
	    _uaSock = new DatagramSocket();
	    transmitDatagram();
	}
	catch(IOException e) {
	    throw new ServiceLocationException("Could not send message.",
					       ServiceLocationException.NETWORK_ERROR);
	}
    }

    /**
     * Returns the next SLP response received.
     * @exception com.solers.slp.ServiceLocationException if an error is received.
     * @exception java.util.NoSuchElementException if no more responses.
     */
    public Object next() throws ServiceLocationException {
	try {
	    if(_received.size() == 0 && (_message.isMcast() || !_requested) &&
	       _errorCode == 0) {
		_errorCode = readResponse();
		_requested = true;
	    }

	    if(_errorCode != 0) {
		short error = _errorCode;
		_errorCode = 0;
		throw new ServiceLocationException("Error from SA", error);
	    }

	    if(_received.size() > 0) {
		Object o = _received.remove(0);
		_delivered.add(o);
		return o;
	    }
	    else {
		throw new NoSuchElementException();
	    }
	}
	catch(IOException e) {
	    cat.error("Exception reading responses.", e);
	    throw new ServiceLocationException(e.toString(),
					       ServiceLocationException.NETWORK_ERROR);
	}
    }

    /**
     * Returns whether more responses exist.
     * In order to allow clients to only block for responses
     * they want, this method must try to receive more
     * responses if none are queued.
     */
    public boolean hasMoreElements() {
	if(_received.size() > 0) {
	    return true;
	}
	else if(_message.isMcast() || !_requested) {
	    try {
		_errorCode =
		    readResponse();
		_requested = true;
	    }
	    catch(IOException e) {
		cat.error("Exception reading responses.", e);
	    }
	}

	return (_received.size() > 0);
    }

    /**
     * @see #next()
     */
    public Object nextElement() {
	try {
	    return next();
	}
	catch(ServiceLocationException e) {
	    throw new NoSuchElementException(e.toString());
	}
    }

    /**
     * Performs the actual transmission of a UA message.
     */
    private void transmitDatagram()
	throws IOException {
	InetAddress addr = _net.getDaAddress();

	if(_net.traceMsg.isDebugEnabled()) {
	    _net.traceMsg.debug("Sending: " + _message.toString());
	}

	uaBos.reset();
	if(!_message.writeMessage(uaDos)) {
	    // Message doesn't fit in MTU,
	    // probably too many previous responders
	    return;
	}
	byte[] buf = uaBos.toByteArray();

	if(addr.isMulticastAddress()) {
	    _message.setMcast(true);
	    _message.setTransmitSchedule(_conf.getMcastTimeouts());
	    buf[5] = (byte)(buf[5] | 0x20);
	}
	else {
	    _message.setMcast(false);
	    _message.setTransmitSchedule(_conf.getDatagramTimeouts());
	    buf[5] = (byte)(buf[5] & 0xDF);

	    if(_net.traceDA.isDebugEnabled()) {
		_net.traceDA.debug("Sending message to DA with XID " +
			      _message.getXid());
	    }
	}

	buf[10] = (byte)(_message.getXid() >> 8);
	buf[11] = (byte)(_message.getXid() & 0xFF);
	DatagramPacket p = new DatagramPacket(buf, buf.length);

	p.setAddress(addr);
	p.setPort(NetworkManager.SLP_RESERVED_PORT);
	_uaSock.send(p);
	_message.sent();
    }

    /**
     * Listens for a response to a UA message. The message may
     * be retransmitted if necessary.  This method implements
     * the multicast convergence algorithm described in RFC
     * 2608.
     */
    short readResponse()
	throws IOException {
	byte[] buf = new byte[_conf.getMTU()];
	DatagramPacket p = new DatagramPacket(buf, _conf.getMTU());
	long started = System.currentTimeMillis();
	int failCount = 0;
	short errorCode = 0;
	boolean gotResponse = false;

	while(_received.size() == 0  // Received something
	      && (_message.isMcast() || !gotResponse) // DA returned no results
	      && started + _maxWait > System.currentTimeMillis() // Request timeout
	      && failCount < 2) {   // Two consecutive non-responses
	    try {
		_uaSock.setSoTimeout(_message.nextTimeout());
		_uaSock.receive(p);

		// Check xid
		short xid = (short)((p.getData()[10] << 8) |
				    (p.getData()[11] & 0xff));

		if(xid == _message.getXid()) {
		    if(_net.traceDA.isDebugEnabled() && !_message.isMcast()) {
			_net.traceDA.debug("Received response from DA with XID " + xid);
		    }
		    failCount = 0;
		    gotResponse = true;
		    _message.addResponder(p.getAddress().getHostAddress());
		    errorCode = parseResponse(p.getData());
		    removeDups();
		}
		else {
		    _net.traceDrop.warn("Dropping message with XID " + xid);
		}
	    }
	    catch(InterruptedIOException e) {
		failCount++;
		transmitDatagram();
	    }
	}

	if(!gotResponse && !_message.isMcast()) {
	    // If no response from DA, it may be down.
	    _net.findDA();
	    return readResponse();
	}

	return errorCode;
    }

    /**
     * Removes duplicate entries from the list of responses.
     */
    private void removeDups() {
	Iterator iter = _delivered.iterator();
	while(iter.hasNext()) {
	    Object o;
	    if(_received.contains(o=iter.next())) {
		_received.remove(o);
	    }
	}
    }

    /**
     * Parses a datagram packet's buffer, adding responses
     * to the list of received values.
     */
    private short parseResponse(byte[] buf)
	throws IOException {

	if(buf == null) {
	    return 0;
	}

	ByteArrayInputStream bis = new ByteArrayInputStream(buf);
	DataInputStream dis = new DataInputStream(bis);

	dis.readByte();
	int functionID = dis.readByte();
	dis.skipBytes(3);
	byte flags = dis.readByte();
	boolean overflow = false;
	if((flags & 0x80) != 0) {
	    overflow = true;
	}

	dis.skipBytes(6);
	int len = dis.readShort() & 0xFFFF;
	dis.skipBytes(len);

	short errorCode = dis.readShort();

	int size;

	switch(functionID) {
	case SLPMessage.DAADVERT:
	    // DA Advertisement
	    dis.skipBytes(4);
	    size = dis.readShort() & 0xFFFF;
	    byte[] host = new byte[size];
	    dis.read(host);
	    _received.add(new String(host));
	    break;

	case SLPMessage.SRVRPLY:
	    // Service Reply
	    size = dis.readShort() & 0xFFFF;
	    for(int i=0;i<size;i++) {
		ServiceURL result = new ServiceURL();
		try {
		    result.readExternal(dis);
		    if(result.verify()) {
			_received.add(result);
		    }
		}
		catch(EOFException e) {
		    if(overflow) {
			break;
		    }
		    else {
			throw e;
		    }
		}
	    }
	    break;

	case SLPMessage.ATTRRPLY:
	    // Attribute Reply
	    size = dis.readShort() & 0xFFFF;
	    byte[] attrs = new byte[size];
	    dis.read(attrs);
	    String list = new String(attrs);
	    AuthenticationBlock[] authBlocks = new AuthenticationBlock[dis.readByte()];
	    for(int j=0;j<authBlocks.length;j++) {
		authBlocks[j] = new AuthenticationBlock();
		authBlocks[j].readBlock(dis);
	    }
	    if(ServiceLocationAttribute.verifyList(list, authBlocks)) {
		Vector v = ServiceLocationAttribute.readFromList(list);
		Iterator i = v.iterator();
		while(i.hasNext()) {
		    ServiceLocationAttribute sla =
			(ServiceLocationAttribute)i.next();
		    _received.add(sla);
		}
	    }
	    break;

	case SLPMessage.SRVTYPERPLY:
	    // Service Type Reply
	    size = dis.readShort() & 0xFFFF;
	    byte[] types = new byte[size];
	    dis.read(types);
	    StringTokenizer st = new StringTokenizer(new String(types), ",");
	    while(st.hasMoreElements()) {
		_received.add(new ServiceType(st.nextToken()));
	    }
	    break;
	}
	return errorCode;
    }

    /**
     * Frees resources allocated by this enumeration.
     */
    public void destroy() {
	if(uaBos != null) {
	    try {
		uaBos.close();
	    }
	    catch(IOException e) {
	    }
	    uaBos = null;
	}
	if(_uaSock != null) {
	    _uaSock.close();
	    _uaSock = null;
	}

    }
}
