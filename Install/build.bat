@echo off

REM Copyright 2013 and later Jeffos. All rights reserved.
REM See https://github.com/Jeff0S/sws-web

REM ===========================================================================
REM You must set these according to your local setup
REM FTP access is reserved for project owners
REM ===========================================================================
set ftp_host=TO_BE_DEFINED
set ftp_user=TO_BE_DEFINED
set ftp_pwd=TO_BE_DEFINED
set vcvars_path="C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"
set makensis_path="D:\Program Files (x86)\NSIS\makensis.exe"
set osx_dmg_path="D:\dev\sws_osx.dmg"


REM ====== INIT ===============================================================
if not exist output mkdir output
if not exist temp mkdir temp
del /q temp\*.*
if not exist ..\version.h (
	echo Error: version.h not found!
	goto error
)
if not exist ..\whatsnew.txt (
	echo Error: whatsnew.txt not found!
	goto error
)

set build_type=
set /p build_type=Build type? (f[eatured] / p[re-release]) 
if %build_type%==f (
	set build_type=featured
	goto whatsnew
)
if %build_type%==p (
	set build_type=pre-release
	goto whatsnew
)
echo Invalid input '%build_type%': 'f', 'p', or 'n' expected!
goto error


REM ====== WHATSNEW ===========================================================
:whatsnew
echo Generating output\whatsnew.htm...
if %build_type%==pre-release (
	..\BuildUtils\Release\MakeWhatsNew.exe -h ..\whatsnew.txt > output\whatsnew.html
)
if %build_type%==featured (
	..\BuildUtils\Release\MakeWhatsNew.exe ..\whatsnew.txt > output\whatsnew.html
)

REM ====== LANGPACK ===========================================================
:langpack
echo Generating output\sws_template.ReaperLangPack...
cd ..\..
call sws\GenLangPack\sws_build_template_langpack.sh
if not errorlevel 0 goto error
cd sws\Install

REM ======BUILD================================================================
REM Note: building will also generate Python function wrapper files
echo.
set choice=
set /p choice=Build (y/n)? 
if %choice%==y goto build 
goto installers_choice 

:build
if not exist %vcvars_path% (
	echo Error: env var 'vcvars_path' not defined in in %0.bat!
	goto error
)
if not exist "%REAPER_DIR%\reaper_plugin_functions.h" (
	echo Error: "%REAPER_DIR%\reaper_plugin_functions.h" not found!
	goto error
)

setlocal
call %vcvars_path% > NUL

REM For VCBuild / Visual Studio 2008 you will need:
REM vcbuild /r /platform:x64 ..\sws_extension.vcproj release
REM if errorlevel 1 goto error
REM vcbuild /r /platform:Win32 ..\sws_extension.vcproj release
REM if errorlevel 1 goto error

REM For MSBuild / Visual Studio 2013
msbuild /t:rebuild /p:Platform=x64,Configuration=release ..\sws_extension.vcxproj
if errorlevel 1 goto error

pause

msbuild /t:rebuild /p:Platform=Win32,Configuration=release ..\sws_extension.vcxproj
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
set /p choice=Upload (y/n)? 
if %choice%==y goto upload
goto success

:upload
if %ftp_host%==TO_BE_DEFINED (
	echo Error: you must configure FTP env vars in %0.bat!
	goto error
)
if not exist %osx_dmg_path% (
	echo Error: %osx_dmg_path% not found!
	goto error
)

REM Two FTP steps are required because the "ftp" command
REM does not release downloaded files while running

REM 1st FTP step: get the current online version

echo user %ftp_user% > temp\get_version.ftp
echo %ftp_pwd%>> temp\get_version.ftp
echo cd www/download/%build_type% >> temp\get_version.ftp
echo ascii >> temp\get_version.ftp
echo get version.h temp\online_version.h >> temp\get_version.ftp
echo quit >> temp\get_version.ftp
echo FTP: getting current version...
ftp -v -n -i -s:temp\get_version.ftp %ftp_host%


REM 2nd FTP step: backup remote files and upload new ones

echo user %ftp_user% > temp\upload.ftp
echo %ftp_pwd%>> temp\upload.ftp
echo cd www/download/%build_type% >> temp\upload.ftp
echo binary >> temp\upload.ftp

REM Backup current remote files into download/old
if not exist temp\online_version.h (
	echo Error: temp\online_version.h not found!
	goto error
)
..\BuildUtils\Release\PrintVersion temp\online_version.h "rename sws-v%%d.%%d.%%d.%%d-install.exe tmp.exe" >> temp\upload.ftp
..\BuildUtils\Release\PrintVersion temp\online_version.h "rename tmp.exe ../old/sws-v%%d.%%d.%%d.%%d-install.exe" >> temp\upload.ftp
..\BuildUtils\Release\PrintVersion temp\online_version.h "rename sws-v%%d.%%d.%%d.%%d-x64-install.exe tmp-x64.exe" >> temp\upload.ftp
..\BuildUtils\Release\PrintVersion temp\online_version.h "rename tmp-x64.exe ../old/sws-v%%d.%%d.%%d.%%d-x64-install.exe" >> temp\upload.ftp
..\BuildUtils\Release\PrintVersion temp\online_version.h "rename sws-v%%d.%%d.%%d.%%d.dmg tmp.dmg" >> temp\upload.ftp
..\BuildUtils\Release\PrintVersion temp\online_version.h "rename tmp.dmg ../old/sws-v%%d.%%d.%%d.%%d.dmg" >> temp\upload.ftp
..\BuildUtils\Release\PrintVersion temp\online_version.h "rename sws-v%%d.%%d.%%d.%%d-template.ReaperLangPack tmp.ReaperLangPack" >> temp\upload.ftp
..\BuildUtils\Release\PrintVersion temp\online_version.h "rename tmp.ReaperLangPack ../old/sws-v%%d.%%d.%%d.%%d-template.ReaperLangPack" >> temp\upload.ftp
echo mdelete *.exe >> temp\upload.ftp
echo mdelete *.dmg >> temp\upload.ftp
echo mdelete *.ReaperLangPack >> temp\upload.ftp
echo mdelete *.h >> temp\upload.ftp
echo mdelete *.html >> temp\upload.ftp
REM ... but keep .htm files, e.g. index.htm in the "featured" folder

REM Upload new files
echo put output\whatsnew.html >> temp\upload.ftp
..\BuildUtils\Release\PrintVersion ..\version.h "put output\sws_extension.exe sws-v%%d.%%d.%%d.%%d-install.exe" >> temp\upload.ftp
..\BuildUtils\Release\PrintVersion ..\version.h "put output\sws_extension_x64.exe sws-v%%d.%%d.%%d.%%d-x64-install.exe" >> temp\upload.ftp
..\BuildUtils\Release\PrintVersion ..\version.h "put %osx_dmg_path% sws-v%%d.%%d.%%d.%%d.dmg" >> temp\upload.ftp
..\BuildUtils\Release\PrintVersion ..\version.h "put output\sws_template.ReaperLangPack sws-v%%d.%%d.%%d.%%d-template.ReaperLangPack" >> temp\upload.ftp
echo ascii >> temp\upload.ftp
echo put ..\version.h >> temp\upload.ftp
echo quit >> temp\upload.ftp

echo FTP: uploading...
ftp -v -n -i -s:temp\upload.ftp %ftp_host%
echo.
pause
goto success

:error
echo.
echo ************
echo   Error!!!
echo ************
goto theend

:success
echo.
echo ****************************************
echo Success!
..\BuildUtils\Release\PrintVersion ..\version.h -d
echo ****************************************
del /q temp\*.*
rmdir temp

:theend
echo.
