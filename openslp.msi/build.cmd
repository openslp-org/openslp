@echo off

rem -------------------------------------------------------------------------
rem Usage: build [--version <version>] [--revision <revision>]
rem              [--arch <win32|x64>] [--build <debug|release>]
rem              [--help]
rem
rem build.cmd rebuilds the openslp installer against wix 3.5
rem -------------------------------------------------------------------------

setlocal

if NOT "%WIX_PATH" == "" goto dostart
echo *** Error: You must point the WIX_PATH environment variable at your WIX binaries.
echo.
goto dohelp

:dostart
rem -------------------------------------------------------------------------
rem Please ensure that your path contains the WIX tool set directory, and 
rem that your environment contains a WIX_PATH variable that points to the
rem root of the WIX tool set directory.

set package=openslp
set version=2.0.0
set revision=0
set arch=win32
set build=release

:doparse
rem -------------------------------------------------------------------------
shift
if "%~0" == "--version"  set version=%~1
if "%~0" == "--revision" set revision=%~1
if "%~0" == "--arch"     set arch=%~1
if "%~0" == "--build"    set build=%~1
if "%~0" == "--help"     goto dohelp
if "%~0" == ""           goto docheckarch
shift
goto doparse

:dohelp
echo.
echo Usage: build.cmd [--version ^<version^>] [--revision ^<revision^>]
echo                  [--arch ^<win32^|x64^>] [--build ^<debug^|release^>]
echo                  [--help]
echo.
echo Where ^<version^> is a three-part dot-separated version number of OpenSLP
echo and ^<revision^> is any string value that can be interpreted as a revision.
echo.
echo Default: build --version %version% --revision %revision% --arch %arch% --build %build%
echo.
echo Use --help to get this help screen.
echo.
echo NOTE! Please ensure that the WIX_PATH environment variable contains the
echo directory in which the WIX tool set has been installed.
echo.
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
set machine=x86
if "%arch%" == "x64" set machine=%arch%

set version_revision=%version%
if NOT "%revision%" == "" set version_revision=%version_revision%_%revision%

set machine_build=%machine%
if NOT "%build%" == "release" set machine_build=%machine_build%_%build%

set output=%package%_%version_revision%_%machine_build%.msi

echo.
echo Building %output%...

%WIX_PATH%\candle -nologo -dVERSION=%version% -dREVISION=%revision% -dMACHINE=%machine% -dBUILD=%build% %package%.wxs
%WIX_PATH%\light -nologo -out %output% %package%.wixobj -ext WixUIExtension -cultures:en-us -sval

:doexit
endlocal
