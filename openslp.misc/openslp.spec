Name            : openslp
Version         : 0.8.1
Release         : 1
Group           : Server/Network
Summary         : OpenSLP implementation of Service Location Protocol V2
Copyright       : Caldera Systems, Inc (BSD)
Packager        : Matthew Peterson <mpeterson@caldera.com>
URL             : http://www.openslp.org
 
BuildRoot       : /tmp/%{Name}-%{Version}
  
Source0: openslp/openslp-%{Version}.tar.gz
Source1: openslp/slptool-%{Version}.tar.gz


%Description
Service Location Protocol is an IETF standards track protocol that
provides a framework to allow networking applications to discover the
existence, location, and configuration of networked services in
enterprise networks.
    

%Prep
%setup -b 1


%Build
./configure --disable-predicates
make
cd ../slptool-%{Version}
make LIBS=../openslp-%{Version}/libslp/.libs INCS=../openslp-%{Version}/libslp


%Install
%{mkDESTDIR}
make install DOC_DIR=$DESTDIR/%{_defaultdocdir}/openslp-%{Version}
mkdir -p $DESTDIR/etc/rc.d/init.d
install -m 755 etc/slpd.caldera_init $DESTDIR%{SVIdir}/slpd
mkdir -p $DESTDIR/usr/bin
install -m 755 ../slptool-%{Version}/slptool $DESTDIR/usr/bin/slptool

if [ -d '/usr/lib/OpenLinux' ]; then 
cat <<EOD  > /etc/sysconfig/daemons/slpd
IDENT=slp
DESCRIPTIVE="SLP Service Agent"
ONBOOT="yes"
EOD
fi


%{fixManPages}
%{fixInfoPages}
%{fixUP} -T $DESTDIR/%{SVIdir} -e 's:\@SVIdir\@:%{SVIdir}:' 


%Clean
%{rmDESTDIR}


%Post
/sbin/ldconfig
/usr/lib/LSB/init-install slpd


%PreUn
/usr/lib/LSB/init-remove slpd


%PostUn 
/sbin/ldconfig


%Files
%defattr(-,root,root)
%doc AUTHORS COPYING INSTALL NEWS README doc/*
%config /etc/slp.conf
%config /etc/slp.reg
%config /etc/sysconfig/daemons/slpd
/etc/rc.d/init.d/slpd
/usr/lib/libslp*
/usr/include/slp.h
/usr/sbin/slpd
/usr/bin/slptool


%ChangeLog
* Mon Dec 18 2000 mpeterson@caldera.com
        Added LSB init stuff
	
* Wed Nov 28 2000 mpeterson@caldera.com
        Removed lisa stuff and RPM_BUILD_ROOT
	
* Wed Jul 17 2000 mpeterson@caldera.com
        Added lisa stuff
	
* Thu Jul 7 2000 david.mccormack@ottawa.com
	Made it work with the new autoconf/automake scripts.
 
* Wed Apr 27 2000 mpeterson
	Started
