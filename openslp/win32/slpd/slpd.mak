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
!MESSAGE "slpd - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "slpd - Win32 Debug" (based on "Win32 (x86) Application")
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

!IF  "$(CFG)" == "slpd - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\slpd.exe"


CLEAN :
	-@erase "$(INTDIR)\libslpattr.obj"
	-@erase "$(INTDIR)\slp_buffer.obj"
	-@erase "$(INTDIR)\slp_compare.obj"
	-@erase "$(INTDIR)\slp_database.obj"
	-@erase "$(INTDIR)\slp_dhcp.obj"
	-@erase "$(INTDIR)\slp_iface.obj"
	-@erase "$(INTDIR)\slp_linkedlist.obj"
	-@erase "$(INTDIR)\slp_message.obj"
	-@erase "$(INTDIR)\slp_net.obj"
	-@erase "$(INTDIR)\slp_parse.obj"
	-@erase "$(INTDIR)\slp_pid.obj"
	-@erase "$(INTDIR)\slp_property.obj"
	-@erase "$(INTDIR)\slp_utf8.obj"
	-@erase "$(INTDIR)\slp_v1message.obj"
	-@erase "$(INTDIR)\slp_xid.obj"
	-@erase "$(INTDIR)\slpd.idb"
	-@erase "$(INTDIR)\slpd_cmdline.obj"
	-@erase "$(INTDIR)\slpd_database.obj"
	-@erase "$(INTDIR)\slpd_incoming.obj"
	-@erase "$(INTDIR)\slpd_knownda.obj"
	-@erase "$(INTDIR)\slpd_log.obj"
	-@erase "$(INTDIR)\slpd_main.obj"
	-@erase "$(INTDIR)\slpd_outgoing.obj"
	-@erase "$(INTDIR)\slpd_predicate.obj"
	-@erase "$(INTDIR)\slpd_process.obj"
	-@erase "$(INTDIR)\slpd_property.obj"
	-@erase "$(INTDIR)\slpd_regfile.obj"
	-@erase "$(INTDIR)\slpd_socket.obj"
	-@erase "$(INTDIR)\slpd_v1process.obj"
	-@erase "$(INTDIR)\slpd_win32.obj"
	-@erase "$(OUTDIR)\slpd.exe"
	-@erase "$(OUTDIR)\slpd.map"
	-@erase "$(OUTDIR)\slpd.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /Zi /O2 /I "../../common" /D "ENABLE" /D "ENABLE_SLPv1" /D "_WINDOWS" /D "i386" /D "USE_PREDICATES" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D SLP_VERSION=\"1.1.1\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\slpd.pdb" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\slpd.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\slpd.pdb" /map:"$(INTDIR)\slpd.map" /debug /machine:I386 /out:"$(OUTDIR)\slpd.exe" 
LINK32_OBJS= \
	"$(INTDIR)\libslpattr.obj" \
	"$(INTDIR)\slp_buffer.obj" \
	"$(INTDIR)\slp_compare.obj" \
	"$(INTDIR)\slp_database.obj" \
	"$(INTDIR)\slp_dhcp.obj" \
	"$(INTDIR)\slp_iface.obj" \
	"$(INTDIR)\slp_linkedlist.obj" \
	"$(INTDIR)\slp_message.obj" \
	"$(INTDIR)\slp_net.obj" \
	"$(INTDIR)\slp_parse.obj" \
	"$(INTDIR)\slp_pid.obj" \
	"$(INTDIR)\slp_property.obj" \
	"$(INTDIR)\slp_utf8.obj" \
	"$(INTDIR)\slp_v1message.obj" \
	"$(INTDIR)\slp_xid.obj" \
	"$(INTDIR)\slpd_cmdline.obj" \
	"$(INTDIR)\slpd_database.obj" \
	"$(INTDIR)\slpd_incoming.obj" \
	"$(INTDIR)\slpd_knownda.obj" \
	"$(INTDIR)\slpd_log.obj" \
	"$(INTDIR)\slpd_main.obj" \
	"$(INTDIR)\slpd_outgoing.obj" \
	"$(INTDIR)\slpd_predicate.obj" \
	"$(INTDIR)\slpd_process.obj" \
	"$(INTDIR)\slpd_property.obj" \
	"$(INTDIR)\slpd_regfile.obj" \
	"$(INTDIR)\slpd_socket.obj" \
	"$(INTDIR)\slpd_v1process.obj" \
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
	-@erase "$(INTDIR)\libslpattr.obj"
	-@erase "$(INTDIR)\slp_buffer.obj"
	-@erase "$(INTDIR)\slp_compare.obj"
	-@erase "$(INTDIR)\slp_database.obj"
	-@erase "$(INTDIR)\slp_dhcp.obj"
	-@erase "$(INTDIR)\slp_iface.obj"
	-@erase "$(INTDIR)\slp_linkedlist.obj"
	-@erase "$(INTDIR)\slp_message.obj"
	-@erase "$(INTDIR)\slp_net.obj"
	-@erase "$(INTDIR)\slp_parse.obj"
	-@erase "$(INTDIR)\slp_pid.obj"
	-@erase "$(INTDIR)\slp_property.obj"
	-@erase "$(INTDIR)\slp_utf8.obj"
	-@erase "$(INTDIR)\slp_v1message.obj"
	-@erase "$(INTDIR)\slp_xid.obj"
	-@erase "$(INTDIR)\slpd.idb"
	-@erase "$(INTDIR)\slpd_cmdline.obj"
	-@erase "$(INTDIR)\slpd_database.obj"
	-@erase "$(INTDIR)\slpd_incoming.obj"
	-@erase "$(INTDIR)\slpd_knownda.obj"
	-@erase "$(INTDIR)\slpd_log.obj"
	-@erase "$(INTDIR)\slpd_main.obj"
	-@erase "$(INTDIR)\slpd_outgoing.obj"
	-@erase "$(INTDIR)\slpd_predicate.obj"
	-@erase "$(INTDIR)\slpd_process.obj"
	-@erase "$(INTDIR)\slpd_property.obj"
	-@erase "$(INTDIR)\slpd_regfile.obj"
	-@erase "$(INTDIR)\slpd_socket.obj"
	-@erase "$(INTDIR)\slpd_v1process.obj"
	-@erase "$(INTDIR)\slpd_win32.obj"
	-@erase "$(OUTDIR)\slpd.exe"
	-@erase "$(OUTDIR)\slpd.ilk"
	-@erase "$(OUTDIR)\slpd.map"
	-@erase "$(OUTDIR)\slpd.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /Gm /ZI /Od /I "../../common" /D "ENABLE_SLPv1" /D "_WINDOWS" /D "i386" /D "USE_PREDICATES" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D SLP_VERSION=\"1.1.1\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\slpd.pdb" /FD /GZ /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\slpd.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\slpd.pdb" /map:"$(INTDIR)\slpd.map" /debug /machine:I386 /out:"$(OUTDIR)\slpd.exe" 
