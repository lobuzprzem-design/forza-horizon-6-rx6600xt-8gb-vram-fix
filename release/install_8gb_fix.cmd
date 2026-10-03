@echo off
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -File "%~dp0install_8gb_fix.ps1"
set "fixExit=%ERRORLEVEL%"
if not "%fixExit%"=="0" echo Installation stopped. Read the message above; no protection setting was changed.
pause
exit /b %fixExit%
