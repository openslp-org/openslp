# -----------------------------------------------
# --------- BEGIN VARIABLE DEFINITIONS ----------
# -----------------------------------------------

%define name openslp
%define ver 1.0.10
%define rel 4

%define libver 1.0.0
%define initscript slpd

# Needed to set up and remove init symlinks when all else fails...
%define startnum 13
%define killnum 87

%define lsbdocdir /usr/share/doc/packages
%define lsbmandir /usr/share/man/en
%define lsbinit /etc/init.d
%define rhdocdir /usr/share/doc
%define rhmandir /usr/share/man
%define rhinit /etc/init.d
%define destdir /opt/lsb-caldera.com-volution

%define _defaultdocdir %{lsbdocdir}

%define libver 1.0.0

# ---------------------------------------------
# -------- END OF VARIABLE DEFINITIONS --------
# ---------------------------------------------

# ---------------------------------------------
# ---------- BEGIN RPM HEADER INFO ------------
# ---------------------------------------------

Name        	: %{name}
Summary     	: OpenSLP implementation of Service Location Protocol V2 

Version     	: %{ver}
Release     	: %{rel}
Copyright   	: Caldera Systems (BSD)
Group       	: System Environment/Daemons
Packager    	: Erik Ratcliffe <eratcliffe@volutiontech.com>
URL         	: http://www.openslp.org/

BuildRoot	: %{_tmppath}/%{name}-root
Provides        : openslp libslp.so libslp.so.0 slpd
Obsoletes	: openslp-server

Source0		: ftp://openslp.org/pub/openslp/%{name}-%{ver}/%{name}-%{ver}.tar.gz
Source1		: slpd.init

%Description
Service Location Protocol is an IETF standards track protocol that
provides a framework to allow networking applications to discover the
existence, location, and configuration of networked services in
enterprise networks.

OpenSLP is an open source implementation of the SLPv2 protocol as defined 
by RFC 2608 and RFC 2614.  This package include the daemon, libraries, header 
files and documentation

# ---------------------------------------------
# ---------- END OF RPM HEADER INFO -----------
# ---------------------------------------------

# ---------------------------------------------
# ------------ BEGIN BUILD SECTION ------------
# ---------------------------------------------

%prep
%setup -n %{name}-%{ver}

%build
libtoolize --force
aclocal
automake
autoconf
./configure
make

# ---------------------------------------------
# ----------- END OF BUILD SECTION ------------
# ---------------------------------------------

# ---------------------------------------------
# ----------- BEGIN INSTALL SECTION -----------
# ---------------------------------------------

%install
[ "${RPM_BUILD_ROOT}" != "/" ] && rm -rf ${RPM_BUILD_ROOT}

mkdir -p ${RPM_BUILD_ROOT}/etc
cp etc/slp.conf ${RPM_BUILD_ROOT}/etc
cp etc/slp.reg ${RPM_BUILD_ROOT}/etc

mkdir -p ${RPM_BUILD_ROOT}/usr/lib
libtool install libslp/libslp.la ${RPM_BUILD_ROOT}/usr/lib
ln -s libslp.so.%{libver} ${RPM_BUILD_ROOT}/usr/lib/libslp.so.0

mkdir -p  ${RPM_BUILD_ROOT}/usr/sbin
libtool install slpd/slpd ${RPM_BUILD_ROOT}/usr/sbin 

mkdir -p ${RPM_BUILD_ROOT}/usr/bin
libtool install slptool/slptool ${RPM_BUILD_ROOT}/usr/bin

mkdir -p ${RPM_BUILD_ROOT}/usr/include
cp libslp/slp.h ${RPM_BUILD_ROOT}/usr/include

