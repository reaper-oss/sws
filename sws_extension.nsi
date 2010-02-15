; sws_extension.nsi
;
; Dump reaper_sws.dll into the proper plugins dir and delete old vers

;--------------------------------

Name "SWS Extension"
OutFile "Release\sws_extension.exe"
InstallDir $PROGRAMFILES\Reaper\Plugins
RequestExecutionLevel highest
LicenseData license.txt

Function .onInit
  SetRegView 32
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
  Delete "$INSTDIR\reaper_console.dll"
  Delete "$INSTDIR\reaper_freeze.dll"
  Delete "$INSTDIR\reaper_markeractions.dll"
  Delete "$INSTDIR\reaper_markerlist.dll"
  Delete "$INSTDIR\reaper_snapshot.dll"
  Delete "$INSTDIR\reaper_xenakios_commands.dll"
  File Release\reaper_sws.dll
  Abort:
SectionEnd
