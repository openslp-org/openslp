# Microsoft Developer Studio Generated NMAKE File, Based on libslp.dsp
!IF "$(CFG)" == ""
CFG=libslp - Win32 Debug
!MESSAGE No configuration specified. Defaulting to libslp - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "libslp - Win32 Release" && "$(CFG)" != "libslp - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libslp.mak" CFG="libslp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libslp - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libslp - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libslp - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\libslp.dll"


CLEAN :
	-@erase "$(INTDIR)\libslp.res"
	-@erase "$(INTDIR)\libslp_delattrs.obj"
	-@erase "$(INTDIR)\libslp_dereg.obj"
	-@erase "$(INTDIR)\libslp_findattrs.obj"
	-@erase "$(INTDIR)\libslp_findscopes.obj"
	-@erase "$(INTDIR)\libslp_findsrvs.obj"
	-@erase "$(INTDIR)\libslp_findsrvtypes.obj"
	-@erase "$(INTDIR)\libslp_handle.obj"
	-@erase "$(INTDIR)\libslp_knownda.obj"
	-@erase "$(INTDIR)\libslp_network.obj"
	-@erase "$(INTDIR)\libslp_parse.obj"
	-@erase "$(INTDIR)\libslp_property.obj"
	-@erase "$(INTDIR)\libslp_reg.obj"
	-@erase "$(INTDIR)\libslp_thread.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\libslp.dll"
	-@erase "$(OUTDIR)\libslp.exp"
	-@erase "$(OUTDIR)\libslp.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I ".\\" /I "..\common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\libslp.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libslp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\common\Release\common.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib wsock32.lib winmm.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\libslp.pdb" /machine:I386 /def:".\libslp.def" /out:"$(OUTDIR)\libslp.dll" /implib:"$(OUTDIR)\libslp.lib" 
DEF_FILE= \
	".\libslp.def"
LINK32_OBJS= \
	"$(INTDIR)\libslp_delattrs.obj" \
	"$(INTDIR)\libslp_dereg.obj" \
	"$(INTDIR)\libslp_findattrs.obj" \
	"$(INTDIR)\libslp_findscopes.obj" \
	"$(INTDIR)\libslp_findsrvs.obj" \
	"$(INTDIR)\libslp_findsrvtypes.obj" \
	"$(INTDIR)\libslp_handle.obj" \
	"$(INTDIR)\libslp_knownda.obj" \
	"$(INTDIR)\libslp_network.obj" \
	"$(INTDIR)\libslp_parse.obj" \
	"$(INTDIR)\libslp_property.obj" \
	"$(INTDIR)\libslp_reg.obj" \
	"$(INTDIR)\libslp_thread.obj" \
	"$(INTDIR)\libslp.res"

"$(OUTDIR)\libslp.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libslp - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\libslp.dll"


CLEAN :
	-@erase "$(INTDIR)\libslp.res"
	-@erase "$(INTDIR)\libslp_delattrs.obj"
	-@erase "$(INTDIR)\libslp_dereg.obj"
	-@erase "$(INTDIR)\libslp_findattrs.obj"
	-@erase "$(INTDIR)\libslp_findscopes.obj"
	-@erase "$(INTDIR)\libslp_findsrvs.obj"
	-@erase "$(INTDIR)\libslp_findsrvtypes.obj"
	-@erase "$(INTDIR)\libslp_handle.obj"
	-@erase "$(INTDIR)\libslp_knownda.obj"
	-@erase "$(INTDIR)\libslp_network.obj"
	-@erase "$(INTDIR)\libslp_parse.obj"
	-@erase "$(INTDIR)\libslp_property.obj"
	-@erase "$(INTDIR)\libslp_reg.obj"
	-@erase "$(INTDIR)\libslp_thread.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\libslp.dll"
	-@erase "$(OUTDIR)\libslp.exp"
	-@erase "$(OUTDIR)\libslp.ilk"
	-@erase "$(OUTDIR)\libslp.lib"
	-@erase "$(OUTDIR)\libslp.map"
	-@erase "$(OUTDIR)\libslp.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /I "..\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\libslp.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libslp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\common\Debug\common.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib wsock32.lib winmm.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\libslp.pdb" /map:"$(INTDIR)\libslp.map" /debug /machine:I386 /def:".\libslp.def" /out:"$(OUTDIR)\libslp.dll" /implib:"$(OUTDIR)\libslp.lib" /pdbtype:sept 
DEF_FILE= \
	".\libslp.def"
LINK32_OBJS= \
	"$(INTDIR)\libslp_delattrs.obj" \
	"$(INTDIR)\libslp_dereg.obj" \
	"$(INTDIR)\libslp_findattrs.obj" \
	"$(INTDIR)\libslp_findscopes.obj" \
	"$(INTDIR)\libslp_findsrvs.obj" \
	"$(INTDIR)\libslp_findsrvtypes.obj" \
	"$(INTDIR)\libslp_handle.obj" \
	"$(INTDIR)\libslp_knownda.obj" \
	"$(INTDIR)\libslp_network.obj" \
	"$(INTDIR)\libslp_parse.obj" \
	"$(INTDIR)\libslp_property.obj" \
	"$(INTDIR)\libslp_reg.obj" \
	"$(INTDIR)\libslp_thread.obj" \
	"$(INTDIR)\libslp.res"

"$(OUTDIR)\libslp.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("libslp.dep")
!INCLUDE "libslp.dep"
!ELSE 
!MESSAGE Warning: cannot find "libslp.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "libslp - Win32 Release" || "$(CFG)" == "libslp - Win32 Debug"
SOURCE=.\libslp.rc

"$(INTDIR)\libslp.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\libslp_delattrs.c

"$(INTDIR)\libslp_delattrs.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\libslp_dereg.c

"$(INTDIR)\libslp_dereg.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\libslp_findattrs.c

"$(INTDIR)\libslp_findattrs.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\libslp_findscopes.c

"$(INTDIR)\libslp_findscopes.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\libslp_findsrvs.c

"$(INTDIR)\libslp_findsrvs.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\libslp_findsrvtypes.c

"$(INTDIR)\libslp_findsrvtypes.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\libslp_handle.c

"$(INTDIR)\libslp_handle.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\libslp_knownda.c

"$(INTDIR)\libslp_knownda.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\libslp_network.c

"$(INTDIR)\libslp_network.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\libslp_parse.c

"$(INTDIR)\libslp_parse.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\libslp_property.c

"$(INTDIR)\libslp_property.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\libslp_reg.c

"$(INTDIR)\libslp_reg.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\libslp_thread.c

"$(INTDIR)\libslp_thread.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

