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
#     The linked in library libcommonlibslp, which is from ../common
#     containing exacts functions which are required.
#
#----------------------------------------------------------------------------

EXTRA_DIST = 

lib_LTLIBRARIES = libslp.la
include_HEADERS = slp.h
INCLUDES	= -I$(top_srcdir)/common 

libslp_la_SOURCES = \
   libslp_delattrs.c \
   libslp_dereg.c \
   libslp_findattrs.c \
   libslp_findscopes.c \
   libslp_findsrvs.c \
   libslp_findsrvtypes.c \
   libslp_handle.c  \
   libslp_knownda.c \
   libslp_network.c \
   libslp_parse.c \
   libslp_property.c  \
   libslp_reg.c \
   libslp_thread.c

noinst_HEADERS = \
   libslp.h

#if you're building on Irix, exchange commented and uncommented lines
#libslp_la_LIBADD = -L../common/libcommonlibslp
libslp_la_LIBADD = ../common/libcommonlibslp.la

libslp_la_LDFLAGS = -version-info 1:0:0