LINK32_OBJS= \
	"$(INTDIR)\libslpattr.obj" \
	"$(INTDIR)\slp_buffer.obj" \
	"$(INTDIR)\slp_compare.obj" \
	"$(INTDIR)\slp_database.obj" \
	"$(INTDIR)\slp_dhcp.obj" \
	"$(INTDIR)\slp_iface.obj" \
	"$(INTDIR)\slp_linkedlist.obj" \
	"$(INTDIR)\slp_message.obj" \
	"$(INTDIR)\slp_net.obj" \
	"$(INTDIR)\slp_parse.obj" \
	"$(INTDIR)\slp_pid.obj" \
	"$(INTDIR)\slp_property.obj" \
	"$(INTDIR)\slp_utf8.obj" \
	"$(INTDIR)\slp_v1message.obj" \
	"$(INTDIR)\slp_xid.obj" \
	"$(INTDIR)\slpd_cmdline.obj" \
	"$(INTDIR)\slpd_database.obj" \
	"$(INTDIR)\slpd_incoming.obj" \
	"$(INTDIR)\slpd_knownda.obj" \
	"$(INTDIR)\slpd_log.obj" \
	"$(INTDIR)\slpd_main.obj" \
	"$(INTDIR)\slpd_outgoing.obj" \
	"$(INTDIR)\slpd_predicate.obj" \
	"$(INTDIR)\slpd_process.obj" \
	"$(INTDIR)\slpd_property.obj" \
	"$(INTDIR)\slpd_regfile.obj" \
	"$(INTDIR)\slpd_socket.obj" \
	"$(INTDIR)\slpd_v1process.obj" \
	"$(INTDIR)\slpd_win32.obj"

"$(OUTDIR)\slpd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
!IF EXISTS("slpd.dep")
!INCLUDE "slpd.dep"
!ELSE 
!MESSAGE Warning: cannot find "slpd.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "slpd - Win32 Release" || "$(CFG)" == "slpd - Win32 Debug"
SOURCE=..\..\libslpattr\libslpattr.c

"$(INTDIR)\libslpattr.obj" : $(SOURCE) "$(INTDIR)"
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


SOURCE=..\..\common\slp_dhcp.c

"$(INTDIR)\slp_dhcp.obj" : $(SOURCE) "$(INTDIR)"
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


SOURCE=..\..\common\slp_xid.c

"$(INTDIR)\slp_xid.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\slpd\slpd_cmdline.c

"$(INTDIR)\slpd_cmdline.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\slpd\slpd_database.c

"$(INTDIR)\slpd_database.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\slpd\slpd_incoming.c

"$(INTDIR)\slpd_incoming.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\slpd\slpd_knownda.c

"$(INTDIR)\slpd_knownda.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\slpd\slpd_log.c

"$(INTDIR)\slpd_log.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\slpd\slpd_main.c

"$(INTDIR)\slpd_main.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\slpd\slpd_outgoing.c

"$(INTDIR)\slpd_outgoing.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\slpd\slpd_predicate.c

"$(INTDIR)\slpd_predicate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\slpd\slpd_process.c

"$(INTDIR)\slpd_process.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\slpd\slpd_property.c

"$(INTDIR)\slpd_property.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\slpd\slpd_regfile.c

"$(INTDIR)\slpd_regfile.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\slpd\slpd_socket.c

"$(INTDIR)\slpd_socket.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\\slpd\slpd_v1process.c

"$(INTDIR)\slpd_v1process.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\slpd\slpd_win32.c

"$(INTDIR)\slpd_win32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

