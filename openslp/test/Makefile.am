EXTRA_DIST = slp_debug.h 

TESTS = SLPOpen/test.script SLPFindSrvTypes/test.script  \
        SLPFindSrvs/test.script SLPReg/test.script       \
        SLPDereg/test.script SLPFindAttrs/test.script    \
        SLPParseSrvURL/test.script SLPEscape/test.script \
        SLPUnescape/test.script 

XFAIL_TESTS = SLPFindAttrs/test.script

INCLUDES = -I$(top_srcdir)/libslp -I$(top_srcdir)/libslpattr \
           -I$(top_srcdir)/common -I$(top_srcdir)/slpd

noinst_PROGRAMS = testslpdereg testslpescape testslpfindattrs testslpfindsrvtypes \
                  testslpfindsrvs testslpopen testslpparsesrvurl testslpreg testslpunescape \
		  testslp_attr_test testslpd_predicate_test

LDADD = ../libslp/libslp.la ../libslpattr/libslpattr.la ../common/libcommonlibslp.la ../common/libcommonslpd.la

if ENABLE_PREDICATES
testslpd_predicate_test_LDADD = $(LDADD) ../slpd/slpd_predicate.o ../common/libcommonslpd.la
endif

testslpdereg_SOURCES = SLPDereg/SLPDereg.c
testslpescape_SOURCES = SLPEscape/SLPEscape.c
testslpfindattrs_SOURCES = SLPFindAttrs/SLPFindAttrs.c
testslpfindsrvs_SOURCES = SLPFindSrvs/SLPFindSrvs.c
testslpfindsrvtypes_SOURCES = SLPFindSrvTypes/SLPFindSrvTypes.c
testslpopen_SOURCES = SLPOpen/SLPOpen.c
testslpparsesrvurl_SOURCES = SLPParseSrvURL/SLPParseSrvURL.c
testslpreg_SOURCES = SLPReg/SLPReg.c
testslpunescape_SOURCES = SLPUnescape/SLPUnescape.c
testslp_attr_test_SOURCES = SLP_attr_test/slp_attr_test.c
testslpd_predicate_test_SOURCES = SLPD_predicate_test/slpd_predicate_test.c

clean-local:
	-rm -f *.output

# We have to manually copy files in the VPATH directories. 
dist-hook:
	@for d in `echo $(VPATH) | sed 's/:/ /g'`; do             \
            if test $$d != $(srcdir); then                        \
                cp -pr $$d $(distdir)/`basename $$d`;             \
            fi;                                                   \
        done

