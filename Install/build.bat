@echo off

REM Copyright 2014 Jeffos. All rights reserved.
REM When uploading, the website is automatically updated (a PHP reads the uploaded version.h).
REM The version checker reads the uploaded version.h too.

REM You must set these according to your local setup
REM FTP access is reserved for project owners
set ftp_user=TO_BE_DEFINED
set ftp_pwd=TO_BE_DEFINED
set vcvars32_path="C:\Program Files\Microsoft Visual Studio 9.0\VC\bin\vcvars32.bat"
set makensis_path="C:\Program Files\NSIS\makensis.exe"


REM ====== INIT ===============================================================
if not exist output mkdir output
if not exist temp mkdir temp

REM == Backup whatsnew.txt and version.h in case the script fails
copy ..\version.h temp\oldversion.h > NUL
copy ..\whatsnew.txt temp\oldwhatsnew.txt > NUL


REM ====== VERSIONING =========================================================
set choice_inc=
set /p choice_inc=Increment version (y/n)? 
if %choice_inc%==y goto inc
goto whatsnew 

:inc
echo Incrementing version...
..\BuildUtils\Release\IncVersion.exe ..\version.h
..\BuildUtils\Release\PrintVersion ..\version.h -d "!v%%d.%%d.%%d #%%d" > ..\whatsnew.txt
type temp\oldwhatsnew.txt >> ..\whatsnew.txt


REM ====== WHATSNEW ===========================================================
:whatsnew
echo Generating output\whatsnew.htm...
..\BuildUtils\Release\MakeWhatsNew.exe -h ..\whatsnew.txt > output\whatsnew.htm

REM ====== LANGPACK ===========================================================
:langpack
echo Generating output\sws_template.ReaperLangPack...
cd ..\..
call sws\GenLangPack\sws_build_template_langpack.sh
if not errorlevel 0 goto error
cd sws\Install

REM ======BUILD================================================================
REM Also generates Python function wrapper files!
echo.
set choice=
set /p choice=Build (y/n)? 
if %choice%==y goto build 
goto installers_choice 

:build
if not exist %vcvars32_path% (
	echo Error: you must configure the env var 'vcvars32_path' in %0.bat!
	goto error
)

setlocal
call %vcvars32_path% > NUL

vcbuild /r /platform:x64 ..\sws_extension.vcproj release
if errorlevel 1 goto error

vcbuild /r /platform:Win32 ..\sws_extension.vcproj release
if errorlevel 1 goto error

endlocal

REM ======INSTALLERS===========================================================
:installers_choice
set choice=
set /p choice=Create Win/NSIS installers (y/n)? 
if %choice%==y goto installers 
goto upload_choice

:installers
if not exist %makensis_path% (
	echo Error: you must configure the env var 'makensis_path' in %0.bat!
	goto error
)

echo Generating output\sws_extension.exe...
%makensis_path% /v2 /X"SetCompressor /FINAL lzma" sws_extension.nsi
if errorlevel 1 goto error

echo Generating output\sws_extension_x64.exe...
%makensis_path% /v2 /X"SetCompressor /FINAL lzma" sws_extension_x64.nsi
if errorlevel 1 goto error

REM ======UPLOAD===============================================================
:upload_choice
set choice=
set /p choice=Upload (f[eatured] / p[re-release] / n)? 
if %choice%==f (
	set choice=featured
	goto upload
)
if %choice%==p (
	set choice=pre-release
	goto upload
)
if %choice%==n goto success

echo Invalid input '%choice%': 'f', 'p', or 'n' expected!
goto error

:upload
if %ftp_user%==TO_BE_DEFINED (
	echo Error: you must configure FTP env vars in %0.bat!
	goto error
)


REM Two FTP steps are required because the "ftp" command
REM does not release downloaded files while running

REM 1st step ==========================
REM get the current version as there might be a gap between local and remote versions 
REM e.g. uploaded a featured version then a pre-release one
echo user %ftp_user% > temp\get_version.ftp
echo %ftp_pwd%>> temp\get_version.ftp
echo cd download/%choice% >> temp\get_version.ftp
echo ascii >> temp\get_version.ftp
echo get version.h temp\online_version.h >> temp\get_version.ftp
echo quit >> temp\get_version.ftp

echo FTP: getting current version...
ftp -v -n -i -s:temp\get_version.ftp ftp.online.net

REM 2nd step ==========================
REM backup remote files and upload new ones
echo user %ftp_user% > temp\upload.ftp
echo %ftp_pwd%>> temp\upload.ftp
echo cd download/%choice% >> temp\upload.ftp
echo binary >> temp\upload.ftp

REM Backup current remote files into download/%choice%/old (except whatsnew.htm: common to all versions)
if exist temp\online_version.h (
	..\BuildUtils\Release\PrintVersion temp\online_version.h "rename sws-v%%d.%%d.%%d.%%d-install.exe tmp.exe" >> temp\upload.ftp
	..\BuildUtils\Release\PrintVersion temp\online_version.h "rename tmp.exe old/sws-v%%d.%%d.%%d.%%d-install.exe" >> temp\upload.ftp
	..\BuildUtils\Release\PrintVersion temp\online_version.h "rename sws-v%%d.%%d.%%d.%%d-x64-install.exe tmp-x64.exe" >> temp\upload.ftp
	..\BuildUtils\Release\PrintVersion temp\online_version.h "rename tmp-x64.exe old/sws-v%%d.%%d.%%d.%%d-x64-install.exe" >> temp\upload.ftp
	..\BuildUtils\Release\PrintVersion temp\online_version.h "rename sws-v%%d.%%d.%%d.%%d.dmg tmp.dmg" >> temp\upload.ftp
	..\BuildUtils\Release\PrintVersion temp\online_version.h "rename tmp.dmg old/sws-v%%d.%%d.%%d.%%d.dmg" >> temp\upload.ftp
	..\BuildUtils\Release\PrintVersion temp\online_version.h "rename sws-v%%d.%%d.%%d.%%d-template.ReaperLangPack tmp.ReaperLangPack" >> temp\upload.ftp
	..\BuildUtils\Release\PrintVersion temp\online_version.h "rename tmp.ReaperLangPack old/sws-v%%d.%%d.%%d.%%d-template.ReaperLangPack" >> temp\upload.ftp
)
echo mdelete *.* >> temp\upload.ftp

REM Upload new files
echo put output\whatsnew.htm index.htm >> temp\upload.ftp
..\BuildUtils\Release\PrintVersion ..\version.h "put output\sws_extension.exe sws-v%%d.%%d.%%d.%%d-install.exe" >> temp\upload.ftp
..\BuildUtils\Release\PrintVersion ..\version.h "put output\sws_extension_x64.exe sws-v%%d.%%d.%%d.%%d-x64-install.exe" >> temp\upload.ftp
..\BuildUtils\Release\PrintVersion ..\version.h "put output\sws_osx.dmg sws-v%%d.%%d.%%d.%%d.dmg" >> temp\upload.ftp
..\BuildUtils\Release\PrintVersion ..\version.h "put output\sws_template.ReaperLangPack sws-v%%d.%%d.%%d.%%d-template.ReaperLangPack" >> temp\upload.ftp

REM Upload for PHP website auto-update
echo ascii >> temp\upload.ftp
echo put ..\version.h >> temp\upload.ftp
echo quit >> temp\upload.ftp

echo FTP: uploading...
ftp -v -n -i -s:temp\upload.ftp ftp.online.net
echo.
echo Can't test if ftp has succeeded (always return 0, unfortunately :/)
echo Please double-check the above log, and terminate batch job if needed (ctrl-c)
echo.
pause
goto success

:error
echo.
echo *****************
echo   Error!!!
echo *****************
echo.
copy /y temp\oldversion.h ..\version.h > NUL
copy /y temp\oldwhatsnew.txt ..\whatsnew.txt > NUL
goto pause

:success
echo.
echo ****************
echo Success!
..\BuildUtils\Release\PrintVersion ..\version.h -d
echo ****************
echo.

:pause
del /q temp\*.*
rmdir temp
pause
echo.
