@echo off
%~d0
cd %~p0
if not exist output mkdir output
if not exist temp mkdir temp

rem ======BUILD===================================================================
call "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\bin\vcvars32.bat" > NUL

rem ======VERSIONING =============================================================
choice /m "Increment version"
if errorlevel 2 goto build

rem == Backup whatsnew.txt and version.h in case build fails
copy ..\version.h temp\oldversion.h > NUL
copy ..\whatsnew.txt temp\oldwhatsnew.txt > NUL

rem == Update version.h and create version "sidebar" for SWS.com
\bin\incversion ..\version.h
type www\extra_1.dat > output\extra.htm
\bin\PrintVersion ..\version.h >> output\extra.htm
type www\extra_2.dat >> output\extra.htm
\bin\date +"%%B %%-d, %%Y" >> output\extra.htm
type www\extra_3.dat >> output\extra.htm

rem == Update whatsnew.txt 
choice /m "Update whatsnew.txt with new version"
if errorlevel 2 goto build
type www\exclaim.dat > output\whatsnew.txt
\bin\PrintVersion c:\dev\sws-extension\version.h -d >> output\whatsnew.txt
type ..\whatsnew.txt >> output\whatsnew.txt
copy /y output\whatsnew.txt .. > NUL

:build
echo.
vcbuild /r /platform:x64 ..\sws_extension.vcproj release
if errorlevel 1 goto error
vcbuild /r /platform:Win32 ..\sws_extension.vcproj release
if errorlevel 1 goto error


rem ======CREATE WEBSITE WHATSNEW ================================================
\bin\MakeWhatsNew ..\whatsnew.txt temp\new2.dat
type www\new1.dat > output\whatsnew.html
type temp\new2.dat >> output\whatsnew.html

rem ======UPLOAD==================================================================
echo.
if not exist upload.bat goto success
choice /m "Upload to server"
if errorlevel 2 goto success
call upload.bat
goto success

:error
echo.
echo *****************
echo   Build error!!!
echo *****************
echo.
copy /y temp\oldversion.h ..\version.h > NUL
copy /y temp\oldwhatsnew.txt ..\whatsnew.txt > NUL
goto pause

:success
echo.
echo ****************
echo New version:
\bin\PrintVersion c:\dev\sws-extension\version.h
echo ****************
echo.

:pause
del /q temp\*.*
rmdir temp
pause