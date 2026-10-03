@echo off
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -File "%~dp0uninstall_8gb_fix.ps1"
set "fixExit=%ERRORLEVEL%"
if not "%fixExit%"=="0" echo Uninstall stopped. Preserve backup/state files and read the message above.
pause
exit /b %fixExit%
