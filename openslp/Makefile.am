#
# Makefile 
# Note the install script, it is not pretty, but it works.
#

SUBDIRS	= common libslpattr slpd libslp test 
DOC_DIR = $(prefix)/doc/openslp-$(VERSION)

install-data-local:
	mkdir -p $(DESTDIR)$(sysconfdir)
	cp -fb $(srcdir)/etc/slp.reg $(DESTDIR)$(sysconfdir)
	cp -fb $(srcdir)/etc/slp.conf $(DESTDIR)$(sysconfdir)
	rm -rf $(DESTDIR)$(DOC_DIR)
	mkdir -p $(DESTDIR)$(DOC_DIR)/IntroductionToSLP
	mkdir -p $(DESTDIR)$(DOC_DIR)/UsersGuide
	mkdir -p $(DESTDIR)$(DOC_DIR)/ProgrammersGuide
	cp $(srcdir)/doc/html/IntroductionToSLP/*.html $(DESTDIR)$(DOC_DIR)/IntroductionToSLP
	cp $(srcdir)/doc/html/UsersGuide/*.html $(srcdir)/doc/html/UsersGuide/*.jpg $(DESTDIR)$(DOC_DIR)/UsersGuide
	cp $(srcdir)/doc/html/ProgrammersGuide/*.html $(srcdir)/doc/html/ProgrammersGuide/*.jpg  $(DESTDIR)$(DOC_DIR)/ProgrammersGuide
