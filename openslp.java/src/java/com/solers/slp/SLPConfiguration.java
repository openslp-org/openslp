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
 * An SLPConfiguration object holds all configurable properties.
 *
 * @author Patrick Callis
 */
class SLPConfiguration {
    Properties _props;

    public static final String USE_SCOPES = "net.slp.useScopes";
    public static final String USE_SCOPES_DEFAULT = "DEFAULT";

    public static final String DA_ADDRESSES = "net.slp.DAAddresses";
    public static final String DA_ADDRESSES_DEFAULT = "";

    public static final String TRACE_DATRAFFIC = "net.slp.traceDATraffic";
    public static final String TRACE_DATRAFFIC_DEFAULT = "false";

    public static final String TRACE_MESSAGE = "net.slp.traceMsg";
    public static final String TRACE_MESSAGE_DEFAULT = "false";

    public static final String TRACE_DROP = "net.slp.traceDrop";
    public static final String TRACE_DROP_DEFAULT = "false";

    public static final String TRACE_REG = "net.slp.traceReg";
    public static final String TRACE_REG_DEFAULT = "false";

    public static final String MCAST_TTL = "net.slp.multicastTTL";
    public static final String MCAST_TTL_DEFAULT = "255";

    public static final String MCAST_MAX_WAIT = "net.slp.multicastMaximumWait";
    public static final String MCAST_MAX_WAIT_DEFAULT = "15000";

    public static final String MCAST_TIMEOUTS = "net.slp.multicastTimeouts";
    public static final String MCAST_TIMEOUTS_DEFAULT = "3000,3000,3000,3000,3000";

    public static final String DATAGRAM_MAX_WAIT = "net.slp.datagramMaximumWait";
    public static final String DATAGRAM_MAX_WAIT_DEFAULT = "15000";

    public static final String DATAGRAM_TIMEOUTS = "net.slp.datagramTimeouts";
    public static final String DATAGRAM_TIMEOUTS_DEFAULT = "3000,3000,3000,3000,3000";

    public static final String MTU = "net.slp.MTU";
    public static final String MTU_DEFAULT = "1400";

    public static final String LOCALE = "net.slp.locale";
    public static final String LOCALE_DEFAULT = "en";

    public static final String SECURITY_ENABLED = "net.slp.securityEnabled";
    public static final String SECURITY_ENABLED_DEFAULT = "false";

    public static final String SPI = "net.slp.spi";
    public static final String SPI_DEFAULT = "";

    public static final String PRIVATE_KEY = "net.slp.privateKey.";
    public static final String PUBLIC_KEY = "net.slp.publicKey.";

    SLPConfiguration(Properties props) {
	_props = props;
    }

    SLPConfiguration(File f) throws IOException {
	BufferedReader in = new BufferedReader(new FileReader(f));
	_props = new Properties();

	String line = null;
	while((line = in.readLine()) != null) {
	    if(!(line.startsWith("#") || line.startsWith(";"))) {
		StringTokenizer st = new StringTokenizer(line, " \t\n\r\f=");
		if(st.countTokens() == 2) {
		    _props.put(st.nextToken(), st.nextToken());
		}
	    }
	}

	in.close();
    }

    String getScopes() {
	return _props.getProperty(USE_SCOPES, USE_SCOPES_DEFAULT);
    }

    String getDaAddresses() {
	return _props.getProperty(DA_ADDRESSES, DA_ADDRESSES_DEFAULT);
    }

    boolean getTraceDaTraffic() {
	return new Boolean(_props.getProperty(TRACE_DATRAFFIC,
					      TRACE_DATRAFFIC_DEFAULT)).booleanValue();
    }

    boolean getTraceMessage() {
	return new Boolean(_props.getProperty(TRACE_MESSAGE,
					      TRACE_MESSAGE_DEFAULT)).booleanValue();
    }

    boolean getTraceDrop() {
	return new Boolean(_props.getProperty(TRACE_DROP,
					      TRACE_DROP_DEFAULT)).booleanValue();
    }

    boolean getTraceReg() {
	return new Boolean(_props.getProperty(TRACE_REG,
					      TRACE_REG_DEFAULT)).booleanValue();
    }

    byte getMcastTTL() {
	return new Byte(_props.getProperty(MCAST_TTL, MCAST_TTL)).byteValue();
    }

    int getMcastMaxWait() {
	return Integer.parseInt(_props.getProperty(MCAST_MAX_WAIT,
						   MCAST_MAX_WAIT_DEFAULT));
    }

    int getDatagramMaxWait() {
	return Integer.parseInt(_props.getProperty(DATAGRAM_MAX_WAIT,
						   DATAGRAM_MAX_WAIT_DEFAULT));
    }

    int[] parseTimeouts(String s) {
	StringTokenizer st = new StringTokenizer(s, ",");
	int[] timeouts = new int[st.countTokens()];
	for(int i=0;i<timeouts.length;i++) {
	    timeouts[i] = Integer.parseInt(st.nextToken());
	}
	return timeouts;
    }

    int[] getMcastTimeouts() {
	return parseTimeouts(_props.getProperty(MCAST_TIMEOUTS, MCAST_TIMEOUTS_DEFAULT));
    }

    int[] getDatagramTimeouts() {
	return parseTimeouts(_props.getProperty(DATAGRAM_TIMEOUTS,
						DATAGRAM_TIMEOUTS_DEFAULT));
    }

    int getMTU() {
	return Integer.parseInt(_props.getProperty(MTU, MTU_DEFAULT));
    }

    Locale getLocale() {
	return new Locale(_props.getProperty(LOCALE, LOCALE_DEFAULT), "");
    }

    boolean getSecurityEnabled() {
	return new Boolean(_props.getProperty(SECURITY_ENABLED,
					      SECURITY_ENABLED_DEFAULT)).booleanValue();
    }

    String getSPI() {
	return _props.getProperty(SPI, SPI_DEFAULT);
    }

    String getPublicKey(String spi) {
	return _props.getProperty(PUBLIC_KEY + spi);
    }

    String getPrivateKey(String spi) {
	return _props.getProperty(PRIVATE_KEY + spi);
    }
}
