::
:: file:			findfl.bat
:: created on:		2021 Mar 20
:: created by:		
:: purpose:			...
::

@echo off
setlocal EnableDelayedExpansion


set currentDirectory=%cd%

 rem todo: make proper evaluation of search pattern construction

set "patternToSearch=%2"

for /f "tokens=*" %%a in ('echo %2^| findstr /i "\."') do ( set hasPoint=%%a )

if not "%hasPoint%" == "" (
	set "patternToSearch=\%2"
)

rem echo ++++ patternToSearch = !patternToSearch!
rem exit /b 0

cd /D %1
dir /s /b | findstr /i "!patternToSearch!"
cd /D %currentDirectory%

exit /b 0

endlocal
