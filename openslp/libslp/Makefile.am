#
# Makefile.am
#   Automake File
#     The linked in library libcommonlibslp, which is from ../common
#     containing exacts functions which are required.
#

lib_LTLIBRARIES		= libslp.so
INCLUDES			= -I../common 
libslp_so_LIBADD	=  $(top_srcdir)/common/libcommonlibslp.a
AR					= gcc
LIBTOOL_DEPS 		= @LIBTOOL_DEPS@
libslp_so_LDFLAGS	+= -shared

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

libslp_so_SOURCES	= libslp_dereg.c   \
	libslp_findscopes.c \
	libslp_reg.c \
	libslp_findsrvs.c \
	libslp_parse.c \
	libslp_property.c  \
	libslp_handle.c  \
	libslp_thread.c \
	libslp_network.c \
	libslp_findattrs.c \
	libslp_delattrs.c \
	libslp_findsrvtypes.c \
	libslp_knownda.c

install-data-local:
	mkdir -p $(libdir)
	mkdir -p $(includedir)
	cp -f $(lib_LTLIBRARIES) $(libdir)/libslp.so.$(VERSION)
	cp -f slp.h	$(includedir)/
	ln -f -s $(libdir)/libslp.so.$(VERSION) $(libdir)/libslp.so

../common/libcommon.a:
