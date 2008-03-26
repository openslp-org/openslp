@echo off

rem -------------------------------------------------------------------------
rem Usage: makesrc [--src <srcdir>] [--tgt <tgtdir>] [--debug] [--help]
rem 
rem makesrc.cmd creates either a debug or release code installation directory 
rem structure from command line input.
rem -------------------------------------------------------------------------

setlocal

set source=..\openslp
set target=.
set build=release

:doparse
rem -------------------------------------------------------------------------
shift
if "%0%" == "--src"     goto dosource
if "%0%" == "--tgt"     goto dotarget
if "%0%" == "--debug"   goto dodebug
if "%0%" == "--help"    goto dohelp
if "%0%" == ""          goto dobuild
goto dohelp

:dosource
shift
set source=%0%
goto doparse

:dotarget
shift
set target=%0%
goto doparse

:dodebug
set build=debug
goto doparse

:dohelp
echo.
echo Usage: makesrc [--src <srcdir>] [--tgt <tgtdir>] [--debug] [--help]
echo.
echo Where <srcdir> is the location of the root of the openslp source directory, 
echo and <tgtdir> is the location to build the install directory structure.
echo The --debug option indicates that debug code should be packaged in the 
echo resulting .msi file, rather than the default release code. Use --help to 
echo get this help screen.
echo.
echo Default (release): "makesrc --src %source% --tgt %target%"
echo.
echo NOTE! Please ensure that the WIX_PATH environment variable contains the
echo directory in which the WIX tool set has been installed.
goto doexit

:dobuild
rem -------------------------------------------------------------------------
rem Check to be sure this is what they really want to do...
echo Copying product [%build%] files from %source% into %target%\OpenSLP.

rem Remove old source directory (if exists), create new structure
rmdir /s /q %target%\OpenSLP

rem Create target directory structure
mkdir %target%\OpenSLP
mkdir %target%\OpenSLP\Docs
mkdir %target%\OpenSLP\Docs\html
mkdir %target%\OpenSLP\Include
mkdir %target%\OpenSLP\Lib

rem Copy files from %source% to %target%[%build%]
xcopy %source%\doc\rfc\* %target%\OpenSLP\Docs\rfc /S /I
xcopy %source%\doc\html\* %target%\OpenSLP\Docs\html /S /I
xcopy %source%\libslp\slp.h %target%\OpenSLP\Include
xcopy %source%\win32\%build%\slp.lib %target%\OpenSLP\Lib
xcopy %source%\win32\%build%\slpstatic.lib %target%\OpenSLP\Lib
xcopy %source%\win32\%build%\slp.dll %target%\OpenSLP
xcopy %source%\win32\%build%\slp.pdb %target%\OpenSLP
xcopy %source%\win32\%build%\slpd.exe %target%\OpenSLP
xcopy %source%\win32\%build%\slpd.pdb %target%\OpenSLP
xcopy %source%\win32\%build%\slptool.exe %target%\OpenSLP
xcopy %source%\win32\%build%\slptool.pdb %target%\OpenSLP

:doexit
endlocal

