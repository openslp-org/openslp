##---------------------------------------------------------------------------
##  Copyright (C) 2000 Caldera Systems, Inc
##  All rights reserved.
##
##  Redistribution and use in source and binary forms, with or without
##  modification, are permitted provided that the following conditions are
##  met:
##
##       Redistributions of source code must retain the above copyright
##       notice, this list of conditions and the following disclaimer.
##
##       Redistributions in binary form must reproduce the above copyright
##       notice, this list of conditions and the following disclaimer in
##       the documentation and/or other materials provided with the
##       distribution.
##
##       Neither the name of Caldera Systems nor the names of its
##       contributors may be used to endorse or promote products derived
##       from this software without specific prior written permission.
##
##  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
##  `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
##  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
##  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CALDERA
##  SYSTEMS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
##  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
##  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,
##  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
##  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
##  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
##  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##---------------------------------------------------------------------------
##
## Makefile 
## Note the install script, it is not pretty, but it works.
##
##---------------------------------------------------------------------------

ACLOCAL_AMFLAGS = -I m4

# Add "test" back to SUBDIRS, remove from EXTRA_DIST after we get it working
EXTRA_DIST = win32 README.W32 FAQ test
SUBDIRS = common libslpattr slpd libslp slptool etc doc obs

# Additional root-level targets - mostly installation and docs
docs install-docs:
	cd doc && $(MAKE) $(AM_MAKEFLAGS) $@

##install-cfg install-clientcfg install-servercfg install-initscript:
##	cd etc && $(MAKE) $(AM_MAKEFLAGS) $@

##install-client:
##	cd client && $(MAKE) $(AM_MAKEFLAGS) install

##install-server:
##	cd common && $(MAKE) $(AM_MAKEFLAGS) install
##	cd server && $(MAKE) $(AM_MAKEFLAGS) install
##	cd stats && $(MAKE) $(AM_MAKEFLAGS) install

dist-hook:
	rm -rf `find $(distdir) -name .svn`

rpmmacros = --define='_rpmdir $(PWD)' --define='_srcrpmdir $(PWD)'\
 --define='_sourcedir $(PWD)' --define='_specdir $(PWD)'\
 --define='_builddir $(PWD)'

rpmcheck:
	-rm -rf _rpm
	@which rpm &> /dev/null; \
        if [ $$? -ne 0 ]; then \
          echo "*** This make target requires an rpm-based linux distribution."; \
          exit 1; \
        fi

srcrpm: rpmcheck dist
	rpmbuild -bs $(rpmmacros) --nodeps obs/$(PACKAGE).spec

rpms: rpmcheck dist
	rpmbuild -ba $(rpmmacros) --nodeps --buildroot=$(PWD)/_rpm obs/$(PACKAGE).spec
	rpmarch=`rpm --showrc | grep ^build\ arch | sed 's/\(.*: \)\(.*\)/\2/'`; \
        test -z $${rpmarch} || ( mv $${rpmarch}/* .; rm -rf $${rpmarch} )
