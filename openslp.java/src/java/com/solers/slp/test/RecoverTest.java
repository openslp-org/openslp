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

import java.util.*;
import com.solers.slp.*;

public class RecoverTest {
    public static void main(String[] args) throws Exception {
	org.apache.log4j.BasicConfigurator.configure();
	org.apache.log4j.Category.getRoot().setPriority(org.apache.log4j.Priority.WARN);

	Advertiser adv = ServiceLocationManager.getAdvertiser(Locale.getDefault());
	adv.register(new ServiceURL("service:slpTest://1.1.1.1", 45), new Vector());

	Locator loc = ServiceLocationManager.getLocator(Locale.getDefault());

	Vector scopes = new Vector();
	scopes.add("DEFAULT");
	ServiceLocationEnumeration enum =
	    loc.findServices(new ServiceType("service:slpTest"), scopes, "");
	System.out.println(enum.next());
	System.out.println("Kill DA now.");
	Thread.sleep(15000);
	System.out.println("Woke up");
	ServiceLocationEnumeration enum2 =
	    loc.findServices(new com.solers.slp.ServiceType("service:slpTest"), scopes, "");

	System.out.println(enum2.next());
    }
}
