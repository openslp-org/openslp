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

!IF  "$(CFG)" == "libslp - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release\obj
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\slp.dll"


CLEAN :
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
	-@erase "$(INTDIR)\slp.idb"
	-@erase "$(INTDIR)\slp.pdb"
	-@erase "$(INTDIR)\slp_buffer.obj"
	-@erase "$(INTDIR)\slp_compare.obj"
	-@erase "$(INTDIR)\slp_database.obj"
	-@erase "$(INTDIR)\slp_iface.obj"
	-@erase "$(INTDIR)\slp_linkedlist.obj"
	-@erase "$(INTDIR)\slp_message.obj"
	-@erase "$(INTDIR)\slp_net.obj"
	-@erase "$(INTDIR)\slp_network.obj"
	-@erase "$(INTDIR)\slp_parse.obj"
	-@erase "$(INTDIR)\slp_pid.obj"
	-@erase "$(INTDIR)\slp_property.obj"
	-@erase "$(INTDIR)\slp_utf8.obj"
	-@erase "$(INTDIR)\slp_v1message.obj"
	-@erase "$(INTDIR)\slp_xcast.obj"
	-@erase "$(INTDIR)\slp_xid.obj"
	-@erase "$(OUTDIR)\obj\slp.map"
	-@erase "$(OUTDIR)\slp.dll"
	-@erase "$(OUTDIR)\slp.exp"
	-@erase "$(OUTDIR)\slp.lib"
	-@erase "$(OUTDIR)\slp.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /Zi /O2 /I "../../common" /D "_USRDLL" /D "LIBSLP_EXPORTS" /D "ENABLE" /D "_WINDOWS" /D "i386" /D "NDEBUG" /D "__WIN32__" /D "WIN32" /D "_MBCS" /D SLP_VERSION=\"1.0.5\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\slp.pdb" /FD /c 

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

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libslp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\slp.pdb" /map:"$(INTDIR)\slp.map" /debug /machine:I386 /def:".\libslp.def" /out:"$(OUTDIR)\slp.dll" /implib:"$(OUTDIR)\slp.lib" 
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
	"$(INTDIR)\slp_buffer.obj" \
	"$(INTDIR)\slp_compare.obj" \
	"$(INTDIR)\slp_database.obj" \
	"$(INTDIR)\slp_iface.obj" \
	"$(INTDIR)\slp_linkedlist.obj" \
	"$(INTDIR)\slp_message.obj" \
	"$(INTDIR)\slp_net.obj" \
	"$(INTDIR)\slp_network.obj" \
	"$(INTDIR)\slp_parse.obj" \
	"$(INTDIR)\slp_pid.obj" \
	"$(INTDIR)\slp_property.obj" \
	"$(INTDIR)\slp_utf8.obj" \
	"$(INTDIR)\slp_v1message.obj" \
	"$(INTDIR)\slp_xcast.obj" \
	"$(INTDIR)\slp_xid.obj"

"$(OUTDIR)\slp.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\slp.dll"
   copy ..\..\libslp\slp.h release
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "libslp - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug\obj
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\slp.dll"


