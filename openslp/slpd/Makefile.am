#----------------------------------------------------------------------------
#  Copyright (C) 2000 Caldera Systems, Inc
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are
#  met:
#
#       Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#       Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#       Neither the name of Caldera Systems nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CALDERA
#  SYSTEMS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
#  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#----------------------------------------------------------------------------
#
# Makefile.am
#   Automake File
#     The linked in library libcommonslpd, which is from ../common
#     containing exacts functions which are required.
#
#----------------------------------------------------------------------------

EXTRA_DIST = slpd_win32.h slpd_win32.c

sbin_PROGRAMS = slpd

INCLUDES = \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/libslpattr \
	-I$(srcdir) \
	-I$(top_srcdir)/libslp \
	-DETCDIR=\"$(sysconfdir)\" \
	-DVARDIR=\"$(localstatedir)\"

if ENABLE_PREDICATES
slp_predicate_SRCS = slpd_predicate.c
slp_predicate_HDRS = slpd_predicate.h
endif

if ENABLE_SLPv1
slpd_v1process_SRCS = slpd_v1process.c
slpd_v1process_HDRS = slpd_v1process.h
endif

if ENABLE_SLPv2_SECURITY
slpd_security_SRCS = slpd_spi.c
slpd_security_HDRS = slpd_spi.h
endif

slpd_SOURCES = \
	$(slp_predicate_SRCS) \
	$(slpd_v1process_SRCS) \
	$(slpd_security_SRCS) \
	slpd_cmdline.c \
	slpd_database.c \
	slpd_incoming.c \
	slpd_knownda.c \
	slpd_log.c \
	slpd_main.c \
	slpd_outgoing.c \
	slpd_process.c \
	slpd_property.c \
	slpd_regfile.c \
	slpd_socket.c

noinst_HEADERS = \
	$(slp_predicate_HDRS) \
	$(slpd_v1process_HDRS) \
	$(slpd_security_HDRS) \
	slpd.h \
	slpd_knownda.h \
	slpd_process.h \
	slpd_unistd.h \
	slpd_cmdline.h \
	slpd_log.h \
	slpd_property.h \
	slpd_database.h \
	slpd_outgoing.h \
	slpd_regfile.h \
	slpd_incoming.h \
	slpd_socket.h
    
#if you're building on Irix, exchange commented and uncommented lines
#slpd_LDADD = ../common/libcommonslpd.a ../libslpattr/libslpattr.a
slpd_LDADD = ../common/libcommonslpd.la ../libslpattr/libslpattr.la
