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

public class FullTest {

    private class SA extends Thread {
	private int _iter;
	private int _lifetime;

	public SA(int iter, int lifetime) {
	    _iter = iter;
	    _lifetime = lifetime;
	}

	public void run() {
	    try {

		Advertiser adv =
		    ServiceLocationManager.getAdvertiser(java.util.Locale.getDefault());

		for(int i=0;i<_iter;i++) {
		    Vector attr = new Vector();
		    Vector values = new Vector();
		    values.add(new Integer((int)Math.round(Math.random() * Integer.MAX_VALUE)));
		    attr.add(new ServiceLocationAttribute("Attribute", values));
		
		    ServiceURL url = new ServiceURL("service:slpTest" + (i%5) + "://" +
						    (i%255) + "." + (i%255) + "." +
						    (i%255) + "." + (i%255) + ":" +
						    i,
						    _lifetime);

		    adv.register(url,attr);
		}
	    }
	    catch(Exception e) {
		e.printStackTrace();
		System.exit(1);
	    }
	}
    }

    private class UA extends Thread {
	private int _iter;

	public UA(int iter) {
	    _iter = iter;
	}

	public void run() {
	    try {
		Locator locator =
		    com.solers.slp.ServiceLocationManager.getLocator(java.util.Locale.getDefault());
		Vector scopes = new Vector();
		scopes.add("default");

		for(int i=0;i<_iter;i++) {
		    ServiceLocationEnumeration enum = locator.findServiceTypes("", scopes);
		    while(enum.hasMoreElements()) {
			ServiceType type = (ServiceType)enum.next();
			System.out.println(type.toString());
			ServiceLocationEnumeration enum2 = locator.findServices(type, scopes, "");
			while(enum2.hasMoreElements()) {
			    ServiceURL url = (ServiceURL)enum2.next();
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
	    catch(Exception e) {
		e.printStackTrace();
		System.exit(1);
	    }
	}

    }

    private FullTest(int iter, int lifetime) {
	for(int i=0;i<10;i++) {
	    new SA(iter, lifetime).start();
	}
	
	for(int i=0;i<10;i++) {
	    new UA(iter).start();
	}
    }

    public static void main(String[] args) {
	org.apache.log4j.BasicConfigurator.configure();
	org.apache.log4j.Category.getRoot().setPriority(org.apache.log4j.Priority.WARN);
	Properties p = new Properties();
	p.put("net.slp.traceDATraffic", "false");
	p.put("net.slp.traceMsg", "false");
	p.put("net.slp.traceDrop", "true");
	p.put("net.slp.traceReg", "false");

    // The following are necessary for secure use
	// p.put("net.slp.securityEnabled", "true");
	// p.put("net.slp.privateKey.myspi", "path to pk8 private key");
    // p.put("net.slp.publicKey.myspi", "path to der public key");
	// p.put("net.slp.spi", "myspi");

	com.solers.slp.ServiceLocationManager.init(p);

	if(args.length != 2) {
	    System.out.println("usage: FullTest iterations lifetime");
	    System.exit(1);
	}

	new FullTest(Integer.parseInt(args[0]), Integer.parseInt(args[1]));
    }

}
