@echo off
cd /d "%~dp0"
if not exist backend\musicms.exe call build.bat
start "" http://localhost:8080
backend\musicms.exe -w frontend -d data
pause
