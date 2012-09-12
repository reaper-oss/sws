; sws_extension.nsi
;--------------------------------

Name "SWS Extension"
OutFile "output\sws_extension.exe"
InstallDir "$PROGRAMFILES32\Reaper\"
!define CPUBITS 32
!define OUT_DLL ..\Release\reaper_sws.dll
!define OUT_PY ..\Release\sws_python.py

; Do all the work!
!include sws_extension.nsh
