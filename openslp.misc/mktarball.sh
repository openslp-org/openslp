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
   
   #Tar up openslp
   mv openslp openslp-$1
   tar -cf openslp-$1.tar openslp-$1
   gzip -9 openslp-$1.tar
   mv openslp-$1.tar.gz $CURRENTDIR
   
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

