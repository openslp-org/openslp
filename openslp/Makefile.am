#
# Makefile 
# Note the install script, it is not pretty, but it works.
#

SUBDIRS	= common libslpattr slpd libslp test
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
	rm -rf $(DESTDIR)$(DOC_DIR)
	mkdir -p $(DESTDIR)$(DOC_DIR)/IntroductionToSLP
	mkdir -p $(DESTDIR)$(DOC_DIR)/UsersGuide
	mkdir -p $(DESTDIR)$(DOC_DIR)/ProgrammersGuide
	cp $(srcdir)/doc/html/IntroductionToSLP/*.html $(DESTDIR)$(DOC_DIR)/IntroductionToSLP
	cp $(srcdir)/doc/html/UsersGuide/*.html $(srcdir)/doc/html/UsersGuide/*.jpg $(DESTDIR)$(DOC_DIR)/UsersGuide
	cp $(srcdir)/doc/html/ProgrammersGuide/*.html $(srcdir)/doc/html/ProgrammersGuide/*.jpg  $(DESTDIR)$(DOC_DIR)/ProgrammersGuide

dist-hook:
	rm -rf `find $(distdir)/doc -name CVS`
	rm -rf `find $(distdir)/etc -name CVS`
	rm -r  `find $(distdir)/win32 -name CVS`
