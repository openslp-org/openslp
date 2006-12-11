@echo off

rem -------------------------------------------------------------------------
rem Usage: build [--ver <version>] [--arch <arch>] [--help]
rem
rem build.cmd rebuilds the openslp installer against wix 3.0 (build 1821)
rem -------------------------------------------------------------------------

setlocal

if "%WIX_PATH"=="" goto dohelp

rem -------------------------------------------------------------------------
rem Please ensure that your path contains the WIX tool set directory, and 
rem that your environment contains a WIX_PATH variable that points to the
rem root of the WIX tool set directory.

set version=2.0.0
set machine=x86

:doparse
rem -------------------------------------------------------------------------
shift
if "%0%" == "--ver"     goto doversion
if "%0%" == "--arch"    goto domachine
if "%0%" == "--help"    goto dohelp
if "%0%" == ""          goto dobuild
goto dohelp

:doversion
shift
set version=%0%
goto doparse

:domachine
shift
set machine=%0%
goto doparse

:dohelp
echo.
echo Usage: build.cmd [--ver <version>] [--arch <arch>] [--help]
echo.
echo Where <version> is a three-part dot-separated version number of OpenSLP.
echo and <arch> is a machine architecture tag, eg., x86, x86_64, alpha, etc.
echo Use --help to get this help screen.
echo.
echo Default: "build --ver=%version% --arch=%machine%"
echo.
echo NOTE! Please ensure that the WIX_PATH environment variable contains the
echo directory in which the WIX tool set has been installed.
goto doexit

:dobuild
rem -------------------------------------------------------------------------
echo.
echo Building openslp_%version%_%machine%.msi...

candle -nologo -dVERSION=%version% openslp.wxs
light -nologo -out openslp_%version%_%machine%.msi openslp.wixobj -ext WixUIExtension -cultures:en-us -sval

:doexit
endlocal

