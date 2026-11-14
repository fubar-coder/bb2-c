; fileiolib-exists-empty-file (#143, won't fix) - Exists returns the length
; of the file (the manual: "Exists actually returns the length of the
; file ... If 0 the file either does not exist or is empty"), so an
; existing empty file gives 0 like a missing one.  Kept: programs use the
; value as the length (AllocMem, ReadMem, LoadBank), and a new value for
; an empty file would break them; "is it there" is ReadFile.
; Here an empty file is written and closed, then Exists of it and of a
; missing file.
; Correct: the empty file is told apart from the missing one.  The
; original (and the port) give 0 for both.
; @configs clib
e$="RAM:fileiolib-exists-empty-file.dat"
m$="RAM:fileiolib-exists-empty-file.none"
KillFile m$
If WriteFile(0,e$) Then CloseFile 0
If Exists(e$) Then NPrint "empty file there" Else NPrint "empty file missing"
If Exists(m$) Then NPrint "missing file there" Else NPrint "missing file missing"
If ReadFile(0,e$) Then NPrint "readfile of the empty file ok" : CloseFile 0
KillFile e$
End
