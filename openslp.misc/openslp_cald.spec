Name            : openslp
Version         : 0.7.8
Release         : 1
Group           : Server/Network
Summary         : OpenSLP implementation of Service Location Protocol V2
Copyright       : Caldera Systems, Inc (BSD)
Packager        : Matthew Peterson <mpeterson@caldera.com>
URL             : http://www.openslp.org
 
BuildRoot       : /tmp/%{Name}-%{Version}
  
Source0: ftp://openslp.org//pub/openslp/%{Name}-%{Version}.tar.gz
   
%Description
Service Location Protocol is an IETF standards track protocol that
provides a framework to allow networking applications to discover the
existence, location, and configuration of networked services in
enterprise networks.
    
%Prep
%setup

%Build
./autogen.sh
./configure
make

%Install
%{mkDESTDIR}
make install 
mkdir -p $DESTDIR/etc/rc.d/init.d
install -m 755 etc/slpd.all_init $DESTDIR/etc/rc.d/init.d/slpd

%{fixManPages}

%Clean
%{rmDESTDIR}

%Post
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
* Wed Nov 28 2000 mpeterson@caldera.com
        Removed lisa stuff and RPM_BUILD_ROOT
	
* Wed Jul 17 2000 mpeterson@caldera.com
        Added lisa stuff
	
* Thu Jul 7 2000 david.mccormack@ottawa.com
	Made it work with the new autoconf/automake scripts.
 
* Wed Apr 27 2000 mpeterson
	Started
