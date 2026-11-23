; ciatrklib-karplus-loop0 (#439) - E8x (Karplus-Strong) averages the
; bytes of the voice's loop with a DBRA counter of loop length x 2 - 2:
; for a loop length of 0 it rewrites 65535 bytes from the loop start.  A
; voice that never played a sample has loop start 0 and length 0: E8x
; there rewrites the memory from address 0 on - a crash (the session ends
; in a timeout).  The module (generated) has E81 in row 0 of channel 0,
; without a note.  The ticks are played by the program
; (ChangeTrackerVBlank, PlayTracker: the CIA interrupt of the original
; does not play in the emulator, #450).
; Correct: E8x does nothing without a loop; the program goes on.
DEFTYPE .l

!TestCase{karplus}
  dosb=OpenLibrary_("dos.library",0)       ; the original needs DOSBase in a6 here (#443)
  GetReg a6,dosb
  r=LoadTrackerModule(0,"CORPUS:k.mod")
  !Require{r=-1,"loaded"}
  e=Peek.l(4)
  ChangeTrackerVBlank
  r=StartTracker(0)
  For i=1 To 12 : PlayTracker : Next
  StopTracker
  !Check{Peek.l(4)=e,"execbase_pointer_kept"}
!EndTestCase
