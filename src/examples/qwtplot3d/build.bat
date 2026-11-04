@echo off
rem Builds lib\qwtplot3d.lib and demo\surface_demo.exe (a GUI program, with no
rem console window). Run from an "x64 Native Tools Command Prompt" for Visual Studio.
rem Uses this repository's GraphApp; set GRAPHAPP_LIB to link another graphapp.lib.
setlocal
cd /d %~dp0
set GRAPHAPP=..\..\..
if "%GRAPHAPP_LIB%"=="" set GRAPHAPP_LIB=%GRAPHAPP%\Windows Binaries\graphapp.lib
where cl >nul 2>nul
if errorlevel 1 (
  echo cl not found: run this from an x64 Native Tools Command Prompt.
  exit /b 1
)
if not exist build mkdir build
if not exist lib mkdir lib

set CFLAGS=/nologo /MT /O2 /W3 /EHsc /DNDEBUG /D_CRT_SECURE_NO_WARNINGS /Iinclude
cl %CFLAGS% /c /Fobuild\ src\qwt3d_*.cpp /I"%GRAPHAPP%\src" /MP
if errorlevel 1 exit /b 1
cl %CFLAGS% /c /Fobuild\ 3rdparty\gl2ps\gl2ps.c
if errorlevel 1 exit /b 1
lib /nologo /OUT:lib\qwtplot3d.lib build\*.obj
if errorlevel 1 exit /b 1

cl %CFLAGS% /I"%GRAPHAPP%\src" /Fobuild\demo.obj /Fedemo\surface_demo.exe demo\surface_demo.cpp ^
  lib\qwtplot3d.lib "%GRAPHAPP_LIB%" opengl32.lib glu32.lib user32.lib gdi32.lib ^
  comdlg32.lib shell32.lib advapi32.lib winmm.lib ole32.lib ^
  /link /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup
del build\demo.obj 2>nul