mkdir -p ${RPM_BUILD_ROOT}%{lsbdocdir}/%{name}-%{ver}
cp -a doc/* ${RPM_BUILD_ROOT}%{lsbdocdir}/%{name}-%{ver}


mkdir -p ${RPM_BUILD_ROOT}%{lsbinit}
install -m 755 %{SOURCE1} ${RPM_BUILD_ROOT}%{lsbinit}/%{initscript}

# ---------------------------------------------
# ---------- END OF INSTALL SECTION -----------
# ---------------------------------------------

# ---------------------------------------------
# ------ BEGIN PRE/POST-INSTALL SECTION -------
# ---------------------------------------------

%pre
rm -f /usr/lib/libslp*

# Who started this /etc/rc.d/init.d stuff anyway??  It's WRONG!
#
# Assume that if %{lsbinit} exists it's either the proper init
# script directory for the distro or it's a symlink already set up
# to fix this issue.
if [ ! -e %{lsbinit} ] && [ -d /etc/rc.d/init.d ]; then
     ln -s /etc/rc.d/init.d %{lsbinit}
fi 

# ---------------------------------------------

%post
if [ -e /usr/lib/libslp.so ]; then rm -f /usr/lib/libslp.so; fi

ln -s libslp.so.%{libver} /usr/lib/libslp.so
/sbin/ldconfig

# Set up the SysV runlevel links.  First try the LSB compatible
# method, then the Red Hat method, then the OpenLinux method, then
# a non-LSB SuSE method, if all else fails use the medieval method...
if [ -x /usr/lib/lsb/install_initd ]; then
       /usr/lib/lsb/install_initd %{initscript} > /dev/null 2>&1
elif [ -x /sbin/chkconfig ]; then
       /sbin/chkconfig --add %{initscript} > /dev/null 2>&1
elif [ -x /usr/lib/LSB/init-install ]; then
       /usr/lib/LSB/init-install %{initscript} > /dev/null 2>&1
elif [ -e /etc/SuSE-release ]; then
  for i in 0 1 6; do
    ln -sf ../%{initscript} %{lsbinit}/rc$i.d/K%{killnum}%{initscript}
  done
  for i in 2 3 4 5; do
    ln -sf ../%{initscript} %{lsbinit}/rc$i.d/S%{startnum}%{initscript}
  done
else
  for i in 0 1 2 6; do
    ln -sf ../init.d/%{initscript} \
       /etc/rc.d/rc$i.d/K%{killnum}%{initscript}
  done
  for i in 3 4 5; do
    ln -sf ../init.d/%{initscript} \
       /etc/rc.d/rc$i.d/S%{startnum}%{initscript}
  done
fi

# You only need to do this on OpenLinux systems.
# Be sure to UN-do this in the %postun section...
if grep OpenLinux /etc/.installed > /dev/null 2>&1; then
cat <<EOD  > /etc/sysconfig/daemons/slpd
IDENT=slp
DESCRIPTIVE="SLP Service Agent"
ONBOOT="yes"
EOD
fi

# The following code snippet is useful for making installed 
# docs in an LSB compatible location available in Red Hat. 
# If there's a better way to see if you're on Red Hat, do tell...
if grep "Red Hat" /etc/redhat-release > /dev/null 2>&1; then
ln -s %{_defaultdocdir}/%{name}-%{ver} \
    %{rhdocdir}/%{name}-%{ver}
fi

# ---------------------------------------------
# ------ END OF PRE/POST-INSTALL SECTION ------
# ---------------------------------------------

# ---------------------------------------------
# ----- BEGIN PRE/POST-UN-INSTALL SECTION -----
# ---------------------------------------------

%preun 
if [ "$1" = "0" ]; then
    # Remove any SysV runlevel links.
    if [ -x /usr/lib/lsb/remove_initd ]; then
            /usr/lib/lsb/remove_initd %{initscript} > /dev/null 2>&1
    elif [ -x /sbin/chkconfig ]; then
           /sbin/chkconfig --del %{initscript} > /dev/null 2>&1
    elif [ -x /usr/lib/LSB/init-remove ]; then
           /usr/lib/LSB/init-remove %{initscript} > /dev/null 2>&1
    elif [ -e /etc/SuSE-release ]; then
       for i in 0 1 6; do
           rm -f %{lsbinit}/rc$i.d/K%{killnum}%{initscript}
       done
       for i in 2 3 4 5; do
           rm -f %{lsbinit}/rc$i.d/S%{startnum}%{initscript}
       done
    else
       for i in 0 1 2 6; do
           rm -f /etc/rc.d/rc$i.d/K%{killnum}%{initscript}
       done
       for i in 3 4 5; do
           rm -f /etc/rc.d/rc$i.d/S%{startnum}%{initscript}
       done
    fi
    %{lsbinit}/%{initscript} stop > /dev/null 2>&1
fi

# ---------------------------------------------

%postun 
if [ "$1" = "0" ]; then
    if [ -e /usr/lib/libslp.so ]; then rm -f /usr/lib/libslp.so; fi
fi

# Remember that file we created in %post just for OpenLinux systems?...
if [ -e /etc/sysconfig/daemons/%{initscript} ]; then
    rm -f /etc/sysconfig/daemons/%{initscript}
fi

# ...and that symlink we set up for doc dirs on Red Hat?...
rm -f %{rhdocdir}/%{name}-%{ver}

/sbin/ldconfig

# -----------------------------------------------
# ----- END OF PRE/POST-UN-INSTALL SECTION ------
# -----------------------------------------------

%clean
[ "${RPM_BUILD_ROOT}" != "/" ] && rm -rf ${RPM_BUILD_ROOT}

# -----------------------------------------------
# ----- END OF BUILD SYSTEM CLEANING SECTION ----
# -----------------------------------------------

# -----------------------------------------------
# ------------ BEGIN FILES SECTION --------------
# -----------------------------------------------

%files
%defattr(-,root,root)
%doc doc/*
%config /etc/slp.conf
/usr/lib/libslp*
/usr/include/slp.h
%config /etc/slp.reg
%{lsbinit}/%{initscript}
/usr/sbin/slpd
/usr/bin/slptool

# -----------------------------------------------
# ------------ END OF FILES SECTION -------------
# -----------------------------------------------

# -----------------------------------------------
# ---------- BEGIN CHANGELOG SECTION ------------
# -----------------------------------------------

%changeLog
* Tue Jan 14 2003 eratcliffe@volutiontech.com
	Removed RPM_BUILD_ROOT from a %post routine.  I must have been tired
	that day...

* Fri Jan 03 2003 eratcliffe@volutiontech.com
    Altered spec file to be more distro agnostic
    Adjusted init script for the same reason.

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

# -----------------------------------------------
# ---------- END OF CHANGELOG SECTION -----------
# -----------------------------------------------

