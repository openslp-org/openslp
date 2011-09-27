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

import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.Locale;
import java.util.StringTokenizer;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * NetworkManager is a singleton which controls the sending and receiving of
 * datagram packets for UAs, and the TCP connection for SAs.
 * 
 * @author Patrick Callis
 */
class NetworkManager {
	/** Default logging */
	private Logger mLogger;

	static final int SLP_RESERVED_PORT = 427;
	static final String SLP_MCAST_ADDRESS = "239.255.255.253";
	static final String SLP_DA_TYPE = "service:directory-agent";

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
		mLogger = Logger.getLogger(ServiceLocationManager.getLoggerName());

		StringTokenizer das = new StringTokenizer(_conf.getDaAddresses(), ",");
		while (das.hasMoreElements()) {
			daAddresses.add(das.nextToken());
		}

		saBos = new ByteArrayOutputStream(_conf.getMTU());
		saDos = new DataOutputStream(saBos);

		nextXid = (short) Math.round(Math.random() * Short.MAX_VALUE);

		findDA();
	}

	/**
	 * Returns the singleton instance of a NetworkManager
	 */
	synchronized static NetworkManager getInstance() {
		if (_singleton == null) {
			_singleton = new NetworkManager();
		}
		return _singleton;
	}

	/**
	 * Opens a TCP connection to the slp daemon running on the localhost. This
	 * connection is used for all SA requests/responses.
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
					_conf.getScopes(), "", Locale.getDefault());

			ServiceLocationEnumeration e = new ServiceLocationEnumerationImpl(
					this, msg);
			if (e.hasMoreElements()) {
				String da = ((String) e.next()).substring(26);

				synchronized (this) {
					daAddresses.add(da);
				}
			}
			e.destroy();
		} catch (Exception e) {
			mLogger.log(Level.SEVERE, "Caught an exception discovering DA", e);
		}
	}

	/**
	 * Returns either the current DA's address, or the SLP multicast address if
	 * no DAs are available.
	 */
	synchronized InetAddress getDaAddress() {
		while (daAddresses.size() > 0) {
			try {
				return InetAddress.getByName((String) daAddresses.get(0));
			} catch (Exception e) {
				daAddresses.remove(0);
			}
		}

		try {
			return InetAddress.getByName(SLP_MCAST_ADDRESS);
		} catch (UnknownHostException e) {
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
	 * Sends a message as an SA. SA Messages are to the slp daemon running on
	 * the localhost. Note that this is a TCP connection.
	 */
	synchronized void saMessage(SLPMessage message)
			throws ServiceLocationException {
		if (_conf.getTraceMessage()) {
			mLogger.info("Sending: " + message.toString());
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
			} catch (IOException e) {
				throw new ServiceLocationException("Cannot connect to slpd. "
						+ "SAs must have slpd running on " + "the localhost.",
						ServiceLocationException.NETWORK_ERROR);
			}

			saBos.reset();
			message.writeMessage(saDos);
			byte[] buf = saBos.toByteArray();
			buf[5] = (byte) (buf[5] & 0xDF);
			buf[10] = (byte) (message.getXid() >> 8);
			buf[11] = (byte) (message.getXid() & 0xFF);
			ostream.write(buf);
			ostream.flush();
			short errorCode = saReadAck(message.getXid(), istream);
			if (errorCode != 0) {
				throw new ServiceLocationException("Error sending SA Message",
						errorCode);
			}

			if (_conf.getTraceReg()) {
				if (message instanceof ServiceRegistration) {
					mLogger.info("Sent registration to slpd.");
				} else if (message instanceof ServiceDeregistration) {
					mLogger.info("Sent deregistration to slpd.");
				}

			}
		} catch (IOException e) {
			mLogger.log(Level.SEVERE, "Could not send SLP message", e);
			throw new ServiceLocationException("Could not send message",
					ServiceLocationException.NETWORK_ERROR);
		} finally {
			if (saSock != null) {
				try {
					saSock.close();
				} catch (Exception e) {
				}
				saSock = null;
			}
		}
	}

	/**
	 * Reads the ack response to an SA message.
	 */
	private short saReadAck(short xid, DataInputStream istream)
			throws IOException {
		// Don't read the version
		istream.readByte();

		byte functionID = istream.readByte();

		if (functionID != SLPMessage.SRVACK) {
			// This should never happen
			mLogger.log(Level.SEVERE, "Received non-ack on loopback");
		}

		istream.skip(8);
		short recvXid = istream.readShort();

		if (recvXid != xid) {
			// Again, this shouldn't happen.
			mLogger.log(Level.SEVERE, "Wrong XID on loopback");
			;
		}

		int langtagLen = istream.readShort() & 0xFFFF;
		istream.skip(langtagLen);

		// Return the error code
		return istream.readShort();
	}
}
