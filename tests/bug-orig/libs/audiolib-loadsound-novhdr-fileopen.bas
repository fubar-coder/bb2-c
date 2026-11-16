; audiolib-loadsound-novhdr-fileopen (#67) - LoadSound and DiskPlay open
; the file (doopensound, iffiolib iffopen) and go to the runtime errors
; "Can't Find VHDR" and "Error Reading File" without iffclose: the file
; stays open (iffiolib has no finit, also after the program ends).  The
; 8SVX here (generated) has no VHDR chunk.  SetErr catches the error;
; then the program asks for an exclusive lock on the file, which fails
; while it is open (object in use).
; Correct: after the error the file is closed - the exclusive lock works.
; @fixed #67
; @configs clib
; @pass file=novhdr.8svx|@TESTDATA@/audiolib-loadsound-novhdr-fileopen.8svx
SetErr
  NPrint "error"
  l.l=Lock_("CORPUS:novhdr.8svx",-1)
  NPrint "exclusive lock ",l<>0
  If l Then UnLock_ l
  End
End SetErr
LoadSound 0,"CORPUS:novhdr.8svx"
NPrint "loaded"
End
