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

import java.io.ByteArrayOutputStream;
import java.io.DataOutput;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.Serializable;
import java.util.Iterator;
import java.util.StringTokenizer;
import java.util.Vector;

/**
 * Implementation of the SLP ServiceLocationAttribute class defined in RFC 2614.
 * 
 * @author Patrick Callis
 */
public class ServiceLocationAttribute implements Serializable {

	private static final long serialVersionUID = 1L;
	private String _id = null;
	private Vector _values = null;

	public ServiceLocationAttribute(String id, Vector values) {
		_id = id;
		_values = values;
	}

	ServiceLocationAttribute(String attr) {
		_values = new Vector();
		if (attr.charAt(0) == '(') {
			int pos = 0, endPos = 0;
			_id = attr.substring(1, (pos = attr.indexOf("=")));
			pos++;
			while ((endPos = attr.indexOf(",", pos)) != -1) {
				_values.add(attr.substring(pos, endPos));
				pos = endPos + 1;
			}
			_values.add(attr.substring(pos, attr.length() - 1));
		} else {
			_id = attr;
		}
	}

	/**
	 * Returns a Vector of ServiceLocationAttributes from an attribute list
	 * string.
	 */
	static Vector readFromList(String list) {
		int pos = 0;
		Vector attrs = new Vector();

		while (list.length() > 0) {
			if (list.charAt(0) == '(') {
				pos = list.indexOf("),") + 1;
			} else {
				pos = list.indexOf(",");
			}

			if (pos <= 0) {
				pos = list.length();
			}

			attrs.add(new ServiceLocationAttribute(list.substring(0, pos)));
			if (pos < list.length()) {
				list = list.substring(pos + 1);
			} else {
				list = "";
			}
		}
		return attrs;
	}

	/**
	 * Parses the string looking for the RFC 2608 escape sequence (\), when
	 * found it will convert the escaped value to the proper object, e.g.
	 * opaque (Byte[]), Integer, Boolean or String 
	 * @param s
	 * @return
	 */
	public static Object unEscape(String s) {
		// There is a discrepancy between RFC2608 & RFC2614 in that
		// RFC2608 states that opaque values start with "\FF" while
		// RFC 2614 states that they should start with "\ff". We will check for
		// both here.
		if (s.startsWith("\\FF") || s.startsWith("\\FF")) {
			StringTokenizer st = new StringTokenizer(s.substring(4), "\\");
			Byte[] b = new Byte[st.countTokens()];
			for (int i = 0; i < b.length; i++) {
				try {
					b[i] = Byte.parseByte(st.nextToken(), 16);
				} catch (NumberFormatException e) {
					b[i] = -1;
				}
			}

			return b;
		} else if (s.equalsIgnoreCase("true")) {
			return Boolean.TRUE;
		} else if (s.equalsIgnoreCase("false")) {
			return Boolean.FALSE;
		} else if (!s.endsWith(" ")) {
			try {
				Integer i = new Integer(s);
				return i;
			} catch (NumberFormatException e) {
			}
		} else {
			// Remove the trailing space
			s = s.substring(0, s.length() - 1);
		}
		// If we get here, it must be just a string
		return s;
	}

	static String escapeString(String s) {

		// If string can be interpreted as an integer or
		// boolean, append a space to the end per RFC 2614
		// Section 5.7.3
		if (s.equalsIgnoreCase("true") || s.equalsIgnoreCase("false")) {
			s += " ";
		} else {
			try {
				Integer.parseInt(s);
				s += " ";
			} catch (NumberFormatException e) {
			}
		}

		StringBuffer buf = new StringBuffer();
		for (int i = 0; i < s.length(); i++) {
			char c = s.charAt(i);
			switch (c) {
			case '{':
			case '}':
			case ',':
			case '\\':
			case '!':
			case '<':
			case '=':
			case '>':
			case '~':
			case ';':
			case '*':
			case '+':
				buf.append("\\");
				byte hexdigit = (byte) ((c & 0xF0) >>> 4);
				buf.append(Integer.toHexString((int) hexdigit));
				hexdigit = (byte) (c & 0x0F);
				buf.append(Integer.toHexString((int) hexdigit));
				break;
			default:
				buf.append(c);
			}

		}
		return buf.toString();
	}

	public static String escapeId(String id) {
		return escapeString(id);
	}

