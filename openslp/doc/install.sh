#!/bin/sh

OPENSLPDIR=openslp-0.6.0 

if [ $UID = 0 ]; then
   mkdir /usr/doc/$OPENSLPDIR
   rm -rf /usr/doc/$OPENSLPDIR/*
   cp -R ./* /usr/doc/$OPENSLPDIR 
   exit 0;
fi

echo Must be root to install OpenSLP documentation in /usr/doc!
exit 1
