# Microsoft Developer Studio Project File - Name="slpd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=slpd - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "slpd.mak".
!MESSAGE 
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

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "slpd - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "ENABLE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "ENABLE_SLPv1" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:windows /machine:I386
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy ..\..\libslp\slp.h release
# End Special Build Tool

!ELSEIF  "$(CFG)" == "slpd - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "ENABLE_SLPv1" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy slp.h ...
PostBuild_Cmds=copy ..\..\libslp\slp.h debug
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "slpd - Win32 Release"
# Name "slpd - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\common\slp_buffer.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_compare.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_da.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_linkedlist.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_logfile.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_message.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_property.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_utf8.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_v1message.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_xid.c
# End Source File
# Begin Source File

SOURCE=..\..\slpd\slpd_cmdline.c
# End Source File
# Begin Source File

SOURCE=..\..\slpd\slpd_database.c
# End Source File
# Begin Source File

SOURCE=..\..\slpd\slpd_incoming.c
# End Source File
# Begin Source File

SOURCE=..\..\slpd\slpd_knownda.c
# End Source File
# Begin Source File

SOURCE=..\..\slpd\slpd_log.c
# End Source File
# Begin Source File

SOURCE=..\..\slpd\slpd_main.c
# End Source File
# Begin Source File

SOURCE=..\..\slpd\slpd_outgoing.c
# End Source File
# Begin Source File

SOURCE=..\..\slpd\slpd_predicate.c
# End Source File
# Begin Source File

SOURCE=..\..\slpd\slpd_process.c
# End Source File
# Begin Source File

SOURCE=..\..\slpd\slpd_property.c
# End Source File
# Begin Source File

SOURCE=..\..\slpd\slpd_regfile.c
# End Source File
# Begin Source File

SOURCE=..\..\slpd\slpd_socket.c
# End Source File
# Begin Source File

SOURCE=..\..\slpd\slpd_v1process.c
# End Source File
# Begin Source File

SOURCE=..\..\slpd\slpd_win32.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\common\slp_buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_compare.h
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_da.h
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_linkedlist.h
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_logfile.h
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_message.h
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_network.h
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_property.h
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_v1message.h
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_xid.h
# End Source File
# Begin Source File

SOURCE=..\..\slpd\slpd.h
# End Source File
# Begin Source File

SOURCE=..\..\slpd\slpd_win32.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
