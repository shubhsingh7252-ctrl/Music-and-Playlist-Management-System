@echo off
REM Windows build. Needs MinGW-w64 gcc (https://winlibs.com) in PATH.
cd /d "%~dp0backend"
gcc -std=gnu11 -O2 -Wall -D__USE_MINGW_ANSI_STDIO=1 -o musicms.exe src\*.c -lws2_32 -lm
if errorlevel 1 (echo BUILD FAILED & pause & exit /b 1)
echo Build OK. Ab run.bat chalao.
