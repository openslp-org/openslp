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

import java.io.DataInputStream;
import java.io.IOException;
import java.net.Socket;
import java.util.Properties;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;

import com.solers.slp.Locator;
import com.solers.slp.ServiceLocationEnumeration;
import com.solers.slp.ServiceLocationManager;
import com.solers.slp.ServiceType;
import com.solers.slp.ServiceURL;

public class SlpUA {

	public static void main(String[] args) throws Exception {
		String attribute = null;

		if (args.length > 0) {
			attribute = args[0];
		}

		Logger log = Logger.getLogger(ServiceLocationManager.getLoggerName());
		log.setLevel(Level.WARNING);
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

		Locator loc = ServiceLocationManager.getLocator(java.util.Locale
				.getDefault());
		Vector scopes = new Vector();
		scopes.add("default");

		String selector = "";
		if (attribute != null) {
			selector += "(Attribute1=" + attribute + ")";
		}

		ServiceLocationEnumeration e = loc.findServices(new ServiceType(
				"service:slpTest"), scopes, selector);

		ServiceURL sUrl = null;

		boolean connected = false;
		while (!connected && e.hasMoreElements()) {
			sUrl = (com.solers.slp.ServiceURL) e.next();

			if (sUrl != null) {
				System.out.println("Found URL: " + sUrl.toString());
				try {
					Socket s = new Socket(sUrl.getHost(), sUrl.getPort());
					DataInputStream in = new DataInputStream(s.getInputStream());
					System.out.println(in.readUTF());
					connected = true;
					in.close();
				} catch (IOException ioe) {
					System.err.println("Caught IOException: " + ioe);
					ioe.printStackTrace();
				}
			}
		}

		e.destroy();
		if (!connected) {
			System.out.println("Could not connect");
		}
	}
}
