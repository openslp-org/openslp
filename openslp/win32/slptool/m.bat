@echo off
if not exist slptool.dep set NO_EXTERNAL_DEPS=1
nmake /nologo /s /f slptool.mak CFG="slptool - Win32 Debug"  %1 %2 %3 %4 %5 %6 %7 %8 %9
nmake /nologo /s /f slptool.mak CFG="slptool - Win32 Release"  %1 %2 %3 %4 %5 %6 %7 %8 %9
set NO_EXTERNAL_DEPS=