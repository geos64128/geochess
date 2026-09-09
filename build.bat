@echo off
rem A d64 left open elsewhere can block the delete; the format
rem below rewrites the image either way.
if exist target rd /S /Q target 2>nul
if not exist target md target

cd src

cl65 -t geos-cbm -O -o ..\target\geochess.cvt geochess-res.grc geochess.c
if errorlevel 1 goto :fail

if exist *.o del /Q *.o

cd ..\target

"%~dp0c1541.exe" -format "geochess,sh" d64 geochess.d64 ^
 -write geochess.cvt geochess.cvt ^
 -write ..\src\GEOCHESSFONT40.cvt geochessf40.cvt ^
 -write ..\src\GEOCHESSFONT80.cvt geochessf80.cvt
if errorlevel 1 goto :fail

del /Q geochess.cvt

cd ..
exit /b 0

:fail
cd /d "%~dp0"
echo BUILD FAILED
exit /b 1
