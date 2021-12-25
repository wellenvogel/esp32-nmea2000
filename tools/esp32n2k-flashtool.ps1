@echo off
Rem Make powershell read this file, skip a number of lines, and execute it.
Rem This works around .ps1 bad file association as non executables.
PowerShell -Command "Get-Content '%~dpnx0' | Select-Object -Skip 6 | Out-String | Invoke-Expression"
pause
goto :eof
# Start of PowerShell script here
Write-Host "Hello World!"
