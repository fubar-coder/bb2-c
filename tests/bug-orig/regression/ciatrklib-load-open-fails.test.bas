; ciatrklib-load-open-fails - LoadTrackerModule of a name that can be
; locked and examined but not opened (a directory, RAM:ciadir): the open
; by name fails (#1146 made the open by name and checks it).  On the
; a1200 the original's OpenFromLock gives the same results, so this is a
; test of the port's path, not of an original bug.
; Correct: the slot is freed, the result is not -1 (IoErr), and no lock
; is left on the directory: an exclusive lock on it works.

DEFTYPE .l

!TestCase{load_directory}
  k=CreateDir_("RAM:ciadir")
  !Require{k<>0,"directory_created"}
  UnLock_ k
  r=LoadTrackerModule(0,"RAM:ciadir")
  !CheckEq{r=-1,0,"result_not_minus_one"}
  !CheckEq{GetTrackerSize(0),0,"slot_freed"}
  l=Lock_("RAM:ciadir",-1)
  !Check{l<>0,"exclusive_lock"}
  If l Then UnLock_ l
!EndTestCase
