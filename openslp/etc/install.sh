#!/bin/sh

OPENSLPDIR=openslp-0.6.0 

if [ $UID = 0 ]; then
   cp slp.reg /etc
   cp slp.conf /etc
   exit 0;
fi

echo Must be root to install OpenSLP configuration files in /etc
exit 1