	/**
	 * A client application can use this function to 'escape' a value such that
	 * it can be transmitted over the wire per RFC2608. Essentially examines the
	 * object for reserved characters or values (Integers, Booleans,
	 * (, ), ',', \, !, <, =, >, ~ &  CTL) and 'escapes' them as needed<BR>
	 * A client should call this for attribute values before placing the value
	 * into an attribute. <I>It is important to understand that this function will
	 * escape all reserved characters including the comma, as such if the
	 * attribute value is a string list (which uses the comma as an explicit
	 * delimiter) then that value should not be sent to this function.</I> 
	 * 
	 * @param value
	 *            The value to be examined for reserved characters
	 * @return A string representation of the object, with reserved characters
	 *         properly escaped, that can be sent to a DA/SA
	 */
	public static String escapeValue(Object value) {
		if (value instanceof String) {
			return escapeString((String) value);
		} else if (value instanceof Integer) {
			return value.toString();
		} else if (value instanceof Boolean) {
			return value.toString();
		} else if (value instanceof byte[]) {
			byte[] arr = (byte[]) value;
			StringBuffer buf = new StringBuffer();
			// There is a discrepency between RFC2608 & RFC2614 in that
			// RFC2608 states that opaque values start with "\FF" while
			// RFC 2614 states that they should start with "\ff". We will use
			// capitals here as it is what will be sent over the wire
			buf.append("\\FF");
			for (int i = 0; i < arr.length; i++) {
				buf.append("\\");
				byte hexdigit = (byte) ((arr[i] & 0xF0) >>> 4);
				buf.append(Integer.toHexString((int) hexdigit));
				hexdigit = (byte) (arr[i] & 0x0F);
				buf.append(Integer.toHexString((int) hexdigit));
			}

			return buf.toString();
		} else {
			throw new IllegalArgumentException("Invalid Object type");
		}
	}

	static int calcAttrListSize(Vector list) {
		int size = 0;
		Iterator i = list.iterator();
		while (i.hasNext()) {
			size += ((ServiceLocationAttribute) i.next()).calcSize();
			size++; // comma
		}
		if (size > 0)
			size--; // no comma on first attr
		return size;
	}

	public Vector getValues() {
		return (Vector) _values.clone();
	}

	public String getId() {
		return _id;
	}

	public boolean equals(Object o) {
		ServiceLocationAttribute a;

		if (o instanceof ServiceLocationAttribute) {
			a = (ServiceLocationAttribute) o;
			if (_id.equals(a.getId())) {
				Vector v = a.getValues();
				if (v != null && v.size() == _values.size()) {
					for (int i = 0; i < v.size(); i++) {
						if (!(_values.get(i).equals(v.get(i)))) {
							return false;
						}
					}
					return true;
				}
			}
		}
		return false;
	}

	public String toString() {
		StringBuffer buf = new StringBuffer();

		buf.append(_id);
		buf.append("=");
		Iterator i = _values.iterator();
		boolean first = true;
		while (i.hasNext()) {
			if (first) {
				first = false;
			} else {
				buf.append(",");
			}
			buf.append("(");
			Object o = i.next();
			buf.append(o.getClass().getName());
			buf.append(")");
			buf.append(o);
		}

		return buf.toString();
	}

	void writeAttribute(DataOutput out) throws IOException {

		out.write("(".getBytes());
		out.write(_id.getBytes());
		out.write("=".getBytes());
		Iterator i = _values.iterator();
		boolean first = true;
		while (i.hasNext()) {
			if (first) {
				first = false;
			} else {
				out.write(",".getBytes());
			}
			Object o = i.next();
			out.write(o.toString().getBytes());
		}
		out.write(")".getBytes());
	}

	int calcSize() {
		int size = 3 // Parens and equals
		+ _id.getBytes().length; // length of id
		Iterator i = _values.iterator();
		while (i.hasNext()) {
			Object val = i.next();
			size += val.toString().getBytes().length; // length of attr
			size++; // comma
		}
		if (size > 0)
			size--; // No comma on first in list
		return size;
	}

	public int hashCode() {
		return _id.hashCode();
	}

	static boolean verifyList(String list, AuthenticationBlock[] blocks) {
		SLPConfiguration conf = ServiceLocationManager.getConfiguration();
		if (conf.getSecurityEnabled()) {
			for (int i = 0; i < blocks.length; i++) {
				try {
					byte[] data = getAuthData(list, blocks[i].getSPI(),
							blocks[i].getTimestamp());
					if (blocks[i].verify(data)) {
						return true;
					}
				} catch (Exception e) {
				}
			}
			return false;
		} else {
			return true;
		}

	}

	private static byte[] getAuthData(String list, String spi, int timestamp)
			throws IOException {
		ByteArrayOutputStream bos = new ByteArrayOutputStream();
		DataOutputStream dos = new DataOutputStream(bos);

		byte[] temp = spi.getBytes();
		dos.writeShort(temp.length);
		dos.write(temp);
		temp = list.getBytes();
		dos.writeShort(temp.length);
		dos.write(temp);
		dos.writeInt(timestamp);
		return bos.toByteArray();
	}
}
