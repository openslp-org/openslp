#############################################################################
#
# Top level make file for the openslp project
#
# Currently, this make file will build a OpenSLP for Linux.  We hope to 
# use automake and autoconf in the future.
#############################################################################

release : slpd libslp doc
	cd slpd ; make $@
	cd libslp ; make $@
	cd doc ; make $@

debug: slpd libslp doc
	cd slpd ; make $@
	cd libslp ; make $@
	cd doc ; make $@
	
install: slpd libslp doc
	cd slpd ; make $@
	cd libslp ; make $@
	cd doc ; make $@

clean: slpd libslp doc
	cd slpd ; make $@
	cd libslp ; make $@
	cd doc ; make $@



