#!/bin/bash
if [ $1 ] ;
then 
   CURRENTDIR=$PWD
   mkdir /tmp/mktarball
   cd /tmp/mktarball
   cvs -z9 -dmpeterson@cvs.openslp.sourceforge.net:/cvsroot/openslp checkout openslp
   cvs -z9 -dmpeterson@cvs.openslp.sourceforge.net:/cvsroot/openslp checkout openslp.contrib
   openslp/autogen.sh
   rm -rf $(find -name CVS)
   mv openslp $1
   tar -cf $1.tar $1
   gzip -9 $1.tar
   mv $1.tar.gz $CURRENTDIR
   tar -zcf openslp-contrib.tar.gz openslp.contrib
   mv openslp-contrib.tar.gz $CURRENTDIR   
   rm -rf /tmp/mktarball
else
   echo Usage mktarball openslp-[version]
fi

