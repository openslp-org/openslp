@echo off

REM -------------------------------------------------------------------------
REM Usage: makesrc [<srcdir>] [<targetdir>] [{debug}|release]
REM 
REM makesrc.cmd creates either a debug or release code installation directory 
REM structure from command line input.
REM -------------------------------------------------------------------------

setlocal

REM check for all reasonable ways of asking for help
if "%1"=="/help"  goto usage
if "%1"=="-help"  goto usage
if "%1"=="--help" goto usage
if "%1"=="/h"     goto usage
if "%1"=="-h"     goto usage
if "%1"=="/?"     goto usage
if "%1"=="-?"     goto usage
if "%1"=="?"      goto usage

REM gather parameters (if any)
set source=..\openslp
set target=.
set build=debug

if "%1"=="" goto parm_done
set source=%1
shift

if "%1"=="" goto parm_done
set target=%1
shift

if "%1"=="" goto parm_done
set build=%1

:parm_done

REM check to be sure this is what they really want to do...
echo Copying various [%build%] files from %source% into %target%\OpenSLP.
echo Press Ctrl-C to quit now, otherwise
pause

REM remove old source directory (if exists), create new structure
rmdir /s /q %target%\OpenSLP

mkdir %target%\OpenSLP
mkdir %target%\OpenSLP\Docs
mkdir %target%\OpenSLP\Docs\html
mkdir %target%\OpenSLP\Include
mkdir %target%\OpenSLP\Lib

REM copy files from %source% to %target%[%build%]
xcopy %source%\doc\rfc\* %target%\OpenSLP\Docs\rfc /S /I
xcopy %source%\doc\html\* %target%\OpenSLP\Docs\html /S /I
xcopy %source%\doxygen\smalllogo.jpg %target%\OpenSLP\Docs\html
xcopy %source%\doxygen\html\* %target%\OpenSLP\Docs\html\SourceCode /S/I

xcopy %source%\libslp\slp.h %target%\OpenSLP\Include

xcopy %source%\win32\%build%\libslp.lib %target%\OpenSLP\Lib

xcopy %source%\win32\%build%\libslp.dll %target%\OpenSLP
xcopy %source%\win32\%build%\libslp.pdb %target%\OpenSLP
xcopy %source%\win32\%build%\slpd.exe %target%\OpenSLP
xcopy %source%\win32\%build%\slpd.pdb %target%\OpenSLP
xcopy %source%\win32\%build%\slptool.exe %target%\OpenSLP
xcopy %source%\win32\%build%\slptool.pdb %target%\OpenSLP

goto done

:usage
echo.
echo Usage: makesrc [<srcdir>] [<targetdir>] [debug|release]
echo.
echo Where <srcdir> is the location of the root of the openslp source directory, 
echo and <targetdir> is the location to build the install directory structure,
echo and debug|release represents the build from which to pull win32 binaries
echo when building the installation image.
echo.
echo The default command line is: "makesrc ..\openslp . debug"
echo.
echo NOTE! Please ensure that the WIX_PATH environment variable contains the
echo directory in which the WIX tool set has been installed.

:done

endlocal

