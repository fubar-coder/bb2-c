; ciatrklib-waitevent-no-libs (#453) - the declaration of
; WaitTrackerEvent has no !libs (its {#graphicslib,#la6} is commented
; out), so the compiler does not load GfxBase into a6, but the routine
; calls WaitTOF as JSR -270(a6): through whatever a6 holds at the call.
; The module (generated) has the event 805 in row 0; the program plays it
; without the CIA interrupt (ChangeTrackerVBlank) from a vertical blank
; interrupt (SetInt 5 with PlayTracker) and sets a6 to 1 right before
; WaitTrackerEvent (inline assembler): the JSR goes to an odd address, an
; address error (the session ends in a timeout).
; Correct: the call loads graphics.library's base itself, waits for the
; event and returns.
DEFTYPE .l

!TestCase{wait_event}
  dosb=OpenLibrary_("dos.library",0)       ; the original needs DOSBase in a6 here (#443)
  GetReg a6,dosb
  r=LoadTrackerModule(0,"CORPUS:w.mod")
  !Require{r=-1,"loaded"}
  ChangeTrackerVBlank
  r=StartTracker(0)
  SetInt 5
    PlayTracker
  End SetInt
  MOVE.l #1,a6
  WaitTrackerEvent
  ClrInt 5
  StopTracker
  !CheckEq{GetTrackerEvent,5,"event"}
!EndTestCase
