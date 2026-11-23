; ciatrklib-v36-calls (#1146) - LoadTrackerModule locks the file,
; examines it and opens it with OpenFromLock, a dos.library V36 function,
; on the program's dos.library without checking its version; on
; Kickstart 1.3 (dos V34) the offset lies behind dos.library's jump
; table: the program crashes at LoadTrackerModule (the session ends in a
; timeout).  The module is the one of the test of #436.
; Correct: on every Kickstart LoadTrackerModule opens the file by its
; name while it holds the lock and unlocks the lock after that; the
; module is loaded, and the program ends normally.
DEFTYPE .l

!TestCase{load_module}
  n$="CORPUS:n.mod"
  dosb=OpenLibrary_("dos.library",0)       ; the original needs DOSBase in a6 here (#443)
  GetReg a6,dosb
  r=LoadTrackerModule(0,n$)
  !CheckEq{r,-1,"loaded"}
  !CheckEq{GetTrackerSize(0),2111,"size"}
  FreeTrackerModule 0
!EndTestCase
