#! gmake


##################################################################
##################################################################
## defs.mk --  Makefile definitions for building Connection software.
## This is a makefile include.  It should be included after DEPTH
## is set and before any other declarations.
##
## The general structure of a Makefile should be:
##
##        #! gmake
##        DEPTH=<your depth in source tree from abi/src>
##        include $(DEPTH)/config/defs.mk
##        <local declarations>
##        include $(DEPTH)/config/rules.mk
##        <local rules>
##
##############################################################################
##############################################################################

VERSION=\"0.7.4\"


##############################################################################
# where do executables and system includes reside...
##############################################################################

VCPATH="//t/MsDev60"
CC 	= $(VCPATH)/vc98/bin/cl.exe
CCC 	= $(VCPATH)/vc98/bin/cl.exe
LINK 	= $(VCPATH)/vc98/bin/link.exe
RC 	=  $(VCPATH)/msdev98/bin/rc.exe
AR 	= $(VCPATH)/vc98/bin/lib -NOLOGO -OUT:"$@"
OS_INC  = $(VCPATH)/vc98/include

##############################################################################
# this is the definition of directories, you may want to can change it
##############################################################################
OUT= d:
OUTDIR			= $(OUT)/openslp
OBJDIR			= $(OUTDIR)/obj
LIBDIR			= $(OUTDIR)/lib
DYNLIBDIR		= $(OUTDIR)/dynlib
BINDIR			= $(OUTDIR)/bin
INSTALLDIR		= d:/mdes/WIN32_4_VC6_OBJ/install
INCLUDE_INSTALLDIR	= $(OUTDIR)/include


##############################################################################
# a couple of compilation flags that shouldn't change
##############################################################################

INCLUDES 	= -I $(INCLUDE_INSTALLDIR) -I $(OS_INC) -I. \
		  -I$(DEPTH)/common -I$(DEPTH)/libslp
DEFINES		= -DVERSION=$(VERSION) -UDEBUG -U_DEBUG -DNDEBUG
LDFLAGS 	= 
DLLFLAGS 	= -nologo -SUBSYSTEM:CONSOLE -PDB:NONE -OUT:"$@" -DLL
CFLAGS 		= -W3 -nologo -GX -MD -DWIN32 -Zp1 -O2 \
                  $(DEFINES) $(INCLUDES)
COPY = cp
OS_LIBS		= kernel32 user32 gdi32 winspool comdlg32 advapi32 \
                  shell32 uuid comctl32 wsock32 winmm

LIBS		= -LIBPATH:$(INSTALLDIR) $(addsuffix .lib, $(APPLIBS))	\
			          $(addsuffix .lib,$(OS_LIBS))

WIN32MAKEFILE = Makefile.win32





##############################################################################
##############################################################################

define MAKE_DESTDIRS
if test ! -d $(OBJDIR); then rm -rf $(OBJDIR); mkdir -p $(OBJDIR); fi
if test ! -d $(LIBDIR); then rm -rf $(LIBDIR); mkdir -p $(LIBDIR); fi
if test ! -d $(DYNLIBDIR); then rm -rf $(DYNLIBDIR); mkdir -p $(DYNLIBDIR); fi
if test ! -d $(BINDIR); then rm -rf $(BINDIR); mkdir -p $(BINDIR); fi
if test ! -d $(INSTALLDIR); then rm -rf $(INSTALLDIR); mkdir -p $(INSTALLDIR); fi
if test ! -d $(INCLUDE_INSTALLDIR); then rm -rf $(INCLUDE_INSTALLDIR); mkdir -p $(INCLUDE_INSTALLDIR); fi
endef


