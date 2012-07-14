; sws_extension.nsi
;--------------------------------

Name "SWS Extension"
OutFile "output\sws_extension.exe"
InstallDir "$PROGRAMFILES32\Reaper\"
!define CPUBITS 32
!define OUTPUTFILE ..\Release\reaper_sws.dll

; Do all the work!
!include sws_extension.nsh
