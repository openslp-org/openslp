EXTRA_DIST = slpd.all_init slpd.caldera_init slpd.calderaUNIX_init slpd.redhat_init slpd.suse_init

dist_sysconf_DATA = slp.conf slp.reg slp.spi

#make sure the slpd log directory is there
install-data-hook:
	mkdir -p $(localstatedir)/log

##install-data-local:
##	mkdir -p $(DESTDIR)$(sysconfdir)
##	file=$(DESTDIR)$(sysconfdir)/slp.reg; \
##	if [ -f $$file ]; then true; \
##	else cp -f $(srcdir)/etc/slp.reg $(DESTDIR)$(sysconfdir); \
##	fi
##	file=$(DESTDIR)$(sysconfdir)/slp.conf; \
##	if [ -f $$file ]; then true; \
##	else cp -f $(srcdir)/etc/slp.conf \
##	$(DESTDIR)$(sysconfdir); \
##	fi
##	file=$(DESTDIR)$(sysconfdir)/slp.spi; \
##	if [ -f $$file ]; then true; \
##	else cp -f $(srcdir)/etc/slp.spi $(DESTDIR)$(sysconfdir); \
##	fi

##uninstall-local:
##	chmod +w $(DESTDIR)$(sysconfdir)/slp.*
##	rm $(DESTDIR)$(sysconfdir)/slp.reg
##	rm $(DESTDIR)$(sysconfdir)/slp.conf
##	rm $(DESTDIR)$(sysconfdir)/slp.spi

