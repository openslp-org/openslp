# Microsoft Developer Studio Project File - Name="libslpstatic" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libslpstatic - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libslpstatic.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libslpstatic.mak" CFG="libslpstatic - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libslpstatic - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libslpstatic - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libslpstatic - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libslpstatic - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../common" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_USRDLL" /D "WINDOWS" /D "i386" /D SLP_VERSION=\"1.1.1\" /D "LIBSLP_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libslpstatic - Win32 Release"
# Name "libslpstatic - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
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

SOURCE=..\..\common\slp_dhcp.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_iface.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_linkedlist.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_message.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_net.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_network.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_parse.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_pid.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_property.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_utf8.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_win32.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_xcast.c
# End Source File
# Begin Source File

SOURCE=..\..\common\slp_xid.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
