TESTS = SLPOpen/test.script SLPFindSrvs/test.script         \
	SLPReg/test.script SLPDereg/test.script             \
	SLPFindAttrs/test.script SLPParseSrvURL/test.script \
	SLPEscape/test.script SLPUnescape/test.script 

INCLUDES = -I$(top_srcdir)/libslp

noinst_PROGRAMS = slpdereg slpescape slpfindattrs slpfindsrvs \
	          slpopen slpparsesrvurl slpreg slpunescape

LDADD = ../libslp/libslp.la

# Automake 1.4 doesn't allow subdirectory makefiles :-(, VPATH is a kludge 
# to get around that. It's either this or a makefile for each test.

VPATH = $(srcdir):$(srcdir)/SLPDereg:$(srcdir)/SLPEscape:\
        $(srcdir)/SLPFindAttrs:$(srcdir)/SLPFindSrvs:\
        $(srcdir)/SLPOpen:$(srcdir)/SLPParseSrvURL:\
        $(srcdir)/SLPReg:$(srcdir)/SLPUnescape

# Program names are in lower case because they conflict with directory names
slpdereg_SOURCES = SLPDereg.c
slpescape_SOURCES = SLPEscape.c
slpfindattrs_SOURCES = SLPFindAttrs.c
slpfindsrvs_SOURCES = SLPFindSrvs.c
slpopen_SOURCES = SLPOpen.c
slpparsesrvurl_SOURCES = SLPParseSrvURL.c
slpreg_SOURCES = SLPReg.c
slpunescape_SOURCES = SLPUnescape.c

# Remove all lines from VPATH (inclusive) to this line and uncomment
# the following lines when Automake 1.5 is released
#
# slpdereg_SOURCES = SLPDereg/SLPDereg.c
# slpescape_SOURCES = SLPEscape/SLPEscape.c
# slpfindattrs_SOURCES = SLPFindAttrs/SLPFindAttrs.c
# slpfindsrvs_SOURCES = SLPFindSrvs/SLPFindSrvs.c
# slpopen_SOURCES = SLPOpen/SLPOpen.c
# slpparsesrvurl_SOURCES = SLPParseSrvURL/SLPParseSrvURL.c
# slpreg_SOURCES = SLPReg/SLPReg.c
# slpunescape_SOURCES = SLPUnescape/SLPUnescape.c

clean-local:
	-rm -f *.output