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

import java.util.*;
import java.io.*;

/**
 * SLPMessage is the base for all messages sent from
 * the SLP client.
 *
 * @author Patrick Callis
 */
abstract class SLPMessage {
    private Locale _locale;
    private byte _msgType;
    private short _xid;

    public static final byte SRVRQST =     1;
    public static final byte SRVRPLY =     2;
    public static final byte SRVREG =      3;
    public static final byte SRVDEREG =    4;
    public static final byte SRVACK =      5;
    public static final byte ATTRRQST =    6;
    public static final byte ATTRRPLY =    7;
    public static final byte DAADVERT =    8;
    public static final byte SRVTYPERQST = 9;
    public static final byte SRVTYPERPLY = 10;
    public static final byte SAADVERT =    11;

    SLPMessage(Locale locale, byte msgType) {
	_locale = locale;
	_msgType = msgType;
    }

    /**
     * Sets the XID that this message was sent with.
     */
    void setXid(short xid) {
	_xid = xid;
    }

    /**
     * Gets the XID this message was sent with.
     */
    short getXid() {
	return _xid;
    }

    boolean writeMessage(DataOutput out) throws IOException {
	if(writeHeader(out, calcSize())) {
	    writeBody(out);
	    return true;
	}
	else
	    return false;
    }

    /**
     * Implemented by subclasses to return the size required
     * by the body of the message.
     */
    protected abstract int calcSize();

    /**
     * Implemented by subclasses to write the message body to
     * a DataOutput.  The number of bytes written should be
     * equal to calcSize()
     */
    protected abstract void writeBody(DataOutput out) throws IOException;

    /**
     * Writes the standard SLP header to a DataOutput.
     */
    private boolean writeHeader(DataOutput out, int bodySize)
	throws IOException {
	byte[] lang = _locale.toString().getBytes();
	int msgSize = 14 + lang.length + bodySize;
	if(msgSize > ServiceLocationManager.getConfiguration().getMTU()) {
	    return false;
	}
	// Version (0)
	out.write(2);
	// Function id (1)
	out.write(_msgType);
	// length (2 - 4)
	out.write((byte)((msgSize) >> 16));
	out.write((byte)(((msgSize) >> 8) & 0xFF));
	out.write((byte)((msgSize) & 0xFF));
	// flags (5)
	out.write((byte)0x40);
	// reserved (6)
	out.write(0);
	// ext offset (7 - 9)
	out.write(0);
	out.write(0);
	out.write(0);
	// xid (10 - 11)   // Just leave this blank and fill in later
	out.writeShort(0);
	// lang tag length (12 - 13)
	out.writeShort((short)lang.length);
	// lang tag (14 - ?)
	out.write(lang);
	return true;
    }

    public abstract String toString();
}
