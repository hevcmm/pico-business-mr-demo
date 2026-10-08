@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0package-demo.ps1" %*
exit /b %ERRORLEVEL%
