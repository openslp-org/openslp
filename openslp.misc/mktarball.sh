#!/bin/bash
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
   ./configure --enable-security
   sed -e"s~$$/$$file~$d/$$file~g" Makefile > Makefile.fixed
   mv Makefile.fixed Makefile
   make dist
   cp openslp*.tar.gz $CURRENTDIR/openslp-$1.tar.gz
   cd ..
   
   #Tar up the contrib stuff
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

