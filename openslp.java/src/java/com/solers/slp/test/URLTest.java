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

import com.solers.slp.ServiceLocationException;
import com.solers.slp.ServiceType;
import com.solers.slp.ServiceURL;

public class URLTest {

    public static void main(String[] args) {

	ServiceURL url = null;
        try {
            url = new ServiceURL("service:slpTest://localhost:1234/a/b/c.html", 50);
        }
        catch (ServiceLocationException ex) {
            System.err.println (ex);
            System.exit (-1);
        }



	ServiceType type = url.getServiceType();

	System.out.println("isServiceURL: " + type.isServiceURL());
	System.out.println("isAbstractType: " + type.isAbstractType());
	System.out.println("isNADefault: " + type.isNADefault());
	System.out.println("ConcreteTypeName: " + type.getConcreteTypeName());
	System.out.println("PrincipleTypeName: " + type.getPrincipleTypeName());
	System.out.println("AbstractTypeName: " + type.getAbstractTypeName());
	System.out.println("NamingAuthority: " + type.getNamingAuthority());
	System.out.println("Type: " + type.toString());

	System.out.println("Transport: " + url.getTransport());
	System.out.println("Host: " + url.getHost());
	System.out.println("Port: " + url.getPort());
	System.out.println("URLPath: " + url.getURLPath());
	System.out.println("Lifetime: " + url.getLifetime());
	System.out.println("URL: " + url.toString());
    }
}
