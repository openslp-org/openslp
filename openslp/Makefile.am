#
# Makefile 
# Note the install script, it is not pretty, but it works.
#

SUBDIRS	= common slpd libslp test
DOC_DIR = /usr/doc/openslp-$(VERSION)

install-data-local:
	mkdir -p $(prefix)$(sysconfdir)
	cp -f $(srcdir)/etc/slp.reg $(prefix)$(sysconfdir)
	cp -f $(srcdir)/etc/slp.conf $(prefix)$(sysconfdir)
	rm -rf $(prefix)$(DOC_DIR)
	mkdir -p $(prefix)$(DOC_DIR)/IntroductionToSLP
	mkdir -p $(prefix)$(DOC_DIR)/UsersGuide
	mkdir -p $(prefix)$(DOC_DIR)/ProgrammersGuide
	cp $(srcdir)/doc/html/IntroductionToSLP/*.html $(prefix)$(DOC_DIR)/IntroductionToSLP
	cp $(srcdir)/doc/html/UsersGuide/*.html $(srcdir)/doc/html/UsersGuide/*.jpg $(prefix)$(DOC_DIR)/UsersGuide
	cp $(srcdir)/doc/html/ProgrammersGuide/*.html $(srcdir)/doc/html/ProgrammersGuide/*.jpg  $(prefix)$(DOC_DIR)/ProgrammersGuide
