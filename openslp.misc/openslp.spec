Name            : openslp
Version         : 0.8.2
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
BuildRoot       : /tmp/%{Name}-%{Version}

Requires	: SysVinit-scripts >= 1.07 

Source0: openslp/openslp-%{Version}.tar.gz
Source1: openslp/slptool-%{Version}.tar.gz

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
%setup -b 1

%Build
./configure --disable-predicates
make
cd ../slptool-%{Version}
make LIBS=-L../openslp-%{Version}/libslp/.libs INCS=-I../openslp-%{Version}/libslp


%Install
%{mkDESTDIR}
make install DOC_DIR=$DESTDIR/%{_defaultdocdir}/openslp-%{Version}
mkdir -p $DESTDIR/etc/rc.d/init.d
install -m 755 etc/slpd.caldera_init $DESTDIR%{SVIdir}/slpd
mkdir -p $DESTDIR/usr/bin
install -m 755 ../slptool-%{Version}/slptool $DESTDIR/usr/bin/slptool

if [ -d '/usr/lib/OpenLinux' ]; then 
mkdir -p $DESTDIR/etc/sysconfig/daemons
cat <<EOD  > $DESTDIR/etc/sysconfig/daemons/slpd
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
