# Microsoft Developer Studio Generated NMAKE File, Based on slpd.dsp
!IF "$(CFG)" == ""
CFG=slpd - Win32 Debug
!MESSAGE No configuration specified. Defaulting to slpd - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "slpd - Win32 Release" && "$(CFG)" != "slpd - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "slpd.mak" CFG="slpd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "slpd - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "slpd - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "slpd - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\slpd.exe"


CLEAN :
	-@erase "$(INTDIR)\slpd_cmdline.obj"
	-@erase "$(INTDIR)\slpd_database.obj"
	-@erase "$(INTDIR)\slpd_incoming.obj"
	-@erase "$(INTDIR)\slpd_knownda.obj"
	-@erase "$(INTDIR)\slpd_log.obj"
	-@erase "$(INTDIR)\slpd_main.obj"
	-@erase "$(INTDIR)\slpd_outgoing.obj"
	-@erase "$(INTDIR)\slpd_process.obj"
	-@erase "$(INTDIR)\slpd_property.obj"
	-@erase "$(INTDIR)\slpd_regfile.obj"
	-@erase "$(INTDIR)\slpd_socket.obj"
	-@erase "$(INTDIR)\slpd_win32.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\slpd.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\slpd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\slpd.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\common\Release\common.lib ..\libslp\Release\libslp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib winmm.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\slpd.pdb" /machine:I386 /out:"$(OUTDIR)\slpd.exe" 
LINK32_OBJS= \
	"$(INTDIR)\slpd_cmdline.obj" \
	"$(INTDIR)\slpd_database.obj" \
	"$(INTDIR)\slpd_incoming.obj" \
	"$(INTDIR)\slpd_knownda.obj" \
	"$(INTDIR)\slpd_log.obj" \
	"$(INTDIR)\slpd_main.obj" \
	"$(INTDIR)\slpd_outgoing.obj" \
	"$(INTDIR)\slpd_process.obj" \
	"$(INTDIR)\slpd_property.obj" \
	"$(INTDIR)\slpd_regfile.obj" \
	"$(INTDIR)\slpd_socket.obj" \
	"$(INTDIR)\slpd_win32.obj"

"$(OUTDIR)\slpd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "slpd - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\slpd.exe"


CLEAN :
	-@erase "$(INTDIR)\slpd_cmdline.obj"
	-@erase "$(INTDIR)\slpd_database.obj"
	-@erase "$(INTDIR)\slpd_incoming.obj"
	-@erase "$(INTDIR)\slpd_knownda.obj"
	-@erase "$(INTDIR)\slpd_log.obj"
	-@erase "$(INTDIR)\slpd_main.obj"
	-@erase "$(INTDIR)\slpd_outgoing.obj"
	-@erase "$(INTDIR)\slpd_process.obj"
	-@erase "$(INTDIR)\slpd_property.obj"
	-@erase "$(INTDIR)\slpd_regfile.obj"
	-@erase "$(INTDIR)\slpd_socket.obj"
	-@erase "$(INTDIR)\slpd_win32.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\slpd.exe"
	-@erase "$(OUTDIR)\slpd.ilk"
	-@erase "$(OUTDIR)\slpd.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\slpd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\slpd.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=..\common\Debug\common.lib ..\libslp\Debug\libslp.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib winmm.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\slpd.pdb" /debug /machine:I386 /out:"$(OUTDIR)\slpd.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\slpd_cmdline.obj" \
	"$(INTDIR)\slpd_database.obj" \
	"$(INTDIR)\slpd_incoming.obj" \
	"$(INTDIR)\slpd_knownda.obj" \
	"$(INTDIR)\slpd_log.obj" \
	"$(INTDIR)\slpd_main.obj" \
	"$(INTDIR)\slpd_outgoing.obj" \
	"$(INTDIR)\slpd_process.obj" \
	"$(INTDIR)\slpd_property.obj" \
	"$(INTDIR)\slpd_regfile.obj" \
	"$(INTDIR)\slpd_socket.obj" \
	"$(INTDIR)\slpd_win32.obj"

"$(OUTDIR)\slpd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("slpd.dep")
!INCLUDE "slpd.dep"
!ELSE 
!MESSAGE Warning: cannot find "slpd.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "slpd - Win32 Release" || "$(CFG)" == "slpd - Win32 Debug"
SOURCE=.\slpd_cmdline.c

!IF  "$(CFG)" == "slpd - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D VERSION="0.7.5" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slpd_cmdline.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "slpd - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slpd_cmdline.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slpd_database.c

!IF  "$(CFG)" == "slpd - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D VERSION="0.7.5" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slpd_database.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "slpd - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slpd_database.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slpd_incoming.c

!IF  "$(CFG)" == "slpd - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D VERSION="0.7.5" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slpd_incoming.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "slpd - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slpd_incoming.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slpd_knownda.c

!IF  "$(CFG)" == "slpd - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D VERSION="0.7.5" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slpd_knownda.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "slpd - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slpd_knownda.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slpd_log.c

!IF  "$(CFG)" == "slpd - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D VERSION="0.7.5" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slpd_log.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "slpd - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slpd_log.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slpd_main.c

!IF  "$(CFG)" == "slpd - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D VERSION="0.7.5" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slpd_main.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "slpd - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slpd_main.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slpd_outgoing.c

!IF  "$(CFG)" == "slpd - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D VERSION="0.7.5" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slpd_outgoing.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "slpd - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slpd_outgoing.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slpd_process.c

!IF  "$(CFG)" == "slpd - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D VERSION="0.7.5" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slpd_process.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "slpd - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slpd_process.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slpd_property.c

!IF  "$(CFG)" == "slpd - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D VERSION="0.7.5" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slpd_property.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "slpd - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slpd_property.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slpd_regfile.c

!IF  "$(CFG)" == "slpd - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D VERSION="0.7.5" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slpd_regfile.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "slpd - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slpd_regfile.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slpd_socket.c

!IF  "$(CFG)" == "slpd - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D VERSION="0.7.5" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slpd_socket.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "slpd - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "./" /I "../common" /I "../libslp" /I "..\common" /I "..\libslp" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slpd_socket.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slpd_win32.c

!IF  "$(CFG)" == "slpd - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "..\common" /I "..\libslp" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\slpd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slpd_win32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "slpd - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "..\common" /I "..\libslp" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\slpd.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slpd_win32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

