
EXTRA_DIST = libslpattr.dsp libslpattr.dsw libslpattr.mak

#if you're building on Irix, exchange commented and uncommented lines
#noinst_LTLIBRARIES	= libslpattr.a  
noinst_LTLIBRARIES	= libslpattr.la  

if ENABLE_PREDICATES
PREDICATE_SRCS = libslpattr.c 
else
PREDICATE_SRCS = libslpattr_tiny.c
endif

#if you're building on Irix, exchange commented and uncommented lines
#EXTRA_libslpattr_a_SOURCES = libslpattr.c libslpattr_tiny.c
#libslpattr_a_SOURCES = $(PREDICATE_SRCS) libslpattr_internal.h
EXTRA_libslpattr_la_SOURCES = libslpattr.c libslpattr_tiny.c
libslpattr_la_SOURCES = $(PREDICATE_SRCS) libslpattr_internal.h

noinst_HEADERS = libslpattr.h 
