#!/bin/sh

OPENSLPDIR=openslp-0.6.0 

if [ $UID = 0 ]; then
   rm -rf /usr/doc/$OPENSLPDIR
   mkdir /usr/doc/$OPENSLPDIR   
   cp -Rf html /usr/doc/$OPENSLPDIR 
   cp -Rf rfc /usr/doc/$OPENSLPDIR 
   exit 0;
fi

echo Must be root to install OpenSLP documentation in /usr/doc!
exit 1
