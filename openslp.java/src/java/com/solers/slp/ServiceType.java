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

import java.io.Serializable;

/**
 * Implementation of the SLP ServiceType class defined in RFC 2614.
 *
 * @author Patrick Callis
 */
public class ServiceType implements Serializable {

    private String _type;
    private boolean _service;
    private boolean _abstract;
    private String _concreteType;
    private String _principleType;
    private String _abstractType;
    private String _namingAuthority;

    public ServiceType(String type) {
	_type = type;

	if(_type.startsWith("service:")) {
	    _service = true;

	    int principleStart = 8;
	    int principleEnd = _type.indexOf(":", principleStart);

	    if(principleEnd != -1) {
		_abstract = true;
		_principleType =
		    _type.substring(principleStart, principleEnd);
		_abstractType =
		    _type.substring(0, principleEnd);
		_concreteType = _type.substring(principleEnd);
	    }
	    else {
		_abstract = false;
		_principleType = _type.substring(principleStart);
		_abstractType = "";
		_concreteType = "";
	    }

	    int namingStart = _type.indexOf(".") + 1;
	    if(namingStart != 0) {
		int namingEnd = _type.indexOf(":", namingStart);
		if(namingEnd == -1) {
		    _namingAuthority = _type.substring(namingStart);
		}
		else {
		    _namingAuthority = _type.substring(namingStart, namingEnd);
		}
	    }
	    else {
		_namingAuthority = "";
	    }
	}
	else {
	    _service = false;
	    _principleType = "";
	    _abstractType = "";
	    _concreteType = "";
	    _namingAuthority = "";
	}
    }

    public boolean isServiceURL() {
	return _service;
    }

    public boolean isAbstractType() {
	return _abstract;
    }

    public boolean isNADefault() {
	return _namingAuthority.equals("");
    }

    public String getConcreteTypeName() {
	return _concreteType;
    }

    public String getPrincipleTypeName() {
	return _principleType;
    }

    public String getAbstractTypeName() {
	return _abstractType;
    }

    public String getNamingAuthority() {
	return _namingAuthority;
    }

    public boolean equals(Object obj) {
	if(obj instanceof ServiceType) {
	    ServiceType t = (ServiceType)obj;
	    return (_service == t._service &&
		    _abstract == t._abstract &&
		    _concreteType.equals(t._concreteType) &&
		    _principleType.equals(t._principleType) &&
		    _abstractType.equals(t._abstractType) &&
		    _namingAuthority.equals(t._namingAuthority));
	}
	else {
	    return false;
	}
    }

    public String toString() {
	return _type;
    }

    public int hashCode() {
	int code = 0;

	if(_concreteType != null) {
	    code ^= (_concreteType.hashCode());
	}
	if(_principleType != null) {
	    code ^= (_principleType.hashCode() << 8);
	}
	if(_abstractType != null) {
	    code ^= (_abstractType.hashCode() << 16);
	}
	if(_namingAuthority != null) {
	    code ^= (_namingAuthority.hashCode() << 24);
	}

	return code;
    }
}
