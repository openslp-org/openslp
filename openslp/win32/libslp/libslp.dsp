# Microsoft Developer Studio Project File - Name="libslp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=libslp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libslp.mak".
!MESSAGE 
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

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libslp - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../../common" /D "_USRDLL" /D "LIBSLP_EXPORTS" /D "ENABLE" /D "_WINDOWS" /D "i386" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D SLP_VERSION=\"1.0.5\" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /machine:I386 /out:"Release/slp.dll"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy ..\..\libslp\slp.h release
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libslp - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBSLP_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../common" /D "_USRDLL" /D "LIBSLP_EXPORTS" /D "_WINDOWS" /D "i386" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D SLP_VERSION=\"1.0.7\" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /debug /machine:I386 /out:"Debug/slp.dll" /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy slp.h ...
PostBuild_Cmds=copy ..\..\libslp\slp.h debug
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "libslp - Win32 Release"
# Name "libslp - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\libslp.def
# End Source File
# Begin Source File

SOURCE=..\..\libslp\libslp_delattrs.c
# End Source File
# Begin Source File

SOURCE=..\..\libslp\libslp_dereg.c
# End Source File
# Begin Source File

SOURCE=..\..\libslp\libslp_findattrs.c
# End Source File
# Begin Source File

SOURCE=..\..\libslp\libslp_findscopes.c
# End Source File
# Begin Source File

SOURCE=..\..\libslp\libslp_findsrvs.c
# End Source File
# Begin Source File

SOURCE=..\..\libslp\libslp_findsrvtypes.c
# End Source File
# Begin Source File

SOURCE=..\..\libslp\libslp_handle.c
# End Source File
# Begin Source File

SOURCE=..\..\libslp\libslp_knownda.c
# End Source File
# Begin Source File

SOURCE=..\..\libslp\libslp_network.c
# End Source File
# Begin Source File

SOURCE=..\..\libslp\libslp_parse.c
# End Source File
# Begin Source File

SOURCE=..\..\libslp\libslp_property.c
# End Source File
# Begin Source File

SOURCE=..\..\libslp\libslp_reg.c
# End Source File
# Begin Source File

SOURCE=..\..\libslp\libslp_thread.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_buffer.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_compare.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_database.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_linkedlist.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_message.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_network.c
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
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\libslp\libslp.h
# End Source File
# Begin Source File

SOURCE=..\..\libslp\slp.h
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_buffer.h
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_compare.h
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_linkedlist.h
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
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
