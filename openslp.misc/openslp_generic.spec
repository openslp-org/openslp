%define	ver 0.9.1
%define	rel 1
%define	name openslp
%define libver 0.0.3

Name        	: openslp
Version     	: %ver
Release     	: %rel
Group       	: Server/Network
Provides        : openslp libslp.so slpd
Obsoletes	: openslp-server
Summary     	: OpenSLP implementation of Service Location Protocol V2 
Copyright   	: Caldera Systems (BSD)
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
%setup -n %{name}-%{ver}

%Build
%{mkDESTDIR}
./configure
make

%Install
%{mkDESTDIR}
mkdir -p $DESTDIR/etc
cp etc/slp.conf $DESTDIR/etc
cp etc/slp.reg $DESTDIR/etc
mkdir -p $DESTDIR/usr/lib
libtool install libslp/libslp.la $DESTDIR/usr/lib
mkdir -p  $DESTDIR/usr/sbin
libtool install slpd/slpd $DESTDIR/usr/sbin 
mkdir -p $DESTDIR/usr/bin
libtool install slptool/slptool $DESTDIR/usr/bin
mkdir -p $DESTDIR/usr/include
cp libslp/slp.h $DESTDIR/usr/include
mkdir -p $DESTDIR/usr/doc/openslp-%{ver}
cp -a doc/* $DESTDIR/usr/doc/openslp-%{ver}

mkdir -p $DESTDIR/etc/sysconfig/daemons 
cat <<EOD  > $DESTDIR/etc/sysconfig/daemons/slpd
IDENT=slp
DESCRIPTIVE="SLP Service Agent"
ONBOOT="yes"
EOD
mkdir -p $DESTDIR/etc/rc.d/init.d
install -m 755 etc/slpd.all_init $DESTDIR/etc/rc.d/init.d/slpd

%Clean
rm -rf $RPM_BUILD_ROOT/usr

%Post
rm -f /usr/lib/libslp.so
ln -s /usr/lib/libslp.so.%{libver} /usr/lib/libslp.so
/sbin/ldconfig

if [ -x /bin/lisa ]; then 
  lisa --SysV-init install slpd S13 2:3:4:5 K87 0:1:6  
elif [ -x /sbin/chkconfig ]; then
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
if [ "$1" = "0" ]; then
  if [ -x /sbin/chkconfig ]; then
    /sbin/chkconfig --del slpd
  elif [ -x /bin/lisa ]; then
    lisa --SysV-init remove slpd $1 
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
%doc /usr/doc/openslp-%{ver}
%config /etc/slp.conf
/usr/lib/libslp*
/usr/include/slp.h
%config /etc/sysconfig/daemons/slpd
%config /etc/slp.reg
/etc/rc.d/init.d/slpd
/usr/sbin/slpd


%ChangeLog
* Wed Jun 13 2001 matt@caldera.com
        Removed server stuff.  We want on binary rpm again
	
* Wed Jul 17 2000 mpeterson@calderasystems.com
        Added lisa stuff
	
* Thu Jul 7 2000 david.mccormack@ottawa.com
	Made it work with the new autoconf/automake scripts.
 
* Wed Apr 27 2000 mpeterson
	started
