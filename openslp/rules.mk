#! gmake

##################################################################
##################################################################
## rules.mk --  Makefile definitions for building OpenSLP.
## This is a makefile include.  It should be included before any
## other rules are defined.
##
## The general structure of an OpenSLP Makefile should be:
##
##        #! gmake
##        DEPTH=<your depth in source tree>
##        include $(DEPTH)/defs.mk
##        <local declarations>
##        include $(DEPTH)/rules.mk
##        <local rules>
##
##################################################################
##################################################################


ifdef LIBRARY_NAME
LIBRARY	= $(LIBDIR)/$(LIBRARY_NAME).lib
INSTALL = $(LIBDIR)/$(LIBRARY_NAME).lib
TARGETS 	= $(OBJS) $(LIBRARY)
endif

ifdef DLL_NAME
SHARED_LIBRARY	= $(DYNLIBDIR)/$(DLL_NAME).dll
INSTALL 	= $(DYNLIBDIR)/$(DLL_NAME).lib \
                  $(DYNLIBDIR)/$(DLL_NAME).dll
TARGETS 	= $(OBJS) $(SHARED_LIBRARY)
DEF = $(DLL_NAME).def
endif

ifdef PROGRAM
INSTALL = $(BINDIR)/$(PROGRAM).exe
TARGETS = $(OBJS) $(PROGRAM)
endif


#
# OBJS is the list of object files.  It can be constructed by
# specifying CSRCS (list of C source files) and CPPSRCS (list
# of C++ source files).
#
OBJS	= $(addprefix $(OBJDIR)/,$(CSRCS:.c=.obj))		\
		  $(addprefix $(OBJDIR)/,$(CPPSRCS:.cpp=.obj))

#
# Win32 resource file
#
RCOBJS		= $(addprefix $(OBJDIR)/,$(RCSRCS:.rc=.res))
OBJS		+= $(RCOBJS)

ifdef DIRS
LOOP_OVER_DIRS		=						\
	@for d in $(DIRS); do						\
		if test -d $$d; then					\
			set -e;						\
			echo "$(MAKE) -f Makefile.win32 $$d $@";	\
			$(MAKE) -f Makefile.win32 -C $$d $@;		\
			set +e;						\
		else							\
			echo "Skipping non-directory $$d...";		\
		fi;							\
	done
endif

################################################################################

all::  install
	@$(MAKE_DESTDIRS)
	+$(LOOP_OVER_DIRS)

build:: $(TARGETS)
#	+$(LOOP_OVER_DIRS)


install:: $(TARGETS)
	@$(MAKE_DESTDIRS)
	@echo Installing $(INSTALL) to $(INSTALLDIR)
	@if [ -n "$(INSTALL)" ]; then cp $(INSTALL) $(INSTALLDIR);fi
	@echo Installing include files to $(INSTALLDIR)
	@if [ "x" != "x$(EXPORT_INCLUDES)" ];then cp -f $(EXPORT_INCLUDES) $(INCLUDE_INSTALLDIR);fi
	@if [ -d ../interface ];then cp -f ../interface/*.h $(INCLUDE_INSTALLDIR);fi
	+$(LOOP_OVER_DIRS)


clean::
	@rm -rf $(TARGETS)
	+$(LOOP_OVER_DIRS)


$(LIBRARY): $(OBJS)
	@echo Building library $(LIBRARY)
	@$(AR) $(OBJS) $(AR_EXTRA_ARGS)

$(SHARED_LIBRARY): $(OBJS) $(DEF)
	@echo Building dynamic library $(SHARED_LIBRARY)
	@$(LINK) $(DLLFLAGS) -DEF:$(DEF) $(LIBS) $(subst /,\\,$(OBJS))

$(RCOBJS): $(RCSRCS)
	@$(MAKE_OBJDIR)
	@$(RC) /fo$(subst /,\\,$(RCOBJS)) $(INCS) $(TMDEFS) $(RCSRCS)
	@echo $(RCOBJS) finished

###############################################################################
## Rule for building .cpp sources in the current directory into .o's in $(OBJDIR)
###############################################################################

$(OBJDIR)/%.obj: %.cpp
	@$(CCC) -Fo$@ -c $(CFLAGS) $<

###############################################################################
## Rule for building .c sources in the current directory into .o's in $(OBJDIR)
###############################################################################

$(OBJDIR)/%.obj: %.c
	@$(CC) -Fo$@ -c $(CFLAGS) $<


$(PROGRAM): $(OBJS)
	@echo Linking $(PROGRAM)....
	$(CC) $(OBJS) -Fe$(BINDIR)/$@  -link $(LDFLAGS) $(LIBS)

################################################################################
# Special gmake rules.
################################################################################

#
# Re-define the list of default suffixes, so gmake won't have to churn through
# hundreds of built-in suffix rules for stuff we don't need.
#
.SUFFIXES:
.SUFFIXES: .a .obj .c .cpp .s .h .i .pl

#
# Fake targets.  Always run these rules, even if a file/directory with that
# name already exists.
#
.PHONY: all clean install 

##################################################################
##################################################################

