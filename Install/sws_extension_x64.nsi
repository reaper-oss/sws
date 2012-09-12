; sws_extension_x64.nsi
;--------------------------------

Name "SWS Extension x64"
OutFile "output\sws_extension_x64.exe"
InstallDir "$PROGRAMFILES64\Reaper (x64)\"
!define CPUBITS 64
!define OUT_DLL ..\x64\Release\reaper_sws64.dll
!define OUT_PY ..\x64\Release\sws_python.py

; Do all the work!
!include sws_extension.nsh
