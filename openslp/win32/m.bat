@echo off
setlocal
if not "%DevEnvDir%"=="" goto vc7x

:vc6x
cd libslp
call m.bat %1 %2 %3 %4 %5 %6 %7 %8 %9
cd ..\slpd
call m.bat %1 %2 %3 %4 %5 %6 %7 %8 %9
cd ..\slptool
call m.bat %1 %2 %3 %4 %5 %6 %7 %8 %9
cd ..
goto done

:vc7x
set SolutionConfig=Release
set Target=build

:nextparam
shift
if "%0"=="debug" set SolutionConfig=%0
if "%0"=="clean" set Target=clean
if "%0"=="" goto dobuild
goto nextparam

:dobuild
"%DevEnvDir%\devenv.exe" openslp.sln /%Target% %SolutionConfig%

:done
endlocal