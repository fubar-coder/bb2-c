; fileiolib-reopen-fields-name (#138) - OpenFile, ReadFile and WriteFile
; first close the File#; when the open file has Fields, the loop that
; frees them leaves the argument pointer at 0, and the length of the new
; file name is then read from -4(0) (on the A1200's 24 bit bus the last
; long of the ROM) and the name is cut there.
; Here a file with a name of 38 characters is opened with OpenFile and
; given Fields, then opened again with ReadFile without CloseFile.
; Correct: the second open finds the file, the name stays whole.  The
; original cuts the name and does not find the file.
; @fixed #138
; @configs clib
f$="RAM:fileiolib-reopen-fields-name-1.dat"
NPrint "name length ",Len(f$)
If OpenFile(0,f$)
  NPrint "opened"
  Fields 0,a.l
  If ReadFile(0,f$) Then NPrint "reopened" Else NPrint "reopen failed"
  CloseFile 0
Else
  NPrint "open failed"
EndIf
KillFile f$
End
