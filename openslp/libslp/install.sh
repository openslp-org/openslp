#!/bin/sh

LIB=`ls libslp.so* | grep ls libslp.so*`

if [ "$LIB" ]; then 
   echo Installing OpenSLP Library LIB ...
   if [ $UID = 0 ]; then
      install ./$SLPD /usr/sbin
      exit 0
   fi
   echo Must be root to install LIB in /usr/lib!
   exit 1
fi

echo The Open SLP Library has not yet been built.  
echo Type "make install" to make and install the daemon.
exit 1




