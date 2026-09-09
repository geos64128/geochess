@echo off
rem Build and run the host-side test suite.
rem
rem These compile with an ordinary host compiler against
rem src\geochess-ai.h. cc65 is not needed and nothing here touches GEOS.

setlocal enabledelayedexpansion

cd /d "%~dp0"

if "%CC%"=="" set CC=gcc
if "%PYTHON%"=="" set PYTHON=python

if not exist build md build

rem savetest is generated from the sources so it cannot drift from them
"%PYTHON%" mksavetest.py build\savetest.c
if errorlevel 1 goto :fail

set FAILS=0

for %%t in (perft rules clickpath search selfplay) do (
    "%CC%" -O2 -I..\src -o build\%%t.exe %%t.c
    if errorlevel 1 goto :fail
)

"%CC%" -O2 -I..\src -o build\savetest.exe build\savetest.c
if errorlevel 1 goto :fail

for %%t in (perft rules clickpath search selfplay savetest) do (
    echo ===== %%t =====
    build\%%t.exe
    if errorlevel 1 (
        echo ----- %%t FAILED
        set /a FAILS=!FAILS!+1
    ) else (
        echo ----- %%t PASSED
    )
    echo.
)

if !FAILS! equ 0 (
    echo all suites passed
    exit /b 0
)

echo !FAILS! suite^(s^) failed
exit /b 1

:fail
echo BUILD FAILED
exit /b 1
