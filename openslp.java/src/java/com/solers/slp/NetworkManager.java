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
import java.net.*;
import java.io.*;
import org.apache.log4j.Category;
import org.apache.log4j.Priority;

/**
 * NetworkManager is a singleton which controls the
 * sending and receiving of datagram packets for UAs,
 * and the TCP connection for SAs.
 *
 * @author Patrick Callis
 */
class NetworkManager {
    /** Default logging category */
    Category cat = Category.getInstance(NetworkManager.class.getName());
    /** DA Traffic logging category */
    Category traceDA = Category.getInstance("net.slp.traceDATraffic");
    /** Message logging category */
    Category traceMsg = Category.getInstance("net.slp.traceMsg");
    /** Drop logging category */
    Category traceDrop = Category.getInstance("net.slp.traceDrop");
    /** Reg logging category */
    Category traceReg = Category.getInstance("net.slp.traceReg");

    static final int    SLP_RESERVED_PORT = 427;
    static final String SLP_MCAST_ADDRESS = "239.255.255.253";
    static final String SLP_DA_TYPE =       "service:directory-agent";

    private static NetworkManager _singleton;
    private static short nextXid;

    /** Used for writing SA messages */
    private ByteArrayOutputStream saBos;
    /** Wrapper for ByteArrayOutputStream */
    private DataOutputStream saDos;

    /** List of known DA urls */
    private Vector daAddresses = new Vector();

    private SLPConfiguration _conf;

    private NetworkManager() {
	_conf = ServiceLocationManager.getConfiguration();

	StringTokenizer das = new StringTokenizer(_conf.getDaAddresses(), ",");
	while(das.hasMoreElements()) {
	    daAddresses.add(das.nextToken());
	}

	if(_conf.getTraceDaTraffic()) {
	    traceDA.setPriority(Priority.DEBUG);
	}
	if(_conf.getTraceMessage()) {
	    traceMsg.setPriority(Priority.DEBUG);
	}
	if(_conf.getTraceDrop()) {
	    traceDrop.setPriority(Priority.DEBUG);
	}
	if(_conf.getTraceReg()) {
	    traceReg.setPriority(Priority.DEBUG);
	}

	saBos = new ByteArrayOutputStream(_conf.getMTU());
	saDos = new DataOutputStream(saBos);

	nextXid = (short)Math.round(Math.random() * Short.MAX_VALUE);

	findDA();
    }

    /**
     * Returns the singleton instance of a NetworkManager
     */
    synchronized static NetworkManager getInstance() {
	if(_singleton == null) {
	    _singleton = new NetworkManager();
	}
	return _singleton;
    }

    /**
     * Opens a TCP connection to the slp daemon running on
     * the localhost.  This connection is used for all SA
     * requests/responses.
     */
    Socket connectToSlpd() throws IOException {
	return new Socket("localhost", SLP_RESERVED_PORT);
    }

    /**
     * Searches for a DA
     */
    void findDA() {
	try {
	    daAddresses.clear();
	    UAMessage msg = new ServiceRequest(new ServiceType(SLP_DA_TYPE),
					       _conf.getScopes(), "",
					       Locale.getDefault());

	    ServiceLocationEnumeration enum =
		new ServiceLocationEnumerationImpl(this, msg);
	    if(enum.hasMoreElements()) {
		String da = ((String)enum.next()).substring(26);

		synchronized(this) {
		    daAddresses.add(da);
		}
	    }
	    enum.destroy();
	}
	catch(Exception e) {
	    cat.info("Caught an exception discovering DA", e);
	}
    }

    /**
     * Returns either the current DA's address, or the
     * SLP multicast address if no DAs are available.
     */
    synchronized InetAddress getDaAddress() {
	while(daAddresses.size() > 0) {
	    try {
		return InetAddress.getByName((String)daAddresses.get(0));
	    }
	    catch(Exception e) {
		daAddresses.remove(0);
	    }
	}

	try {
	    return InetAddress.getByName(SLP_MCAST_ADDRESS);
	}
	catch(UnknownHostException e) {
	    return null;
	}
    }

    /**
     * Returns the next XID to use.
     */
    synchronized short nextXid() {
	return nextXid++;
    }

    /**
     * Sends a message as an SA. SA Messages are to the slp daemon
     * running on the localhost.  Note that this is a TCP connection.
     */
    synchronized void saMessage(SLPMessage message) throws ServiceLocationException {
	if(traceMsg.isDebugEnabled()) {
	    traceMsg.debug("Sending: " + message.toString());
	}

	Socket saSock = null;
	try {
	    message.setXid(nextXid++);
	    DataInputStream istream = null;
	    DataOutputStream ostream = null;
	    try {
		saSock = connectToSlpd();
		ostream = new DataOutputStream(saSock.getOutputStream());
		istream = new DataInputStream(saSock.getInputStream());
	    }
	    catch(IOException e) {
		throw new
                ServiceLocationException("Cannot connect to slpd. " +
					     "SAs must have slpd running on " +
					     "the localhost.",
					     ServiceLocationException.NETWORK_ERROR);
	    }

	    saBos.reset();
	    message.writeMessage(saDos);
	    byte[] buf = saBos.toByteArray();
	    buf[5] = (byte)(buf[5] & 0xDF);
	    buf[10] = (byte)(message.getXid() >> 8);
	    buf[11] = (byte)(message.getXid() & 0xFF);
	    ostream.write(buf);
	    ostream.flush();
	    short errorCode = saReadAck(message.getXid(), istream);
	    if(errorCode != 0) {
		throw new ServiceLocationException("Error sending SA Message", errorCode);
	    }

	    if(traceReg.isDebugEnabled()) {
		if(message instanceof ServiceRegistration) {
		    traceReg.debug("Sent registration to slpd.");
		}
		else if(message instanceof ServiceDeregistration) {
		    traceReg.debug("Sent deregistration to slpd.");
		}

	    }
	}
	catch(IOException e) {
	    cat.error("Could not send SLP message", e);
	    throw new ServiceLocationException("Could not send message",
					       ServiceLocationException.NETWORK_ERROR);
	}
	finally {
	    if(saSock != null) {
		try {
		    saSock.close();
		}
		catch(Exception e) {
		}
		saSock = null;
	    }
	}
    }

    /**
     * Reads the ack response to an SA message.
     */
    private short saReadAck(short xid, DataInputStream istream) throws IOException {
	byte version = istream.readByte();

	byte functionID = istream.readByte();

	if(functionID != SLPMessage.SRVACK) {
	    // This should never happen
	    cat.error("Received non-ack on loopback");
	}

	istream.skip( 8 );
	short recvXid = istream.readShort();

	if(recvXid != xid) {
	    // Again, this shouldn't happen.
	    cat.error("Wrong XID on loopback");
	}

	int langtagLen = istream.readShort() & 0xFFFF;
	istream.skip(langtagLen);

	// Return the error code
	return istream.readShort();
    }
}
