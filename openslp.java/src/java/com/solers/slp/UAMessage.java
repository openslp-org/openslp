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

import com.solers.slp.SLPMessage;

import java.util.*;

/**
 * UAMessage is the base for User Agent messages
 *
 * @author Patrick Callis
 */
abstract class UAMessage extends SLPMessage {
    private long _sentAt = -1;
    private int _timesSent = 0;
    private int[] _schedule = null;
    private Vector _responders = null;
    private boolean _isMcast;

    UAMessage(Locale locale, byte msgType) {
	super(locale, msgType);
    }

    void setMcast(boolean mcast) {
	_isMcast = mcast;
    }

    boolean isMcast() {
	return _isMcast;
    }

    void sent() {
	_sentAt = System.currentTimeMillis();
	_timesSent++;
    }

    void setTransmitSchedule(int[] schedule) {
	if(schedule == null) {
	    throw new RuntimeException();
	}
	_schedule = schedule;
    }

    int nextTimeout() {
	if(_timesSent < _schedule.length ) {
	    int timeout =  (int)(_sentAt
			 - System.currentTimeMillis()
			 + _schedule[_timesSent-1]);
	    return (timeout > 0) ? timeout : 1; // 0 => infinite
	}
	else {
	    // No hints as to how long to wait, we'll use the
	    // default of 3000
	    return 3000;
	}
    }

    void addResponder(String responder) {
	if(_responders == null) {
	    _responders = new Vector();
	}
	_responders.add(responder);
    }

    protected String getResponders() {
	String resp = "";
	if(_responders != null) {
	    Iterator i = _responders.iterator();
	    while(i.hasNext()) {
		resp += (String)i.next();

		if(i.hasNext()) {
		    resp += ",";
		}
	    }
	}
	return resp;
    }



}
