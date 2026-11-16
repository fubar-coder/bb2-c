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
; @fixed #453
; @rwe ciatrklib
; @pass file=w.mod|@TESTDATA@/ciatrklib-waitevent-no-libs.mod
; @pass timeout=30
DEFTYPE .l
r=LoadTrackerModule(0,"CORPUS:w.mod")
NPrint "loaded ",r
If r=0 Then End
ChangeTrackerVBlank
r=StartTracker(0)
SetInt 5
  PlayTracker
End SetInt
MOVE.l #1,a6
WaitTrackerEvent
ClrInt 5
StopTracker
NPrint "event ",GetTrackerEvent
End
