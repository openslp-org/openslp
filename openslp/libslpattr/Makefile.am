
EXTRA_DIST = libslpattr.dsp libslpattr.dsw libslpattr.mak

noinst_LTLIBRARIES	= libslpattr.la  

if ENABLE_PREDICATES
PREDICATE_SRCS = libslpattr.c 
else
PREDICATE_SRCS = libslpattr_tiny.c
endif

EXTRA_libslpattr_la_SOURCES = libslpattr.c libslpattr_tiny.c
libslpattr_la_SOURCES = $(PREDICATE_SRCS) libslpattr_internal.h

noinst_HEADERS = libslpattr.h 
