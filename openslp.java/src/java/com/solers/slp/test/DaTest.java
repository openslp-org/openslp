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

import java.net.*;
import java.io.*;

public class DaTest {

    public static void main(String[] args) throws Exception {
	MulticastSocket sock = new MulticastSocket(427);

	sock.joinGroup(InetAddress.getByName("239.255.255.253"));

	byte[] buf = new byte[1400];
	DatagramPacket p = new DatagramPacket(buf, buf.length);
	sock.receive(p);
	System.out.println("Got something!");
    }
}
