/*
 * Copyright 2003 Solers Corporation.  All rights reserved.
 *
 * Modification and use of this SLP API software and
 * associated documentation ("Software") is permitted provided that the
 * conditions specified in the license.txt file included within this
 * distribution are met.
 *
 */

package com.solers.slp;

import java.security.*;
import java.security.spec.*;
import java.io.*;
import org.apache.log4j.Category;

/**
 * Implementation of the SLP Authentication Block.
 *
 * @author Patrick Callis
 */
public class AuthenticationBlock {

    private static Category cat =
	Category.getInstance(AuthenticationBlock.class.getName());
    private PublicKey _publicKey = null;
    private PrivateKey _privateKey = null;
    private int _timestamp;
    private byte[] _data = null;
    private SLPConfiguration _conf;
    private byte[] _sig = null;
    private String _spi = null;

    AuthenticationBlock(short bsd, String spi, int timestamp,
			byte[] data, byte[] signature)
	throws ServiceLocationException {
	this();

	if(bsd != 0x0002) {
	    throw new ServiceLocationException("Only BSD 0x0002 (DSA) is supported.",
					       ServiceLocationException.NOT_IMPLEMENTED);
	}

	_timestamp = timestamp;
	_data = data;
	_spi = spi;

	if(signature == null) {
	    sign();
	}
	else {
	    _sig = signature;
	}
    }

    AuthenticationBlock() {
	_conf = ServiceLocationManager.getConfiguration();
    }

    private void sign() throws ServiceLocationException {
	String keyLoc = _conf.getPrivateKey(_spi);
	if(cat.isDebugEnabled()) {
	    cat.debug("Signing with SPI: " + _spi);
	}

	try {
	    FileInputStream keyfis = new FileInputStream(keyLoc);

	    byte[] encKey = new byte[keyfis.available()];
	    keyfis.read(encKey);
	    keyfis.close();

	    PKCS8EncodedKeySpec privKeySpec = new PKCS8EncodedKeySpec(encKey);

	    KeyFactory keyFactory = KeyFactory.getInstance("DSA");
	    _privateKey = keyFactory.generatePrivate(privKeySpec);

	    Signature signature = Signature.getInstance("SHA1withDSA");
	    signature.initSign(_privateKey);
	    signature.update(_data);
	    _sig = signature.sign();
	}
	catch(Exception e) {
	    e.printStackTrace();
	    throw new ServiceLocationException("Could not sign data",
					  ServiceLocationException.AUTHENTICATION_FAILED);
	}
    }

    String getSPI() {
	return _spi;
    }

    int getTimestamp() {
	return _timestamp;
    }

    boolean verify(byte[] data) throws ServiceLocationException {
	try {
	    if(_publicKey == null) {
		String keyLoc = _conf.getPublicKey(_spi);
		FileInputStream keyfis = new FileInputStream(keyLoc);
		byte[] encKey = new byte[keyfis.available()];
		keyfis.read(encKey);
		keyfis.close();
		X509EncodedKeySpec pubKeySpec = new X509EncodedKeySpec(encKey);
		KeyFactory keyFactory = KeyFactory.getInstance("DSA");
		_publicKey = keyFactory.generatePublic(pubKeySpec);
	    }

	    Signature signature = Signature.getInstance("SHA1withDSA");
	    signature.initVerify(_publicKey);
	    signature.update(data);
	    boolean success = signature.verify(_sig);
	    if(cat.isDebugEnabled()) {
		if(success) {
		    cat.debug("Verified with SPI: " + _spi);
		}
		else {
		    cat.debug("Verification failed with SPI: " + _spi);
		}
	    }

	    return success;
	}
	catch(Exception e) {
	    e.printStackTrace();
	    throw new ServiceLocationException("Could not verify data with SPI: " + _spi,
					  ServiceLocationException.AUTHENTICATION_FAILED);
	}
    }

    int calcSize() {
	return 2  // BSD
	    + 2   // Block length
	    + 4   // timestamp
	    + 2   // spi length
	    + _spi.getBytes().length // spi
	    + _sig.length;  // signature
    }

    void writeBlock(DataOutput out) throws IOException {
	out.writeShort(0x0002);   // BSD
	out.writeShort((short)calcSize());
	out.writeInt(_timestamp);

	byte[] temp = _spi.getBytes();
	out.writeShort(temp.length);
	out.write(temp);
	out.write(_sig);
    }

    void readBlock(DataInput in) throws IOException {
	short bsd = in.readShort();
	int size = in.readShort() & 0xFFFF;
	_timestamp = in.readInt();
	int spilen = in.readShort() & 0xFFFF;
	byte[] spibytes = new byte[spilen];
	in.readFully(spibytes);
	_spi = new String(spibytes);
	_sig = new byte[size - 2 - 2 - 4 - 2 - spilen];
	in.readFully(_sig);
    }
}
