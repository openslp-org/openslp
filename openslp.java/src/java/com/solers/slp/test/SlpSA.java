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

import com.solers.slp.Advertiser;
import com.solers.slp.ServiceLocationAttribute;
import com.solers.slp.ServiceLocationManager;
import com.solers.slp.ServiceURL;

import java.net.*;
import java.io.*;
import java.util.*;

public class SlpSA {

    public static void main(String[] args) throws Exception {
	if(args.length < 1) {
	    System.out.println("usage: SlpSA port <attr val>");
	    System.exit(1);
	}
	int port = Integer.parseInt(args[0]);
	String attribute = null;
	if(args.length > 1) {
	    attribute = args[1];
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
	// p.put("net.slp.privateKey.myspi", "path to pk8 private key");
	// p.put("net.slp.spi", "myspi");

	ServiceLocationManager.init(p);

	Advertiser adv = ServiceLocationManager.getAdvertiser(java.util.Locale.getDefault());

	Vector attr = new Vector();
	if(attribute != null) {
	    Vector values = new Vector();
	    values.add(attribute);
	    attr.add(new ServiceLocationAttribute("Attribute1", values));
	}

	ServiceURL url = new ServiceURL("service:slpTest://" +
					InetAddress.getLocalHost().getHostAddress() +
					":" + port,
					60);

	adv.register(url, attr);

	ServerSocket ss = new ServerSocket(port);

	while(true) {
	    Socket s = ss.accept();
	    System.err.println("Accepted a connection");
	    DataOutputStream o = new DataOutputStream(s.getOutputStream());
	    o.writeUTF("Success");
	    o.close();
	}
    }

}
