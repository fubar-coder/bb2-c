; ciatrklib-karplus-loop0 (#439) - E8x (Karplus-Strong) averages the
; bytes of the voice's loop with a DBRA counter of loop length x 2 - 2:
; for a loop length of 0 it rewrites 65535 bytes from the loop start.  A
; voice that never played a sample has loop start 0 and length 0: E8x
; there rewrites the memory from address 0 on - a crash (the session ends
; in a timeout).  The module (generated) has E81 in row 0 of channel 0,
; without a note.  The ticks are played by the program
; (ChangeTrackerVBlank, PlayTracker: the CIA interrupt does not play in
; the emulator).
; Correct: E8x does nothing without a loop; the program goes on.
; @rwe ciatrklib
; @pass file=k.mod|@TESTDATA@/ciatrklib-karplus-loop0.mod
DEFTYPE .l
r=LoadTrackerModule(0,"CORPUS:k.mod")
NPrint "loaded ",r
If r
  e=Peek.l(4)
  ChangeTrackerVBlank
  r=StartTracker(0)
  For i=1 To 12 : PlayTracker : Next
  StopTracker
  NPrint "execbase pointer kept ",Peek.l(4)=e
EndIf
NPrint "after"
End
