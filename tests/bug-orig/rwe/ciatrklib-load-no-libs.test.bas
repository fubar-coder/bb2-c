; ciatrklib-load-no-libs (#443) - the declaration of LoadTrackerModule
; has no !libs (its {#doslib,#la6} is commented out), so the compiler does
; not load DOSBase into a6, but the routine calls Lock, Examine,
; OpenFromLock, Read, Close and IoErr through a6: through whatever a6
; holds at the call.  The program sets a6 to 1 right before the call
; (inline assembler): JSR -$54(a6) jumps to an odd address, an address
; error (the session ends in a timeout).
; Correct: the call loads dos.library's base itself; the module is
; loaded.  The module is the one of the test of #436.
DEFTYPE .l

!TestCase{load_a6}
  n$="CORPUS:n.mod"
  MOVE.l #1,a6
  r=LoadTrackerModule(0,n$)
  !CheckEq{r,-1,"loaded"}
  !CheckEq{GetTrackerSize(0),2111,"size"}
  FreeTrackerModule 0
!EndTestCase
