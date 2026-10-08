@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0run-pico-business-mr.ps1" %*
exit /b %ERRORLEVEL%
