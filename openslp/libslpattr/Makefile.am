noinst_LTLIBRARIES	= libslpattr.la 

if USE_PREDICATES
PREDICATE_SRCS = libslpattr.c
else
PREDICATE_SRCS = libslpattr_tiny.c
endif


libslpattr_la_SOURCES = $(PREDICATE_SRCS)

noinst_HEADERS = \
	libslpattr.h
