#!/bin/bash

echo "Matt, Fix up the makefiles (the '$$d' thing) and use make dist!"
exit 0

if [ $1 ] ;
then 
   #Set up temporary directory
   CURRENTDIR=$PWD
   mkdir /tmp/mktarball
   cd /tmp/mktarball
   
   #Get everything from CVS
   cvs -z3 -dmpeterson@cvs.openslp.sourceforge.net:/cvsroot/openslp checkout openslp
   cvs -z3 -dmpeterson@cvs.openslp.sourceforge.net:/cvsroot/openslp checkout openslp.contrib
   rm -rf $(find -name CVS)
   openslp/autogen.sh
   
   #Tar up openslp "make dist"
   cd openslp
   ./autogen.sh
   ./configure
   make dist
   tar -zxf openslp-$1.tar.gz
   cp -a win32 openslp-$1
   cp -a doc openslp-$1
   cp -a etc openslp-$1
   cp -a README.W32 openslp-$1
   tar -zcf openslp-$1.tar.gz openslp-$1
   mv openslp-$1.tar.gz $CURRENTDIR
   cd ..
   
   #Tar up slptool
   mv openslp.contrib openslp-contrib-$1
   tar -cf openslp-contrib-$1.tar openslp-contrib-$1
   gzip -9 openslp-contrib-$1.tar
   mv openslp-contrib-$1.tar.gz $CURRENTDIR
   
   # Clean up temp direcory
   rm -rf /tmp/mktarball
else
   # Display usage
   echo Usage mktarball [version]
   echo       version is something like 0.8.1
fi

