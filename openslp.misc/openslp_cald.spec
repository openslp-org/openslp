Name        	: openslp
Version     	: 0.6.7
Release     	: 1
Group       	: Server/Network
Summary     	: OpenSLP implementation of Service Location Protocol V2 
Copyright   	: Caldera Systems (LGPL)
Packager    	: Matthew Peterson <mpeterson@calderasystems.com>
URL         	: http://www.openslp.org/

BuildRoot   	: /tmp/%{Name}-%{Version}

Source0		: ftp://openslp.org/pub/openslp/openslp-0.6.7/openslp-0.6.7.tar.gz


%Description
Installs OpenSLP daemon, libraries, header files and documentation


%Prep
%setup -n openslp-%{Version}


%Build
make


%Install
%{mkDESTDIR}
mkdir -p $DESTDIR/usr/{lib,include,doc,sbin}
mkdir -p $DESTDIR/etc/rc.d/init.d
make
#We'll need these until I get the automake done
mkdir -p $DESTDIR/usr/doc/openslp-%{Version}  
cp AUTHORS $DESTDIR/usr/doc/openslp-%{Version}
cp README $DESTDIR/usr/doc/openslp-%{Version}
cp INSTALL $DESTDIR/usr/doc/openslp-%{Version}
cp COPYING $DESTDIR/usr/doc/openslp-%{Version}
cp -Rf doc/html $DESTDIR/usr/doc/openslp-%{Version}
cp -Rf doc/rfc $DESTDIR/usr/doc/openslp-%{Version} 
cp etc/slp.reg $DESTDIR/etc
cp etc/slp.conf $DESTDIR/etc
cp etc/slpd.caldera_init $DESTDIR/etc/rc.d/init.d/slpd
cp libslp/libslp.so* $DESTDIR/usr/lib
cp libslp/slp.h $DESTDIR/usr/include
cp slpd/slpd $DESTDIR/usr/sbin


%Clean
%{rmDESTDIR}


%Post -p /sbin/ldconfig


%PostUn -p /sbin/ldconfig


%Files
%defattr(-,root,root)
/usr/include/*
/usr/lib/*
/usr/sbin/*
/usr/doc/*
/etc/*


%ChangeLog
* Wed Apr 27 2000 mpeterson
	started
