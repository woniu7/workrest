@echo off
setlocal enabledelayedexpansion

rem ============================================================================
rem  MSVC (cl.exe) build for the windows platform.
rem
rem  Mirrors src/windows/Makefile: same source-selection rules, same libraries,
rem  same console-subsystem / int main() entry -- only the toolchain differs.
rem
rem  Run from a "x64 Native Tools Command Prompt for VS" (so cl/link are on
rem  PATH), inside this directory (src\windows):
rem
rem      build.bat                 :: gui view (default)
rem      build.bat cli             :: cli view
rem      build.bat sdl3            :: sdl3 view  (needs SDL3_DIR, see below)
rem      build.bat clean           :: remove build\msvc\
rem
rem  Options via environment variables:
rem      set STATIC=1              :: static CRT (/MT), standalone exe
rem      set SDL3_DIR=C:\SDL3      :: root with include\SDL3\SDL.h and lib\SDL3.lib
rem ============================================================================

set VIEW=%1
if "%VIEW%"=="" set VIEW=gui

rem --- clean ----------------------------------------------------------------
if /I "%VIEW%"=="clean" (
    if exist "..\..\build\msvc" rmdir /s /q "..\..\build\msvc"
    echo cleaned build\msvc
    exit /b 0
)

set COMMON=..
set OUT=..\..\build\msvc\%VIEW%

rem --- validate view --------------------------------------------------------
if /I not "%VIEW%"=="gui" if /I not "%VIEW%"=="cli" if /I not "%VIEW%"=="sdl3" (
    echo VIEW must be one of: gui cli sdl3
    exit /b 1
)

rem --- source selection (mirrors VIEW_EXCLUDE in ../common.mk) ---------------
rem  Always: app.c rest.c (common) + platform.c keyboard.c (windows).
rem  Plus exactly one view:  gui -> gui.c | cli -> ..\cli.c | sdl3 -> ..\sdl3.c
set SRC=%COMMON%\app.c %COMMON%\rest.c platform.c keyboard.c
if /I "%VIEW%"=="gui"  set SRC=%SRC% gui.c
if /I "%VIEW%"=="cli"  set SRC=%SRC% %COMMON%\cli.c
if /I "%VIEW%"=="sdl3" set SRC=%SRC% %COMMON%\sdl3.c

rem --- flags ----------------------------------------------------------------
rem  /W3 warnings, /O2 like the Makefile's -O2, /Zi for a .pdb.
rem  UNICODE is deliberately NOT defined: TCHAR stays char and DrawText/wsprintf
rem  resolve to their ...A variants, identical to the mingw (ANSI) build. Do not
rem  add /D UNICODE unless you also switch gui.c to the wide-string APIs.
set CRT=/MD
if "%STATIC%"=="1" set CRT=/MT
set CFLAGS=/nologo /W3 /O2 !CRT! /Zi /D_CRT_SECURE_NO_WARNINGS

rem  Same libs the mingw Makefile passes as -lgdi32 -luser32 -lwtsapi32.
set LIBS=user32.lib gdi32.lib wtsapi32.lib

if /I "%VIEW%"=="sdl3" (
    if "%SDL3_DIR%"=="" (
        echo sdl3 view needs SDL3_DIR set ^(root with include\ and lib\^).
        exit /b 1
    )
    set CFLAGS=!CFLAGS! /I"%SDL3_DIR%\include"
    set LIBS=!LIBS! /LIBPATH:"%SDL3_DIR%\lib" SDL3.lib
)

if not exist "%OUT%" mkdir "%OUT%"

rem --- compile + link -------------------------------------------------------
rem  Each .c resolves its "..." includes relative to its own dir, so sources
rem  spanning src\ and src\windows\ need no /I for the project headers.
rem  Objects + pdb go into the output dir (unique basenames, no collision).
echo Building %VIEW% -^> %OUT%\rest.exe
cl !CFLAGS! %SRC% /Fe"%OUT%\rest.exe" /Fo"%OUT%\\" /Fd"%OUT%\rest.pdb" /link !LIBS!
if errorlevel 1 (
    echo BUILD FAILED
    exit /b 1
)

if /I "%VIEW%"=="sdl3" (
    if exist "%SDL3_DIR%\lib\SDL3.dll" copy /y "%SDL3_DIR%\lib\SDL3.dll" "%OUT%\SDL3.dll" >nul
    echo Note: SDL3.dll copied next to rest.exe ^(required at runtime^).
)

echo OK: %OUT%\rest.exe
endlocal
