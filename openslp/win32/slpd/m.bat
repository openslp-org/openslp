@echo off
setlocal
if "%DevEnvDir%"=="" goto domsvc6

:domsvc7
set openslp_opt_type=%1
if "%openslp_opt_type%"=="" set openslp_opt_type=build
devenv ..\openslp.sln /%openslp_opt_type% Debug /project slpd
devenv ..\openslp.sln /%openslp_opt_type% Release /project slpd
goto done

:domsvc6
if not exist slpd.dep set NO_EXTERNAL_DEPS=1
nmake /nologo /s /f slpd.mak CFG="slpd - Win32 Debug"  %1 %2 %3 %4 %5 %6 %7 %8 %9
nmake /nologo /s /f slpd.mak CFG="slpd - Win32 Release"  %1 %2 %3 %4 %5 %6 %7 %8 %9
set NO_EXTERNAL_DEPS=

:done
endlocal
