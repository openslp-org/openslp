/*
 * Copyright 2003 Solers Corporation.  All rights reserved.
 *
 * Modification and use of this SLP API software and
 * associated documentation ("Software") is permitted provided that the
 * conditions specified in the license.txt file included within this
 * distribution are met.
 *
 */

package com.solers.slp.test;

import com.solers.slp.*;

import java.util.*;
import java.io.*;
import java.net.*;

public class SlpUA {

    public static void main(String[] args) throws Exception {
	String attribute = null;

	if(args.length > 0) {
	    attribute = args[0];
	}

	org.apache.log4j.BasicConfigurator.configure();
	org.apache.log4j.Category.getRoot().setPriority(org.apache.log4j.Priority.WARN);
	Properties p = new Properties();
	p.put("net.slp.traceDATraffic", "true");
	p.put("net.slp.traceMsg", "true");
	p.put("net.slp.traceDrop", "true");
	p.put("net.slp.traceReg", "true");
    // The following are necessary for secure use
	// p.put("net.slp.securityEnabled", "true");
    // p.put("net.slp.publicKey.myspi", "path to der public key");
	// p.put("net.slp.spi", "myspi");
	ServiceLocationManager.init(p);

	Locator loc = ServiceLocationManager.getLocator(java.util.Locale.getDefault());
	Vector scopes = new Vector();
	scopes.add("default");

	String selector = "";
	if(attribute != null) {
	    selector += "(Attribute1=" + attribute + ")";
	}

	ServiceLocationEnumeration enum =
	    loc.findServices(new ServiceType("service:slpTest"), scopes, selector);

	ServiceURL sUrl = null;

	boolean connected = false;
	while(!connected && enum.hasMoreElements()) {
	    sUrl = (com.solers.slp.ServiceURL)enum.next();

	    if(sUrl != null) {
		System.out.println("Found URL: " + sUrl.toString());
		try {
		    Socket s = new Socket(sUrl.getHost(), sUrl.getPort());
		    DataInputStream in = new DataInputStream(s.getInputStream());
		    System.out.println(in.readUTF());
		    connected = true;
		    in.close();
		}
		catch(IOException e) {
		}
	    }
	}

	enum.destroy();
	if(!connected) {
	    System.out.println("Could not connect");
	}
    }

}
