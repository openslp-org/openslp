#!/bin/sh

SLPD=`ls slpd|grep slpd`

if [ "$SLPD" ]; then
   
   echo Installing OpenSLP daemon $SLPD ...
   if [ $UID = 0 ]; then
      install ./$SLPD /usr/sbin
      exit 0
   fi
   echo Must be root to install $SLPD in /usr/sbin!
   exit 1
fi

echo The Open SLP Daemon has not yet been built.  
echo Type "make install" to make and install the daemon.
exit 1




