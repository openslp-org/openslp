@echo off

REM -------------------------------------------------------------------------
REM Usage: build [version] [machine]
REM 
REM build.cmd rebuilds the openslp installer
REM -------------------------------------------------------------------------

setlocal

if "%WIX_PATH"=="" goto usage

REM Please ensure that your path contains the WIX tool set directory, and 
REM that your environment contains a WIX_PATH variable that points to the
REM root of the WIX tool set directory.

set version=2.0.0
set machine=x86

if "%1"=="" goto parm_done
version=%1
shift

if "%1"=="" goto parm_done
machine=%1
shift

:parm_done

echo.
echo Building openslp_%version%_%machine%.msi...

candle -nologo openslp.wxs openslp_files.wxs
light -nologo -out openslp_%version%_%machine%.msi openslp.wixobj openslp_files.wixobj %WIX_PATH%\wixui.wixlib -loc %WIX_PATH%\WixUI_en-us.wxl

goto done

:usage
echo.
echo Usage: build.cmd [version] [machine]
echo.
echo Where [version] is a three-part dot-separated version number of OpenSLP.
echo and [machine] is a machine architecture tag, eg., x86, x86_64, alpha, etc.
echo.
echo The default configuration is version=2.0.0 and machine=x86.
echo.
echo NOTE! Please ensure that the WIX_PATH environment variable contains the
echo directory in which the WIX tool set has been installed.

:done

endlocal

