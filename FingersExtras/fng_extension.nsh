; 
;
; Dump reaper_fingers.dll into the proper plugins dir and delete old vers

;--------------------------------
!define SF_SELECTED   1
!define ExtVersion 013beta3

Name "finger's REAPER extension"

OutFile Release\fingers_extension_${ExtVersion}_${Cpu}.exe
InstallDir ${progFiles}\Reaper\Plugins
Var /GLOBAL GROOVE_DIR
Var /GLOBAL PYTHON_DIR
RequestExecutionLevel highest
LicenseData license.txt


Function .onInit
  SetRegView ${nCpu}
  ReadRegStr $0 HKLM Software\Reaper ""
  StrCmp $0 "" +1
  StrCpy $INSTDIR $0\Plugins
  StrCpy $GROOVE_DIR $APPDATA\REAPER\Grooves
  StrCpy $PYTHON_DIR $APPDATA\REAPER\Python
FunctionEnd

Page license
Page components
Page directory plugDirNeeded
PageEx directory
  DirVar $GROOVE_DIR
  DirText "Install grooves"
  PageCallbacks grooveDirNeeded
PageExEnd
PageEx directory
  DirVar $PYTHON_DIR
  DirText "Install MIDI Python hacks"
  PageCallbacks pythonDirNeeded
PageExEnd
Page instfiles

Section "Plugin" plug_id
  SetOutPath $INSTDIR
  CheckReaper:
  FindProcDLL::FindProc "reaper.exe"
  StrCmp $R0 "1" "" ReaperClosed
  MessageBox MB_OKCANCEL "Please close all instances of REAPER before continuing installation." IDOK CheckReaper IDCANCEL Abort  
  ReaperClosed:
  Delete "$INSTDIR\reaper_FNG_Extension.dll"
  Delete "$INSTDIR\reaper_FNG_Extension_x64.dll"
  File Build\${Cpu}Release\reaper_fingers.dll
  Abort:
SectionEnd

Section "Grooves" groove_id
  SetOutPath $GROOVE_DIR
  File Grooves\*.rgt
  WriteINIStr $APPDATA\REAPER\REAPER.ini "fingers" "groove_dir" $GROOVE_DIR
SectionEnd

Section "Python MIDI hacks" python_id
  SetOutPath $PYTHON_DIR
  File python\*.py
SectionEnd

Function grooveDirNeeded
Push $0
Push $1
# check Grooves section for selected flag
SectionGetFlags ${groove_id} $1
IntOp $0 ${SF_SELECTED} & $1
IntCmpU $0 0 isequal done done
isequal:
  Pop $0
  Pop $1
  Abort
done:
Pop $0
Pop $1
FunctionEnd

Function plugDirNeeded
Push $0
Push $1
# check component section for selected flag on plugin
SectionGetFlags ${plug_id} $1
IntOp $0 ${SF_SELECTED} & $1
IntCmpU $0 0 isequal done done
isequal:
  Pop $0
  Pop $1
  Abort
done:
Pop $0
Pop $1
FunctionEnd

Function pythonDirNeeded
Push $0
Push $1
# check component section for selected flag on plugin
SectionGetFlags ${python_id} $1
IntOp $0 ${SF_SELECTED} & $1
IntCmpU $0 0 isequal done done
isequal:
  Pop $0
  Pop $1
  Abort
done:
Pop $0
Pop $1
FunctionEnd
