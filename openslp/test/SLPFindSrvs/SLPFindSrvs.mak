# Microsoft Developer Studio Generated NMAKE File, Based on SLPFindSrvs.dsp
!IF "$(CFG)" == ""
CFG=SLPFindSrvs - Win32 Debug
!MESSAGE No configuration specified. Defaulting to SLPFindSrvs - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "SLPFindSrvs - Win32 Release" && "$(CFG)" != "SLPFindSrvs - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SLPFindSrvs.mak" CFG="SLPFindSrvs - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SLPFindSrvs - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "SLPFindSrvs - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SLPFindSrvs - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\SLPFindSrvs.exe"


CLEAN :
	-@erase "$(INTDIR)\SLPFindSrvs.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\SLPFindSrvs.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "..\\" /I "..\..\common" /I "..\..\libslp" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\SLPFindSrvs.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\common\Release\common.lib ..\..\libslp\Release\libslp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib wsock32.lib winmm.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\SLPFindSrvs.pdb" /machine:I386 /out:"$(OUTDIR)\SLPFindSrvs.exe" 
LINK32_OBJS= \
	"$(INTDIR)\SLPFindSrvs.obj"

"$(OUTDIR)\SLPFindSrvs.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "SLPFindSrvs - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\SLPFindSrvs.exe"


CLEAN :
	-@erase "$(INTDIR)\SLPFindSrvs.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\SLPFindSrvs.exe"
	-@erase "$(OUTDIR)\SLPFindSrvs.ilk"
	-@erase "$(OUTDIR)\SLPFindSrvs.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "..\\" /I "..\..\common" /I "..\..\libslp" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\SLPFindSrvs.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\..\common\Debug\common.lib ..\..\libslp\Debug\libslp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib wsock32.lib winmm.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\SLPFindSrvs.pdb" /debug /machine:I386 /out:"$(OUTDIR)\SLPFindSrvs.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\SLPFindSrvs.obj"

"$(OUTDIR)\SLPFindSrvs.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("SLPFindSrvs.dep")
!INCLUDE "SLPFindSrvs.dep"
!ELSE 
!MESSAGE Warning: cannot find "SLPFindSrvs.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "SLPFindSrvs - Win32 Release" || "$(CFG)" == "SLPFindSrvs - Win32 Debug"
SOURCE=.\SLPFindSrvs.c

!IF  "$(CFG)" == "SLPFindSrvs - Win32 Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /I "..\\" /I "..\..\common" /I "..\..\libslp" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\SLPFindSrvs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "SLPFindSrvs - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\\" /I "..\..\common" /I "..\..\libslp" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\SLPFindSrvs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

