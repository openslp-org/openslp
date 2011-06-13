@echo off

rem -------------------------------------------------------------------------
rem Usage: makesrc [--src <srcdir>] [--tgt <tgtdir>] 
rem                [--arch <win32|x64>] [--build <debug|release>]
rem                [--help]
rem 
rem makesrc.cmd creates either a debug or release, win32 or x64 code 
rem installation directory structure from command line input. The default 
rem architecture and build type are win32 and release (respectively).
rem -------------------------------------------------------------------------

setlocal

set source=..\openslp
set target=.
set build=release
set arch=win32

:doparse
rem -------------------------------------------------------------------------
shift
if "%~0" == "--src"     set src=%~1
if "%~0" == "--tgt"     set target=%~1
if "%~0" == "--arch"    set arch=%~1
if "%~0" == "--build"	set build=%~1
if "%~0" == ""			goto docheckarch
shift
goto doparse

:dohelp
echo.
echo Usage: makesrc [--src ^<srcdir^>] [--tgt ^<tgtdir^>] 
echo                [--arch ^<win32^|x64^>] [--build ^<debug^|release^>] [--help]
echo.
echo Where ^<srcdir^> is the location of the root of the openslp source directory, 
echo and ^<tgtdir^> is the location to build the install directory structure.
echo The --build option allows you to specify debug or release build (release is the
echo default). The --arch option allows you to specify win32 or x64 architecture
echo (win32 is the default). Use --help to get this help screen.
echo.
echo Default: "makesrc --src %source% --tgt %target% --build %build% --arch %arch%"
echo.
echo NOTE! Please ensure that the WIX_PATH environment variable contains the
echo directory in which the WIX tool set has been installed.
goto doexit

:docheckarch
rem -------------------------------------------------------------------------
rem Be sure they're using a valid architecture...
if "%arch%" == "win32" goto docheckbuild
if "%arch%" == "x64" goto docheckbuild
echo *** Error: Unknown architecture %arch%.
goto dohelp

:docheckbuild
if "%build%" == "debug" goto dobuild
if "%build%" == "release" goto dobuild
echo *** Error: Unknown build type %build%.
goto dohelp

:dobuild
rem -------------------------------------------------------------------------
rem State what we're going to do...
echo Copying %arch% %build% files from %source% into %target%\OpenSLP.

rem Remove old source directory (if exists), create new structure
if EXIST %target%\OpenSLP rmdir /s /q %target%\OpenSLP

rem Create target directory structure
mkdir %target%\OpenSLP\Docs\rfc
mkdir %target%\OpenSLP\Docs\html
mkdir %target%\OpenSLP\Include
mkdir %target%\OpenSLP\Lib

rem Copy files from %source% to %target%[%build%]
xcopy %source%\doc\doc\rfc\* %target%\OpenSLP\Docs\rfc /S /I
xcopy %source%\doc\doc\html\* %target%\OpenSLP\Docs\html /S /I
xcopy %source%\libslp\slp.h %target%\OpenSLP\Include
xcopy %source%\win32\%arch%\%build%\slp.lib %target%\OpenSLP\Lib
xcopy %source%\win32\%arch%\%build%\slpstatic.lib %target%\OpenSLP\Lib
xcopy %source%\win32\%arch%\%build%\slp.dll %target%\OpenSLP
xcopy %source%\win32\%arch%\%build%\slp.pdb %target%\OpenSLP
xcopy %source%\win32\%arch%\%build%\slpd.exe %target%\OpenSLP
xcopy %source%\win32\%arch%\%build%\slpd.pdb %target%\OpenSLP
xcopy %source%\win32\%arch%\%build%\slptool.exe %target%\OpenSLP
xcopy %source%\win32\%arch%\%build%\slptool.pdb %target%\OpenSLP

:doexit
endlocal
