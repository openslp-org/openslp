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

EXTRA_DIST = win32 doc doxygen etc README.W32 FAQ
SUBDIRS = obs common libslpattr slpd libslp slptool test

DOC_DIR = $(prefix)/doc/openslp-$(VERSION)

install-data-local:
	mkdir -p $(DESTDIR)$(sysconfdir)
	file=$(DESTDIR)$(sysconfdir)/slp.reg; \
	if [ -f $$file ]; then true; \
	else cp -f $(srcdir)/etc/slp.reg $(DESTDIR)$(sysconfdir); \
	fi
	file=$(DESTDIR)$(sysconfdir)/slp.conf; \
	if [ -f $$file ]; then true; \
	else cp -f $(srcdir)/etc/slp.conf \
	$(DESTDIR)$(sysconfdir); \
	fi
	file=$(DESTDIR)$(sysconfdir)/slp.spi; \
	if [ -f $$file ]; then true; \
	else cp -f $(srcdir)/etc/slp.spi $(DESTDIR)$(sysconfdir); \
	fi
	rm -rf $(DESTDIR)$(DOC_DIR)
	mkdir -p $(DESTDIR)$(DOC_DIR)
	cp -r $(srcdir)/doc/* $(DESTDIR)$(DOC_DIR)

dist-hook:
	chmod -R og-w $(distdir)
