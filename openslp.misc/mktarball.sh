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
   
   #Tar up openslp
   mv openslp openslp-$1
   tar -cf openslp-$1.tar openslp-$1
   gzip -9 openslp-$1.tar
   mv openslp-$1.tar.gz $CURRENTDIR
   
   #Tar up slptool
   mv openslp.contrib/slptool slptool-$1
   tar -cf slptool-$1.tar slptool-$1
   gzip -9 slptool-$1.tar
   mv slptool-$1.tar.gz $CURRENTDIR
   
   # Clean up temp direcory
   rm -rf /tmp/mktarball
else
   # Display usage
   echo Usage mktarball [version]
   echo       version is something like 0.8.1
fi

