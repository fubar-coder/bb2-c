; brexxlib-loadtape-unchecked (#101) - LoadTape reads the 8 byte start
; time without checking the result: any file becomes a tape, a file
; shorter than 8 bytes gives a start time from its bytes.
; Here: a file of 5 bytes ("ABCDE").
; Correct: the read of the start time fails, the tape stays empty (no
; events, start time 0).  The original takes "ABCDE" as the start time.
; @fixed #101
; @configs clib
DEFTYPE .l
If WriteFile(0,"RAM:bl101.tape") Then FileOutput 0 : Print "ABCDE" : CloseFile 0
DefaultOutput
LoadTape 0,"RAM:bl101.tape"
KillFile "RAM:bl101.tape"
a=Addr Tape(0)
NPrint "events ",Peek.l(a)<>0," time ",Hex$(Peek.l(a+4))," ",Hex$(Peek.l(a+8))
End
