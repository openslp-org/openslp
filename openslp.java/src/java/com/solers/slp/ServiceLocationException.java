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

/**
 * Implementation of the ServiceLocationException class defined
 * in RFC 2614.
 *
 * A ServiceLocationException is thrown by all methods when
 * errors occur.
 */
public class ServiceLocationException extends Exception {
    public static final short LANGUAGE_NOT_SUPPORTED = 1;
    public static final short PARSE_ERROR = 2;
    public static final short INVALID_REGISTRATION = 3;
    public static final short SCOPE_NOT_SUPPORTED = 4;
    public static final short AUTHENTICATION_ABSENT = 6;
    public static final short AUTHENTICATION_FAILED = 7;
    public static final short INVALID_UPDATE = 13;
    public static final short REFRESH_REJECTED = 15;
    public static final short NOT_IMPLEMENTED = 16;
    public static final short NETWORK_INIT_FAILED = 17;
    public static final short NETWORK_TIMED_OUT = 18;
    public static final short NETWORK_ERROR = 19;
    public static final short INTERNAL_SYSTEM_ERROR = 20;
    public static final short TYPE_ERROR = 21;
    public static final short BUFFER_OVERFLOW = 22;

    private short code;

    public ServiceLocationException(String why, short errorCode) {
	super(why);
	code = errorCode;
    }

    /**
     * Return the error code.  The error code takes on one of the static
     * field values.
     */
    public short getErrorCode() {
	return code;
    }

    public String toString() {
	return super.toString() + " Error code: " + code;
    }
}
