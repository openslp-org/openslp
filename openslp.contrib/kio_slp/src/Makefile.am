## Makefile.am of kdebase/kioslave/man

INCLUDES=$(all_includes)
LDFLAGS =$(all_libraries) 
####### Files

lib_LTLIBRARIES = kio_slp.la

kio_slp_la_SOURCES = kio_slp.cpp 
kio_slp_la_LIBADD = -lkio -lslp
kio_slp_la_LDFLAGS = -module -avoid-version -no-undefined
noinst_HEADERS = kio_slp.h

kdelnk_DATA = slp.protocol
kdelnkdir = $(kde_servicesdir)

METASOURCES = AUTO
