; ciatrklib-load-readerror-unlock (#444) - LoadTrackerModule opens the
; module with OpenFromLock (the file handle then owns the lock, Close
; frees it); when Read returns -1 it closes the file and then UnLocks the
; same lock a second time.  inject makes the first Read of the file fail
; (readfail=, which follows the Lock of the name into OpenFromLock) and,
; with ownedunlock, counts an UnLock of a lock a file owns and does not
; do it (a second free of the lock would corrupt memory).  The module is
; the one of the test of #436.
; Correct: after the failed Read the lock is not unlocked again.
; @rwe ciatrklib
; @pass file=e.mod|@TESTDATA@/ciatrklib-free-while-playing.mod
; @pass runwrap=@INJECT@|readfail=e.mod/1 ownedunlock
DEFTYPE .l
r=LoadTrackerModule(0,"CORPUS:e.mod")
NPrint "after"
End
