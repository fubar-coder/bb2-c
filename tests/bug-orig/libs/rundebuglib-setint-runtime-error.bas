; rundebuglib-setint-runtime-error (#1073) - a runtime error inside a SetInt
; routine of an executable compiled with runtime error checking: the
; routine runs in supervisor mode, and the original's TRAP #0 handler gave
; such a trap to the TRAP #0 vector it found (exec's), with the vectors
; put back; exec does not hand a trap of supervisor mode to a task, the
; machine stops.  rundebuglib cannot show its requester from an interrupt.
; Here the routine counts the frames and reads an array out of range at
; the count of 20; the main program waits 100 frames.
; Correct: the runtime error is reported as any other (the requester) and
; the program ends: the routine is left and runs no more, the main
; program's next statement shows the error.
; @fixed #1073
; @pass runtime_errors=on
DEFTYPE .l
Dim a(5)
SetInt 5
  c+1
  If c=20 Then a(c)=1
End SetInt
NPrint "start"
For i=1 To 100 : VWait : Next
NPrint "c ",c
End
