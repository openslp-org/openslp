#!/bin/bash
if [ $1 ] ;
then 
   CURRENTDIR=$PWD
   mkdir /tmp/mktarball
   cd /tmp/mktarball
   cvs -z9 -dmpeterson@cvs.openslp.sourceforge.net:/cvsroot/openslp checkout openslp
   rm -Rf openslp/openslp/CVS
   rm -Rf openslp/common/CVS
   rm -Rf openslp/doc/CVS 
   rm -Rf openslp/doc/html/CVS
   rm -Rf openslp/doc/html/UsersGuide/CVS
   rm -Rf openslp/doc/html/ProgrammersGuide/CVS
   rm -Rf openslp/doc/html/IntroductionToSLP/CVS
   rm -Rf openslp/doc/rfc/CVS
   rm -Rf openslp/etc/CVS
   rm -Rf openslp/libslp/CVS
   rm -Rf openslp/slpd/CVS
   mv openslp $1
   tar -cf $1.tar $1
   gzip -9 $1.tar
   mv $1.tar.gz $CURRENTDIR
   rm -rf /tmp/mktarball
else
   echo Usage mktarball openslp-[version]
fi

