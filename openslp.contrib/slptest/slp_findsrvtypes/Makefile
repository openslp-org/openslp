# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.4 from Makefile.am

# Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

#
# Makefile.am
#   Automake File
#     The linked in library libslp is from ../libslp
#

include ../SETUP.mk
BINARY = slp_findsrvtypes
INCLUDES = -I$(SLPROOT)/libslp -I$(SLPROOT)/common

SOURCES = slp_findsrvtypes.c ../slp_testcommon.h
LDADD = $(SLPROOT)/libslp/libslp.la

DEFS =  -DPACKAGE=\"openslp\" -DVERSION=\"0.9.0\" -DENABLE_SLPv1=1 -DENABLE_PREDICATES=1 -DHAVE_LIBCRYPTO=1 -DHAVE_DSA_VERIFY=1 -DHAVE_DSA_SIZE=1 -DHAVE_DSA_FREE=1 -DHAVE_SHA1=1 -DENABLE_SECURITY=1 -DSTDC_HEADERS=1 -DHAVE_STRING_H=1 -DHAVE_SA_RESTORER=1 -DHAVE_LIBNSL=1 -DHAVE_LIBM=1 -DHAVE_CEIL=1 -DHAVE_LOG10=1  -I.  
LIBS = -lm -lnsl -lcrypto -lslp 
OBJECTS =  slp_findsrvtypes.o
CFLAGS = -DDEBUG -D_REENTRANT=1 -DLINUX -Wall -g3 -O0

BINARY:slp_findsrvtypes.o
	gcc -o $(BINARY) $(DEFS) $(OBJECTS) $(LIBS) 

slp_findsrvtypes.o: $(SOURCES)
	gcc -c $(DEFS) $(CFLAGS) $(INCLUDES) slp_findsrvtypes.c

clean: 
	-rm -rf slp_findsrvtypes.o
	-rm -rf slp_findsrvtypes
#*.h:	
