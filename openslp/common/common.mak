# Microsoft Developer Studio Generated NMAKE File, Based on common.dsp
!IF "$(CFG)" == ""
CFG=common - Win32 Debug
!MESSAGE No configuration specified. Defaulting to common - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "common - Win32 Release" && "$(CFG)" != "common - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "common.mak" CFG="common - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "common - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "common - Win32 Debug" (based on "Win32 (x86) Static Library")
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

!IF  "$(CFG)" == "common - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\common.lib"


CLEAN :
	-@erase "$(INTDIR)\slp_buffer.obj"
	-@erase "$(INTDIR)\slp_compare.obj"
	-@erase "$(INTDIR)\slp_da.obj"
	-@erase "$(INTDIR)\slp_linkedlist.obj"
	-@erase "$(INTDIR)\slp_logfile.obj"
	-@erase "$(INTDIR)\slp_message.obj"
	-@erase "$(INTDIR)\slp_network.obj"
	-@erase "$(INTDIR)\slp_property.obj"
	-@erase "$(INTDIR)\slp_xid.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\common.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I ".\\" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fp"$(INTDIR)\common.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\common.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\common.lib" 
LIB32_OBJS= \
	"$(INTDIR)\slp_buffer.obj" \
	"$(INTDIR)\slp_compare.obj" \
	"$(INTDIR)\slp_da.obj" \
	"$(INTDIR)\slp_linkedlist.obj" \
	"$(INTDIR)\slp_logfile.obj" \
	"$(INTDIR)\slp_message.obj" \
	"$(INTDIR)\slp_network.obj" \
	"$(INTDIR)\slp_property.obj" \
	"$(INTDIR)\slp_xid.obj"

"$(OUTDIR)\common.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "common - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\common.lib"


CLEAN :
	-@erase "$(INTDIR)\slp_buffer.obj"
	-@erase "$(INTDIR)\slp_compare.obj"
	-@erase "$(INTDIR)\slp_da.obj"
	-@erase "$(INTDIR)\slp_linkedlist.obj"
	-@erase "$(INTDIR)\slp_logfile.obj"
	-@erase "$(INTDIR)\slp_message.obj"
	-@erase "$(INTDIR)\slp_network.obj"
	-@erase "$(INTDIR)\slp_property.obj"
	-@erase "$(INTDIR)\slp_xid.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\common.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fp"$(INTDIR)\common.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\common.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\common.lib" 
LIB32_OBJS= \
	"$(INTDIR)\slp_buffer.obj" \
	"$(INTDIR)\slp_compare.obj" \
	"$(INTDIR)\slp_da.obj" \
	"$(INTDIR)\slp_linkedlist.obj" \
	"$(INTDIR)\slp_logfile.obj" \
	"$(INTDIR)\slp_message.obj" \
	"$(INTDIR)\slp_network.obj" \
	"$(INTDIR)\slp_property.obj" \
	"$(INTDIR)\slp_xid.obj"

"$(OUTDIR)\common.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
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
!IF EXISTS("common.dep")
!INCLUDE "common.dep"
!ELSE 
!MESSAGE Warning: cannot find "common.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "common - Win32 Release" || "$(CFG)" == "common - Win32 Debug"
SOURCE=.\slp_buffer.c

!IF  "$(CFG)" == "common - Win32 Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /I ".\\" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fp"$(INTDIR)\common.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slp_buffer.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "common - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slp_buffer.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slp_compare.c

!IF  "$(CFG)" == "common - Win32 Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /I ".\\" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fp"$(INTDIR)\common.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slp_compare.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "common - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slp_compare.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slp_da.c

!IF  "$(CFG)" == "common - Win32 Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /I ".\\" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fp"$(INTDIR)\common.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slp_da.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "common - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slp_da.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slp_linkedlist.c

!IF  "$(CFG)" == "common - Win32 Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /I ".\\" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fp"$(INTDIR)\common.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slp_linkedlist.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "common - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slp_linkedlist.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slp_logfile.c

!IF  "$(CFG)" == "common - Win32 Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /I ".\\" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fp"$(INTDIR)\common.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slp_logfile.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "common - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slp_logfile.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slp_message.c

!IF  "$(CFG)" == "common - Win32 Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /I ".\\" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fp"$(INTDIR)\common.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slp_message.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "common - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slp_message.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slp_network.c

!IF  "$(CFG)" == "common - Win32 Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /I ".\\" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fp"$(INTDIR)\common.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slp_network.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "common - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slp_network.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slp_property.c

!IF  "$(CFG)" == "common - Win32 Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /I ".\\" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fp"$(INTDIR)\common.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slp_property.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "common - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slp_property.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\slp_xid.c

!IF  "$(CFG)" == "common - Win32 Release"

CPP_SWITCHES=/nologo /MT /W3 /GX /O2 /I ".\\" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fp"$(INTDIR)\common.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\slp_xid.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "common - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

"$(INTDIR)\slp_xid.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 


!ENDIF 

