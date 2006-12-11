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

#if you're building on Irix, replace .la with .a below
noinst_LTLIBRARIES = libcommonslpd.la libcommonlibslp.la

AM_YFLAGS = -d

EXTRA_DIST = slp_win32.c

if ENABLE_SLPv1
slp_v1message_SRCS = slp_v1message.c slp_utf8.c
endif

if ENABLE_SLPv2_SECURITY
slp_security_SRCS = slp_auth.c slp_crypto.c slp_spi.c
endif

#if you're building on Irix, replace .la with .a below
libcommonlibslp_la_SOURCES = \
   slp_atomic.c \
   slp_buffer.c \
   slp_compare.c \
   slp_database.c \
   slp_debug.c \
   slp_dhcp.c \
   slp_iface.c \
   slp_linkedlist.c \
   slp_message.c \
   slp_net.c \
   slp_network.c \
   slp_parse.c \
   slp_pid.c \
   slp_property.c \
   slp_thread.c \
   $(slp_security_SRCS) \
   $(slp_v1message_SRCS) \
   slp_v2message.c \
   slp_xcast.c \
   slp_xid.c \
   slp_xmalloc.c

# Place yacc files before lex files below - lexx files include generated yacc headers

#if you're building on Irix, replace .la with .a below
libcommonslpd_la_SOURCES = \
   slp_atomic.c \
   slp_buffer.c \
   slp_compare.c \
   slp_database.c \
   slp_debug.c \
   slp_dhcp.c \
   slp_iface.c \
   slp_linkedlist.c \
   slp_message.c \
   slp_net.c \
   slp_parse.c \
   slp_pid.c \
   slp_predicate.c \
   slp_property.c \
   slp_thread.c \
   $(slp_security_SRCS) \
   $(slp_v1message_SRCS) \
   slp_v2message.c \
   slp_xid.c \
   slp_xmalloc.c \
   slp_filter_y.y \
   slp_filter_l.l \
   slp_attr_y.y \
   slp_attr_l.l

noinst_HEADERS = \
   slp_atomic.h \
   slp_attr.h \
   slp_auth.h \
   slp_buffer.h \
   slp_compare.h \
   slp_crypto.h \
   slp_database.h \
   slp_debug.h \
   slp_dhcp.h \
   slp_filter.h \
   slp_iface.h \
   slp_linkedlist.h \
   slp_message.h \
   slp_net.h \
   slp_network.h \
   slp_parse.h \
   slp_pid.h \
   slp_predicate.h \
   slp_property.h \
   slp_socket.h \
   slp_spi.h \
   slp_thread.h \
   slp_types.h \
   slp_utf8.h \
   slp_v1message.h \
   slp_v2message.h \
   slp_xcast.h \
   slp_xid.h \
   slp_xmalloc.h