CLEAN :
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
	-@erase "$(INTDIR)\slp.idb"
	-@erase "$(INTDIR)\slp.pdb"
	-@erase "$(INTDIR)\slp_buffer.obj"
	-@erase "$(INTDIR)\slp_compare.obj"
	-@erase "$(INTDIR)\slp_database.obj"
	-@erase "$(INTDIR)\slp_iface.obj"
	-@erase "$(INTDIR)\slp_linkedlist.obj"
	-@erase "$(INTDIR)\slp_message.obj"
	-@erase "$(INTDIR)\slp_net.obj"
	-@erase "$(INTDIR)\slp_network.obj"
	-@erase "$(INTDIR)\slp_parse.obj"
	-@erase "$(INTDIR)\slp_pid.obj"
	-@erase "$(INTDIR)\slp_property.obj"
	-@erase "$(INTDIR)\slp_utf8.obj"
	-@erase "$(INTDIR)\slp_v1message.obj"
	-@erase "$(INTDIR)\slp_xcast.obj"
	-@erase "$(INTDIR)\slp_xid.obj"
	-@erase "$(OUTDIR)\obj\slp.map"
	-@erase "$(OUTDIR)\slp.dll"
	-@erase "$(OUTDIR)\slp.exp"
	-@erase "$(OUTDIR)\slp.ilk"
	-@erase "$(OUTDIR)\slp.lib"
	-@erase "$(OUTDIR)\slp.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /ZI /Od /I "../../common" /D "_USRDLL" /D "LIBSLP_EXPORTS" /D "_WINDOWS" /D "i386" /D "_DEBUG" /D "__WIN32__" /D "WIN32" /D "_MBCS" /D SLP_VERSION=\"1.0.5\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\slp.pdb" /FD /GZ /c 

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

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libslp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\slp.pdb" /map:"$(INTDIR)\slp.map" /debug /machine:I386 /def:".\libslp.def" /out:"$(OUTDIR)\slp.dll" /implib:"$(OUTDIR)\slp.lib" 
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
	"$(INTDIR)\slp_buffer.obj" \
	"$(INTDIR)\slp_compare.obj" \
	"$(INTDIR)\slp_database.obj" \
	"$(INTDIR)\slp_iface.obj" \
	"$(INTDIR)\slp_linkedlist.obj" \
	"$(INTDIR)\slp_message.obj" \
	"$(INTDIR)\slp_net.obj" \
	"$(INTDIR)\slp_network.obj" \
	"$(INTDIR)\slp_parse.obj" \
	"$(INTDIR)\slp_pid.obj" \
	"$(INTDIR)\slp_property.obj" \
	"$(INTDIR)\slp_utf8.obj" \
	"$(INTDIR)\slp_v1message.obj" \
	"$(INTDIR)\slp_xcast.obj" \
	"$(INTDIR)\slp_xid.obj"

"$(OUTDIR)\slp.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE="$(InputPath)"
PostBuild_Desc=Copy slp.h ...
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\slp.dll"
   copy ..\..\libslp\slp.h debug
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("libslp.dep")
!INCLUDE "libslp.dep"
!ELSE 
!MESSAGE Warning: cannot find "libslp.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "libslp - Win32 Release" || "$(CFG)" == "libslp - Win32 Debug"
SOURCE=..\..\libslp\libslp_delattrs.c

"$(INTDIR)\libslp_delattrs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\libslp\libslp_dereg.c

"$(INTDIR)\libslp_dereg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\libslp\libslp_findattrs.c

"$(INTDIR)\libslp_findattrs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\libslp\libslp_findscopes.c

"$(INTDIR)\libslp_findscopes.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\libslp\libslp_findsrvs.c

"$(INTDIR)\libslp_findsrvs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\libslp\libslp_findsrvtypes.c

"$(INTDIR)\libslp_findsrvtypes.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\libslp\libslp_handle.c

"$(INTDIR)\libslp_handle.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\libslp\libslp_knownda.c

"$(INTDIR)\libslp_knownda.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\libslp\libslp_network.c

"$(INTDIR)\libslp_network.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\libslp\libslp_parse.c

"$(INTDIR)\libslp_parse.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\libslp\libslp_property.c

"$(INTDIR)\libslp_property.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\libslp\libslp_reg.c

"$(INTDIR)\libslp_reg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\libslp\libslp_thread.c

"$(INTDIR)\libslp_thread.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\slp_buffer.c

"$(INTDIR)\slp_buffer.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\slp_compare.c

"$(INTDIR)\slp_compare.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\slp_database.c

"$(INTDIR)\slp_database.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\slp_iface.c

"$(INTDIR)\slp_iface.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\slp_linkedlist.c

"$(INTDIR)\slp_linkedlist.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\slp_message.c

"$(INTDIR)\slp_message.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\slp_net.c

"$(INTDIR)\slp_net.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\slp_network.c

"$(INTDIR)\slp_network.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\slp_parse.c

"$(INTDIR)\slp_parse.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\slp_pid.c

"$(INTDIR)\slp_pid.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\slp_property.c

"$(INTDIR)\slp_property.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\slp_utf8.c

"$(INTDIR)\slp_utf8.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\slp_v1message.c

"$(INTDIR)\slp_v1message.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\slp_xcast.c

"$(INTDIR)\slp_xcast.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\slp_xid.c

"$(INTDIR)\slp_xid.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

