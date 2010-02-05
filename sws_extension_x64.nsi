; sws_extension.nsi
;
; Dump reaper_sws64.dll into the proper plugins dir and delete old vers

;--------------------------------

Name "SWS Extension x64"
OutFile "x64\Release\sws_extension_x64.exe"
InstallDir "$PROGRAMFILES\Reaper (x64)\Plugins"
RequestExecutionLevel highest
LicenseData license.txt

Function .onInit
  ReadRegStr $0 HKLM Software\Reaper ""
  StrCmp $0 "" +1
  StrCpy $INSTDIR $0\Plugins
FunctionEnd

Page license
Page directory
Page instfiles

Section ""
  SetOutPath $INSTDIR
  CheckReaper:
  FindProcDLL::FindProc "reaper.exe"
  StrCmp $R0 "1" "" ReaperClosed
  MessageBox MB_OKCANCEL "Please close all instances of REAPER before continuing installation." IDOK CheckReaper IDCANCEL Abort  
  ReaperClosed:
  File x64\Release\reaper_sws64.dll
  Abort:
SectionEnd
