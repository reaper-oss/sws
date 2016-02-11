; sws_extension.nsi
;
; Dump reaper_sws.dll into the proper plugins dir and delete old vers

;--------------------------------

!include "WinMessages.nsh"

RequestExecutionLevel highest
LicenseData license.txt
Var GROOVE_DIR
Var INI_PATH
DirText "Please select a valid REAPER installation folder:" 

Page license
Page directory
Page components SetPaths
Page instfiles

Section "Extension DLL" extension_id
  SetOutPath $INSTDIR\Plugins
  CheckReaper:
  FindProcDLL::FindProc "reaper.exe"
  StrCmp $R0 "1" "" ReaperClosed
  MessageBox MB_OKCANCEL "Please close all instances of REAPER before continuing installation." IDOK CheckReaper IDCANCEL Abort  
  ReaperClosed:
  Delete "$INSTDIR\Plugins\reaper_console.dll"
  Delete "$INSTDIR\Plugins\reaper_freeze.dll"
  Delete "$INSTDIR\Plugins\reaper_markeractions.dll"
  Delete "$INSTDIR\Plugins\reaper_markerlist.dll"
  Delete "$INSTDIR\Plugins\reaper_snapshot.dll"
  Delete "$INSTDIR\Plugins\reaper_xenakios_commands.dll"
  Delete "$INSTDIR\Plugins\reaper_SnMExtension.dll"
  Delete "$INSTDIR\Plugins\reaper_FNG_Extension.dll"
  Delete "$INSTDIR\Plugins\reaper_fingers.dll"
  Delete "$INSTDIR\Plugins\reaper_dragzoom.dll"
  Delete "$INSTDIR\Plugins\reaper_WT_InTheSky.dll"
  Delete "$INSTDIR\Plugins\reaper_sws_whatsnew.txt"
  File ${OUT_DLL}
  File ${OUT_PY}
  Abort:
SectionEnd

Section "Grooves" groove_id
  SetOutPath $GROOVE_DIR
  File ..\FingersExtras\Grooves\*.rgt
  ; Since we don't allow the user to select the groove dir anymore,
  ; don't set the path in the INI.  The code sets the default instead.
  ;WriteINIStr $INI_PATH\REAPER.ini "fingers" "groove_dir" $GROOVE_DIR
SectionEnd

Function .onInit
  SetRegView ${CPUBITS}
  ReadRegStr $0 HKLM Software\Reaper ""
  StrCmp $0 "" +1
  StrCpy $INSTDIR $0
  SectionSetFlags ${extension_id} 17 ; Read only and selected
FunctionEnd

Function .onVerifyInstDir
  IfFileExists $INSTDIR\reaper.exe PathGood
    Abort ; if $INSTDIR is not a reaper install directory	
  PathGood:
FunctionEnd

Function .onMouseOverSection
  IntCmp $0 ${extension_id} Extension
  IntCmp $0 ${groove_id} Grooves
	Abort
	
  Extension:
    StrCpy $1 "The main extension file, reaper_sws.dll, is required.  It is installed in the $INSTDIR\Plugins directory."
    Goto SetTheText
  Grooves:
    StrCpy $1 "Grooves are used with the groove tool, and are installed in the $GROOVE_DIR directory."
  
  SetTheText:
    FindWindow $0 "#32770" "" $HWNDPARENT
    GetDlgItem $0 $0 1006
    SendMessage $0 ${WM_SETTEXT} 0 "STR:$1"
FunctionEnd

Function SetPaths
  IfFileExists $INSTDIR\reaper.ini Portable
    StrCpy $INI_PATH $APPDATA\Reaper
    StrCpy $GROOVE_DIR $INI_PATH\Grooves
  	Goto SetGrooveDir
  Portable:
    StrCpy $INI_PATH $INSTDIR
  SetGrooveDir:
  StrCpy $GROOVE_DIR $INI_PATH\Grooves
FunctionEnd
