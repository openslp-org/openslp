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

public class ListAll {

    public static void main(String[] args) throws Exception {
	org.apache.log4j.BasicConfigurator.configure();
	org.apache.log4j.Category.getRoot().setPriority(org.apache.log4j.Priority.WARN);
	Properties p = new Properties();
	p.put("net.slp.traceDATraffic", "false");
	p.put("net.slp.traceMsg", "false");
	p.put("net.slp.traceDrop", "true");
	p.put("net.slp.traceReg", "false");


    // The following are necessary for secure use
	// p.put("net.slp.securityEnabled", "true");
    // p.put("net.slp.publicKey.myspi", "path to der public key");
	// p.put("net.slp.spi", "myspi");
	ServiceLocationManager.init(p);

	Locator locator = ServiceLocationManager.getLocator(java.util.Locale.getDefault());
	Vector scopes = new Vector();
	scopes.add("default");

	ServiceLocationEnumeration enum = locator.findServiceTypes("*", scopes);
	while(enum.hasMoreElements()) {
	    ServiceType type = (ServiceType)enum.next();
	    System.out.println(type.toString());
	    ServiceLocationEnumeration enum2 = locator.findServices(type, scopes, "");
	    while(enum2.hasMoreElements()) {
		ServiceURL url = (com.solers.slp.ServiceURL)enum2.next();
		System.out.println("\t" + url.toString());
		ServiceLocationEnumeration enum3 =
		    locator.findAttributes(url, scopes, new Vector());
		while(enum3.hasMoreElements()) {
		    System.out.println("\t\t" + enum3.next().toString());
		}
		enum3.destroy();
	    }
	    enum2.destroy();
	}
	enum.destroy();
    }
}
