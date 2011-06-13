@echo off

rem -------------------------------------------------------------------------
rem Usage: build [--version <version>] [--revision <revision>]
rem              [--machine <x86|amd64>] [--build <debug|release>]
rem              [--help]
rem
rem build.cmd rebuilds the openslp installer against wix 3.0 (build 1821)
rem -------------------------------------------------------------------------

setlocal

if "%WIX_PATH" == "" goto dohelp

rem -------------------------------------------------------------------------
rem Please ensure that your path contains the WIX tool set directory, and 
rem that your environment contains a WIX_PATH variable that points to the
rem root of the WIX tool set directory.

set package=openslp
set version=2.0.0
set revision=beta2
set machine=x86
set build=release

:doparse
rem -------------------------------------------------------------------------
shift
if "%~0" == "--version"  set version=%~1
if "%~0" == "--revision" set revision=%~1
if "%~0" == "--machine"  set machine=%~1
if "%~0" == "--build"    set build=%~1
if "%~0" == ""           goto docheckmachine
shift
goto doparse

:dohelp
echo.
echo Usage: build.cmd [--version ^<version^>] [--revision ^<revision^>]
echo                  [--machine ^<x86^|amd64^>] [--build ^<debug^|release^>]
echo                  [--help]
echo.
echo Where ^<version^> is a three-part dot-separated version number of OpenSLP.
echo and ^<revision^> is any string value that can be interpreted as a revision.
echo The current defaults are %version% and %revision%. The default --machine tag
echo is %machine%. The default --build type is %build%. Use --help to get this
echo help screen.
echo.
echo NOTE! Please ensure that the WIX_PATH environment variable contains the
echo directory in which the WIX tool set has been installed.
goto doexit

:docheckmachine
rem -------------------------------------------------------------------------
rem Be sure they're using a valid machine type...
if "%machine%" == "x86" goto docheckbuild
if "%machine%" == "amd64" goto docheckbuild
echo *** Error: Unknown machine type %machine%.
goto dohelp

:docheckbuild
if "%build%" == "debug" goto dobuild
if "%build%" == "release" goto dobuild
echo *** Error: Unknown build type %build%.
goto dohelp

:dobuild
rem -------------------------------------------------------------------------
set version_revision=%version%
if NOT "%revision%" == "" set version_revision=%version_revision%_%revision%

set machine_build=%machine%
if NOT "%build%" == "release" set machine_build=%machine_build%_%build%

set output=%package%_%version_revision%_%machine_build%.msi

echo.
echo Building %output%...

%WIX_PATH%\candle -nologo -dVERSION=%version% -dREVISION=%revision% %package%.wxs
%WIX_PATH%\light -nologo -out %output% %package%.wixobj -ext WixUIExtension -cultures:en-us -sval

:doexit
endlocal
