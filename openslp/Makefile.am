#
# Makefile 
# Note the install script, it is not pretty, but it works.
#

SUBDIRS	= common libslpattr slpd libslp slptool test
EXTRA_DIST = win32 doc etc README.W32
DOC_DIR = $(prefix)/doc/openslp-$(VERSION)

install-data-local:
	mkdir -p $(DESTDIR)$(sysconfdir)
	file=$(DESTDIR)$(sysconfdir)/slp.reg;\
        if [ -f $$file ]; then cp -f $$file $$file.bak; else true; fi
	cp -f $(srcdir)/etc/slp.reg $(DESTDIR)$(sysconfdir)
	file=$(DESTDIR)$(sysconfdir)/slp.conf;\
	if [ -f $$file ]; then cp -f $$file $$file.bak; else true; fi
	cp -f $(srcdir)/etc/slp.conf $(DESTDIR)$(sysconfdir)
	file=$(DESTDIR)$(sysconfdir)/slp.spi;\
	if [ -f $$file ]; then cp -f $$file $$file.bak; else true; fi
	cp -f $(srcdir)/etc/slp.spi $(DESTDIR)$(sysconfdir)
	rm -rf $(DESTDIR)$(DOC_DIR)
	mkdir -p $(DESTDIR)$(DOC_DIR)
	cp -r $(srcdir)/doc/* $(DESTDIR)$(DOC_DIR)
	rm -rf `find $(DESTDIR)$(DOC_DIR) -name CVS`

dist-hook:
	rm -rf `find $(distdir)/doc -name CVS`
	rm -rf `find $(distdir)/etc -name CVS`
	rm -rf `find $(distdir)/win32 -name CVS`
	chmod -R og-w $(distdir)
