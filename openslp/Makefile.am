#
# Makefile 
# Note the install script, it is not pretty, but it works.
#
SUBDIRS	= common slpd libslp
DOC_DIR = $(prefix)/doc/openslp-$(VERSION)

RANLIB	= @RANLIB@
INCLUDEDIR	=	common/ slpd/	

install-data-local:
	cp -f etc/slp.reg	/etc
	cp -f etc/slp.conf	/etc
	rm -rf $(DOC_DIR)
	mkdir -p $(DOC_DIR)/IntroductionToSLP
	mkdir -p $(DOC_DIR)/UsersGuide
	mkdir -p $(DOC_DIR)/ProgrammersGuide
	cp doc/html/IntroductionToSLP/*.html $(DOC_DIR)/IntroductionToSLP
	cp doc/html/UsersGuide/*.html doc/html/UsersGuide/*.jpg $(DOC_DIR)/UsersGuide
	cp doc/html/ProgrammersGuide/*.html doc/html/ProgrammersGuide/*.jpg  $(DOC_DIR)/ProgrammersGuide
