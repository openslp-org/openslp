#!/bin/sh

LIB=`ls libslp.so* | grep libslp.so*`

if [ "$LIB" ]; then 
   echo Installing OpenSLP Library LIB ...
   if [ $UID = 0 ]; then
      install ./$LIB /usr/lib
      cd /usr/lib
      ldconfig
      exit 0
   fi
   echo Must be root to install LIB in /usr/lib!
   exit 1
fi

echo The Open SLP Library has not yet been built.  
echo Type "make install" to make and install the daemon.
exit 1




