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

import java.util.Locale;
import java.util.Vector;
import java.util.logging.Level;
import java.util.logging.Logger;

import com.solers.slp.Advertiser;
import com.solers.slp.Locator;
import com.solers.slp.ServiceLocationEnumeration;
import com.solers.slp.ServiceLocationManager;
import com.solers.slp.ServiceType;
import com.solers.slp.ServiceURL;

public class RecoverTest {
	public static void main(String[] args) throws Exception {
		Logger log = Logger.getLogger(ServiceLocationManager.getLoggerName());
		log.setLevel(Level.WARNING);

		Advertiser adv = ServiceLocationManager.getAdvertiser(Locale
				.getDefault());
		adv.register(new ServiceURL("service:slpTest://1.1.1.1", 45),
				new Vector());

		Locator loc = ServiceLocationManager.getLocator(Locale.getDefault());

		Vector scopes = new Vector();
		scopes.add("DEFAULT");
		ServiceLocationEnumeration e = loc.findServices(new ServiceType(
				"service:slpTest"), scopes, "");
		System.out.println(e.next());
		System.out.println("Kill DA now.");
		Thread.sleep(15000);
		System.out.println("Woke up");
		ServiceLocationEnumeration enum2 = loc.findServices(
				new com.solers.slp.ServiceType("service:slpTest"), scopes, "");

		System.out.println(enum2.next());
	}
}
