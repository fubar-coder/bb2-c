; audiolib-loadsound-fileopen (#67) - LoadSound checks the length of the
; sample only after iffopen and goes to the runtime error "Sound too long"
; without iffclose: the file stays open (iffiolib has no finit, also
; after the program ends).  The 8SVX here (generated) has a BODY of 128K.
; SetErr catches the error; then the program asks for an exclusive lock
; on the file, which fails while it is open (object in use).
; Correct: after the error the file is closed - the exclusive lock works.
; @configs clib
; @pass file=big.8svx|@TESTDATA@/audiolib-loadsound-fileopen.8svx
SetErr
  NPrint "error"
  l.l=Lock_("CORPUS:big.8svx",-1)
  NPrint "exclusive lock ",l<>0
  If l Then UnLock_ l
  End
End SetErr
LoadSound 0,"CORPUS:big.8svx"
NPrint "loaded"
End
