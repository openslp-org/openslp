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
if "%1"=="Debug" set SolutionConfig=%1
"%DevEnvDir%\devenv.exe" openslp.sln /build %SolutionConfig%

:done
endlocal