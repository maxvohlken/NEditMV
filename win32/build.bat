@echo off
setlocal
set EXCEED=..\..\exceed-7.1
rem set EXCEED=C:\APPS\Hummingbird\Connectivity\7.10\Exceed
set XMLINK=Static
copy config.h ..
copy MVVersion.h ..\source
cd ..\util & nmake -nologo -f util.mak -u EXCEED=%EXCEED% XMLINK=%XMLINK% %*
if errorlevel 1 goto done
cd ..\source
nmake -nologo -f nedit.mak -u EXCEED=%EXCEED% XMLINK=%XMLINK% %*
if errorlevel 1 goto done
nmake -nologo -f nc.mak -u EXCEED=%EXCEED% XMLINK=%XMLINK% %*
if errorlevel 1 goto done
:done
endlocal
