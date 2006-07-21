@echo off

REM -------------------------------------------------------------------------
REM Usage: regenfiles
REM 
REM regenfiles.cmd generates a file called openslp_files.wxs, which contains
REM all the necessary XML structure for all files in the OpenSLP directory.
REM -------------------------------------------------------------------------

setlocal

if EXIST openslp_files.wxs.old erase openslp_files.wxs.old

if EXIST openslp_files.wxs rename openslp_files.wxs openslp_files.wxs.old

REM Please ensure that the Wix toolset is in your path...
tallow -nologo -d OpenSLP > openslp_files.wxs

echo.
echo The Wix source file [openslp_files.wxs] has been regenerated. The previous
echo version is saved in [openslp_files.wxs.old]. Add an XML header to the new
echo file, compare the new against the old, copy GUIDs from old to new where the
echo component contents have NOT changed, use NEW GUIDs where they HAVE changed.
echo Ensure that the WIX Component structure is duplicated in the new file so
echo the WIX Features will reference Components properly.

endlocal

