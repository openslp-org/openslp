Name            : openslp
Version         : 1.0.3
Release         : 1
Group           : Server/Network
Summary     	: Open source implementation of Service Location Protocol V2.
Summary(de) 	: Open source Implementierung des Service Location Protocols V2.
Summary(es) 	: Implementación open source del Service Location Protocol V2.
Summary(fr) 	: Implémentation Open Source du Service Location Protocol V2.
Summary(it) 	: Implementazione open source del Service Location Protocol V2.
Summary(pt) 	: Implementação 'open source' do protocolo Service Location Protocol V2.
Copyright       : Caldera Systems, Inc (BSD)
Packager        : Matthew Peterson <mpeterson@caldera.com>
URL             : http://www.openslp.org
BuildRoot       : /var/tmp/%{Name}-%{Version}

Provides	: libslp.so
Requires	: SysVinit-scripts >= 1.07, libtool

Source0		: %{Name}-%{Version}.tar.gz
Source1		: slpd.init

%Description
Service Location Protocol is an IETF standards track protocol that
provides a framework to allow networking applications to discover the
existence, location, and configuration of networked services in
enterprise networks.

%Description -l de
Das Service Location Protocol ist ein IETF standard Protokoll welches ein Gerüst
bereitstellt um es Netzwerk-fähigen Anwendungen zu ermöglichen die Existenz,
den Ort und die Konfiguration von Netzwerkdiensten in Unternehmensnetzwerken zu
entdecken.

%Description -l es
El Protocolo de Localización de Servicios es un protocolo de seguimiento acorde
al estándar IETF que proporciona un entorno para permitir a las aplicaciones de
red descubrir la existencia, localización y configuración de servicios de red 
en redes empresariales.

%Description -l fr
Service Location Protocol est un protocole de suivi des normes IETF
qui fournit un cadre permettant à des applications réseau de 
découvrir l'existence, l'emplacement et la configuration de 
services de réseau dans les réseaux d'entreprise.

%Description -l it
Il Service Location Protocol (protocollo di localizzazione di servizi)
è un protocollo standard IETF che fornisce un'infrastruttura per
permettere alle applicazioni di rete di scoprire l'esistenza, la localizzazione
e la configurazione dei servizi nelle reti delle aziende.

%Description -l pt
O Service Location Protocol é um protocolo normalizado pelo IETF que
oferece uma plataforma para permitir às aplicações de rede que descubram
a existência, localização e a configuração dos serviços de rede nas redes
duma empresa.

%Prep
%setup 

%Build
./configure --disable-predicates
make

%Install
%{mkDESTDIR}

mkdir -p $DESTDIR/etc
mkdir -p $DESTDIR%{SVIdir}
mkdir -p $DESTDIR/usr/{sbin,lib,bin,include}
mkdir -p $DESTDIR/etc/sysconfig/daemons 
mkdir -p $DESTDIR%{_defaultdocdir}/%{Name}-%{Version}

cp etc/slp.conf $DESTDIR/etc
cp etc/slp.reg $DESTDIR/etc
cp libslp/slp.h $DESTDIR/usr/include
cp -a doc/* $DESTDIR%{_defaultdocdir}/%{Name}-%{Version}

libtool install slpd/slpd $DESTDIR/usr/sbin 
libtool install slptool/slptool $DESTDIR/usr/bin
libtool install libslp/libslp.la $DESTDIR/usr/lib
ln -s libslp.so.%{libver} $DESTDIR/usr/lib/libslp.so.0

cat <<EOD  > $DESTDIR/etc/sysconfig/daemons/slpd
IDENT=slp
DESCRIPTIVE="SLP Service Agent"
ONBOOT="yes"
OPTIONS=""
EOD

install -m 755 %{SOURCE1} $DESTDIR%{SVIdir}/slpd

%{fixManPages}
%{fixInfoPages}
%{fixUP} -T $DESTDIR/%{SVIdir} -e 's:\@SVIdir\@:%{SVIdir}:' 


%Clean
%{rmDESTDIR}


%Post
if [ -e /usr/lib/libslp.so ]; then
	rm -f /usr/lib/libslp.so
fi
ln -s /usr/lib/libslp.so.%{libver} /usr/lib/libslp.so
libtool --finish /usr/lib > /dev/null 2>&1

/usr/lib/LSB/init-install slpd


%PreUn
%{SVIdir}/slpd stop > /dev/null 2>&1
/usr/lib/LSB/init-remove slpd


%PostUn 
/sbin/ldconfig


%Files
%defattr(-,root,root)
%config /etc/slp.conf
%config /etc/slp.reg
%config /etc/sysconfig/daemons/slpd
%{SVIdir}/slpd
%{_defaultdocdir}/%{Name}-%{Version}/*
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
