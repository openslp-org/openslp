@echo off
cd libslp
call m.bat %1 %2 %3 %4 %5 %6 %7 %8 %9
cd ..\slpd
call m.bat %1 %2 %3 %4 %5 %6 %7 %8 %9
cd ..\slptool
call m.bat %1 %2 %3 %4 %5 %6 %7 %8 %9
cd ..

