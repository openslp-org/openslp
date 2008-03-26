##---------------------------------------------------------------------------
##  Copyright (C) 2000 Caldera Systems, Inc
##  All rights reserved.
##
##  Redistribution and use in source and binary forms, with or without
##  modification, are permitted provided that the following conditions are
##  met:
##
##       Redistributions of source code must retain the above copyright
##       notice, this list of conditions and the following disclaimer.
##
##       Redistributions in binary form must reproduce the above copyright
##       notice, this list of conditions and the following disclaimer in
##       the documentation and/or other materials provided with the
##       distribution.
##
##       Neither the name of Caldera Systems nor the names of its
##       contributors may be used to endorse or promote products derived
##       from this software without specific prior written permission.
##
##  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
##  `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
##  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
##  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CALDERA
##  SYSTEMS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
##  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
##  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,
##  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
##  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
##  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
##  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##---------------------------------------------------------------------------

EXTRA_DIST = slp_debug.h \
	SLPOpen/test.script SLPFindSrvTypes/test.script \
	SLPFindSrvs/test.script SLPReg/test.script \
	SLPDereg/test.script SLPFindAttrs/test.script \
	SLPParseSrvURL/test.script SLPEscape/test.script \
	SLPUnescape/test.script

TESTS = \
	SLPOpen/test.script SLPFindSrvTypes/test.script \
	SLPFindSrvs/test.script SLPReg/test.script \
	SLPDereg/test.script SLPFindAttrs/test.script \
	SLPParseSrvURL/test.script SLPEscape/test.script \
	SLPUnescape/test.script

XFAIL_TESTS = SLPFindAttrs/test.script

INCLUDES = \
	-I$(top_srcdir)/libslp \
	-I$(top_srcdir)/libslpattr \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/slpd

noinst_PROGRAMS = \
	testslpdereg \
	testslpescape \
	testslpfindattrs \
	testslpfindsrvtypes \
	testslpfindsrvs \
	testslpopen \
	testslpparsesrvurl \
	testslpreg \
	testslpunescape \
	testslp_attr_test \
	testslpd_predicate_test

LDADD = \
	../libslp/libslp.la \
	../libslpattr/libslpattr.la \
	../common/libcommonlibslp.la \
	../common/libcommonslpd.la

if ENABLE_PREDICATES
testslpd_predicate_test_SOURCES = SLPD_predicate_test/slpd_predicate_test.c
testslpd_predicate_test_LDADD = \
	$(LDADD) \
	../slpd/slpd_predicate.o \
	../common/libcommonslpd.la
endif

# Program names are in lower case because they conflict with directory names
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

##clean-local:
##	-rm -f *.output
##
### We have to manually copy files in the VPATH directories. 
##dist-hook:
##	@for d in `echo $(VPATH) | sed 's/:/ /g'`; do \
##         if test $$d != $(srcdir); then \
##            cp -pr $$d $(distdir)/`basename $$d`; \
##         fi; \
##      done

