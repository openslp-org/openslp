%define	ver 0.8.0
%define	rel 1
%define	name openslp
%define libver 0.0.2

Name        	: openslp
Version     	: %ver
Release     	: %rel
Group       	: Server/Network
Provides        : openslp libslp.so slpd
Summary     	: OpenSLP implementation of Service Location Protocol V2 
Copyright   	: Caldera Systems (LGPL)
Packager    	: Matthew Peterson <mpeterson@calderasystems.com>
URL         	: http://www.openslp.org/
BuildRoot   	: /tmp/%{name}-%{ver}
Source0		: ftp://openslp.org/pub/openslp/%{name}-%{ver}/%{name}-%{ver}.tar.gz


%Description
Service Location Protocol is an IETF standards track protocol that
provides a framework to allow networking applications to discover the
existence, location, and configuration of networked services in
enterprise networks.

OpenSLP is an open source implementation of the SLPv2 protocol as defined 
by RFC 2608 and RFC 2614.  This package include the daemon, libraries, header 
files and documentation

%Prep
%setup

%Build
#./configure --with-RPM-prefix=$RPM_BUILD_ROOT
./autogen.sh
./configure --prefix=$RPM_BUILD_ROOT
./configure
make

%Install
%{mkDESTDIR}
make install 
mkdir -p $DESTDIR/etc/rc.d/init.d
install -m 755 etc/slpd.all_init $DESTDIR/etc/rc.d/init.d/slpd

%Clean
rm -rf $RPM_BUILD_ROOT

%Post
rm -f /usr/lib/libslp.so
ln -s /usr/lib/libslp.so.%{libver} /usr/lib/libslp.so
/sbin/ldconfig

if [ -d '/usr/lib/OpenLinux' ]; then 
cat <<EOD  > /etc/sysconfig/daemons/slpd
IDENT=slp
DESCRIPTIVE="SLP Service Agent"
ONBOOT="yes"
EOD
fi

if [ -x /sbin/chkconfig ]; then
  chkconfig --add slpd
else 
  for i in 2 3 4 5; do
    ln -sf /etc/rc.d/init.d/slpd /etc/rc.d/rc$i.d/S13slpd
  done
  for i in 0 1 6; do
    ln -sf /etc/rc.d/init.d/slpd /etc/rc.d/rc$i.d/K87slpd
  done
fi

%PreUn
rm -f /etc/sysconfig/daemons/slpd
if [ "$1" = "0" ]; then
  if [ -x /sbin/chkconfig ]; then
    /sbin/chkconfig --del slpd
  else
    for i in 2 3 4 5; do
      rm -f /etc/rc.d/rc$i.d/S13slpd
    done
    for i in 0 1 6; do
      rm -f /etc/rc.d/rc$i.d/K87slpd
    done
  fi
fi

%PostUn 
if [ "$1" = "0" ]; then
  rm -f /usr/lib/libslp.so
fi
/sbin/ldconfig


%Files
%defattr(-,root,root)
%doc AUTHORS COPYING INSTALL NEWS README doc/*
%config /etc/slp.conf
%config /etc/slp.reg
/etc/rc.d/init.d/slpd
/usr/lib/libslp*
/usr/include/slp.h
/usr/sbin/slpd


%ChangeLog
* Wed Jul 17 2000 mpeterson@calderasystems.com
        Added lisa stuff
	
* Thu Jul 7 2000 david.mccormack@ottawa.com
	Made it work with the new autoconf/automake scripts.
 
* Wed Apr 27 2000 mpeterson
	started
