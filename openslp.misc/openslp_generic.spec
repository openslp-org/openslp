%define	ver 1.0.9
%define	rel 1
%define	name openslp
%define libver 1.0.0

Name        	: openslp
Version     	: %ver
Release     	: %rel
Group       	: Server/Network
Provides        : openslp libslp.so libslp.so.0 slpd
Obsoletes	: openslp-server
Summary     	: OpenSLP implementation of Service Location Protocol V2 
Copyright   	: Caldera Systems (BSD)
Packager    	: Matthew Peterson <mpeterson@calderasystems.com>
URL         	: http://www.openslp.org/
BuildRoot	: %{_tmppath}/%{name}-root
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
libtoolize --force
aclocal
automake
autoconf
./configure
make

%Install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/etc
cp etc/slp.conf $RPM_BUILD_ROOT/etc
cp etc/slp.reg $RPM_BUILD_ROOT/etc
mkdir -p $RPM_BUILD_ROOT/usr/lib
libtool install libslp/libslp.la $RPM_BUILD_ROOT/usr/lib
ln -s libslp.so.%{libver} $RPM_BUILD_ROOT/usr/lib/libslp.so.0
mkdir -p  $RPM_BUILD_ROOT/usr/sbin
libtool install slpd/slpd $RPM_BUILD_ROOT/usr/sbin 
mkdir -p $RPM_BUILD_ROOT/usr/bin
libtool install slptool/slptool $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/include
cp libslp/slp.h $RPM_BUILD_ROOT/usr/include
mkdir -p $RPM_BUILD_ROOT/usr/doc/openslp-%{ver}
cp -a doc/* $RPM_BUILD_ROOT/usr/doc/openslp-%{ver}

mkdir -p $RPM_BUILD_ROOT/etc/sysconfig/daemons 
cat <<EOD  > $RPM_BUILD_ROOT/etc/sysconfig/daemons/slpd
IDENT=slp
DESCRIPTIVE="SLP Service Agent"
ONBOOT="yes"
EOD
mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d
install -m 755 etc/slpd.all_init $RPM_BUILD_ROOT/etc/rc.d/init.d/slpd

%Clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%Pre
rm -f /usr/lib/libslp*

%Post
rm -f /usr/lib/libslp.so
ln -s libslp.so.%{libver} /usr/lib/libslp.so
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

# Except SUSE who needs to do this a little differently
if [ -d /etc/init.d ] && [ -d /etc/init.d/rc3.d ]; then
  ln -s /etc/rc.d/init.d/slpd /etc/init.d/slpd
  for i in 0 1 6
  do
    ln -sf ../slpd /etc/init.d/rc$i.d/K87slpd
  done
  for i in 2 3 4 5
  do
    ln -sf ../slpd /etc/init.d/rc$i.d/S13slpd
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
  if [ -d /etc/init.d ] && [ -d /etc/init.d/rc3.d ]; then
    rm /etc/init.d/slpd
    for i in 0 1 6
    do
      rm -f /etc/init.d/rc$i.d/K87slpd
    done
    for i in 2 3 4 5
    do
      rm -f /etc/init.d/rc$i.d/S13slpd
    done
  fi
  /etc/rc.d/init.d/slpd stop
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
/usr/bin/slptool


%ChangeLog
* Wed Feb 06 2002 alain.richard@equation.fr
	Adapted to enable build under redhat 7.x (uses BuildRoot macro,
	install instead of installtool for non libraries objects,
	protected rm -r for install & clean)

* Wed Jun 13 2001 matt@caldera.com
        Removed server stuff.  We want on binary rpm again
	
* Wed Jul 17 2000 mpeterson@calderasystems.com
        Added lisa stuff
	
* Thu Jul 7 2000 david.mccormack@ottawa.com
	Made it work with the new autoconf/automake scripts.
 
* Wed Apr 27 2000 mpeterson
	started
