%define	ver 0.7.2
%define	rel	1
%define	name openslp

Name        	: openslp
Version     	: %ver
Release     	: %rel
Group       	: Server/Network
Summary     	: OpenSLP implementation of Service Location Protocol V2 
Copyright   	: Caldera Systems (LGPL)
Packager    	: Matthew Peterson <mpeterson@calderasystems.com>
URL         	: http://www.openslp.org/
BuildRoot   	: /tmp/%{name}-%{ver}
Source0		    : ftp://openslp.org/pub/openslp/%{name}-%{ver}/%{name}-%{ver}.tar.gz

%Description
Service Location Protocol is an IETF standards track protocol that
provides a framework to allow networking applications to discover the
existence, location, and configuration of networked services in
enterprise networks.

OpenSLP is an open source implementation of the SLPv2 protocol as defined 
by RFC 2608 and RFC 2614.  This package include the daemon, libraries, header 
files and documentation

%Prep
%setup -n %{name}-%{ver}

%Build
./configure
make

%Install
make install DESTDIR=$RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc/sysconfig/daemons 
cat <<EOD  > $RPM_BUILD_ROOT/etc/sysconfig/daemons/slpd
IDENT=slp
DESCRIPTIVE="SLP Service Agent"
ONBOOT="yes"
EOD
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d
cp etc/slpd.redhat_init $RPM_BUILD_ROOT/etc/rc.d/init.d/slpd

%Clean
rm -rf $RPM_BUILD_ROOT

%Post
/sbin/ldconfig
chkconfig --add slpd

%PostUn 
/sbin/ldconfig
chkconfig --del slpd

%Files
%defattr(-,root,root)
%doc /usr/doc/openslp-%{ver}
%config /etc/slp.conf
%config /etc/slp.reg
%config /etc/sysconfig/daemons/slpd
/etc/rc.d/init.d/slpd
/usr/lib/libslp.so.1.0.0
/usr/lib/libslp.so.1
/usr/lib/libslp.so
/usr/include/slp.h
/usr/sbin/slpd

%ChangeLog
* Thu Jul 27 2000 david.mccormack@ottawa.com
    Now reflects the update library creation.

* Fri Jul 7 2000 david.mccormack@ottawa.com
	Made it work with the new autoconf/automake scripts.

* Wed Apr 27 2000 mpeterson
    started

