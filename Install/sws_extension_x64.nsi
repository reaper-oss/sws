; sws_extension_x64.nsi
;--------------------------------

Name "SWS Extension x64"
OutFile "output\sws_extension_x64.exe"
InstallDir "$PROGRAMFILES64\Reaper (x64)"
!define CPUBITS 64
!define OUTPUTFILE ..\x64\Release\reaper_sws64.dll

; Do all the work!
!include sws_extension.nsh
