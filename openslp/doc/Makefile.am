EXTRA_DIST = doc openslp.dxy README openslp_logo_web_color_150px.jpg main.html

distdir=$(PACKAGE)-doxy-$(VERSION)

docs: $(distdir).tar.gz

install-docs: docs
	$(mkdir_p) $(DESTDIR)$(datadir)/doc
	$(INSTALL_DATA) $(distdir).tar.gz $(DESTDIR)$(datadir)/doc
	$(INSTALL_DATA) $(srcdir)/doc/* $(DESTDIR)$(datadir)/doc

$(distdir).tar.gz: html/index.html html/openslp_logo_web_color_150px.jpg
	tar chof - html | gzip -9 -c >$@

html/index.html: ../config.h doxyfile main.html
	doxygen doxyfile

html/openslp_logo_web_color_150px.jpg: $(srcdir)/openslp_logo_web_color_150px.jpg
	cp $+ $@

CLEANFILES = doxywarn.txt html/*

clean-local:
	rm -rf html

