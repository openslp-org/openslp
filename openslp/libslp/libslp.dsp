# Microsoft Developer Studio Project File - Name="libslp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

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
!MESSAGE "libslp - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libslp - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
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
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I ".\\" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libslp - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ  /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".\\" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FD /GZ  /c
# SUBTRACT CPP /YX
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

# Name "libslp - Win32 Release"
# Name "libslp - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\libslp_delattrs.c

!IF  "$(CFG)" == "libslp - Win32 Release"

# ADD CPP /I ".\ ..\common"
# SUBTRACT CPP /I ".\\"

!ELSEIF  "$(CFG)" == "libslp - Win32 Debug"

# ADD CPP /I "..\common"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libslp_dereg.c

!IF  "$(CFG)" == "libslp - Win32 Release"

# ADD CPP /I ".\ ..\common"
# SUBTRACT CPP /I ".\\"

!ELSEIF  "$(CFG)" == "libslp - Win32 Debug"

# ADD CPP /I "..\common"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libslp_findattrs.c

!IF  "$(CFG)" == "libslp - Win32 Release"

# ADD CPP /I ".\ ..\common"
# SUBTRACT CPP /I ".\\"

!ELSEIF  "$(CFG)" == "libslp - Win32 Debug"

# ADD CPP /I "..\common"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libslp_findscopes.c

!IF  "$(CFG)" == "libslp - Win32 Release"

# ADD CPP /I ".\ ..\common"
# SUBTRACT CPP /I ".\\"

!ELSEIF  "$(CFG)" == "libslp - Win32 Debug"

# ADD CPP /I "..\common"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libslp_findsrvs.c

!IF  "$(CFG)" == "libslp - Win32 Release"

# ADD CPP /I ".\ ..\common"
# SUBTRACT CPP /I ".\\"

!ELSEIF  "$(CFG)" == "libslp - Win32 Debug"

# ADD CPP /I "..\common"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libslp_findsrvtypes.c

!IF  "$(CFG)" == "libslp - Win32 Release"

# ADD CPP /I ".\ ..\common"
# SUBTRACT CPP /I ".\\"

!ELSEIF  "$(CFG)" == "libslp - Win32 Debug"

# ADD CPP /I "..\common"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libslp_handle.c

!IF  "$(CFG)" == "libslp - Win32 Release"

# ADD CPP /I ".\ ..\common"
# SUBTRACT CPP /I ".\\"

!ELSEIF  "$(CFG)" == "libslp - Win32 Debug"

# ADD CPP /I "..\common"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libslp_knownda.c

!IF  "$(CFG)" == "libslp - Win32 Release"

# ADD CPP /I ".\ ..\common"
# SUBTRACT CPP /I ".\\"

!ELSEIF  "$(CFG)" == "libslp - Win32 Debug"

# ADD CPP /I "..\common"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libslp_network.c

!IF  "$(CFG)" == "libslp - Win32 Release"

# ADD CPP /I ".\ ..\common"
# SUBTRACT CPP /I ".\\"

!ELSEIF  "$(CFG)" == "libslp - Win32 Debug"

# ADD CPP /I "..\common"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libslp_parse.c

!IF  "$(CFG)" == "libslp - Win32 Release"

# ADD CPP /I ".\ ..\common"
# SUBTRACT CPP /I ".\\"

!ELSEIF  "$(CFG)" == "libslp - Win32 Debug"

# ADD CPP /I "..\common"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libslp_property.c

!IF  "$(CFG)" == "libslp - Win32 Release"

# ADD CPP /I ".\ ..\common"
# SUBTRACT CPP /I ".\\"

!ELSEIF  "$(CFG)" == "libslp - Win32 Debug"

# ADD CPP /I "..\common"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libslp_reg.c

!IF  "$(CFG)" == "libslp - Win32 Release"

# ADD CPP /I ".\ ..\common"
# SUBTRACT CPP /I ".\\"

!ELSEIF  "$(CFG)" == "libslp - Win32 Debug"

# ADD CPP /I "..\common"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\libslp_thread.c

!IF  "$(CFG)" == "libslp - Win32 Release"

# ADD CPP /I ".\ ..\common"
# SUBTRACT CPP /I ".\\"

!ELSEIF  "$(CFG)" == "libslp - Win32 Debug"

# ADD CPP /I "..\common"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
