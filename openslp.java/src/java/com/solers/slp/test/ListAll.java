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

import java.util.Properties;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;

import com.solers.slp.Locator;
import com.solers.slp.ServiceLocationEnumeration;
import com.solers.slp.ServiceLocationManager;
import com.solers.slp.ServiceType;
import com.solers.slp.ServiceURL;

public class ListAll {

	public static void main(String[] args) throws Exception {
		Logger log = Logger.getLogger(ServiceLocationManager.getLoggerName());
		log.setLevel(Level.WARNING);
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

		Locator locator = ServiceLocationManager.getLocator(java.util.Locale
				.getDefault());
		Vector scopes = new Vector();
		scopes.add("default");

		ServiceLocationEnumeration e = locator.findServiceTypes("*", scopes);
		while (e.hasMoreElements()) {
			ServiceType type = (ServiceType) e.next();
			System.out.println(type.toString());
			ServiceLocationEnumeration enum2 = locator.findServices(type,
					scopes, "");
			while (enum2.hasMoreElements()) {
				ServiceURL url = (com.solers.slp.ServiceURL) enum2.next();
				System.out.println("\t" + url.toString());
				ServiceLocationEnumeration enum3 = locator.findAttributes(url,
						scopes, new Vector());
				while (enum3.hasMoreElements()) {
					System.out.println("\t\t" + enum3.next().toString());
				}
				enum3.destroy();
			}
			enum2.destroy();
		}
		e.destroy();
	}
}
