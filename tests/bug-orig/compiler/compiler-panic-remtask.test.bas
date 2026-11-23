; compiler-panic-remtask (#24) - PANIC in the stop requester of a program
; run from the editor: menu10 removes the program's process with RemTask,
; nothing of the program is cleaned up.  The job line of the test runs the
; victim (compiler-panic-remtask.test/victim.bas) from the editor (COMPILE/RUN)
; and hits PANIC 2 s after it started; the victim has opened RAM:panic.dat
; and waits.  Then the case asks for an exclusive lock on that file: it
; fails while the file is still open.
; Correct: the aborted program's file is closed - the exclusive lock works.
; #24 is not fixed on purpose (a clean abort of a running program is not
; worth the complexity): the test stays REPRODUCED.
DEFTYPE .l

!TestCase{lock_after_panic}
  l=Lock_("RAM:panic.dat",-1)
  !Check{l<>0,"exclusive_lock"}
  If l Then UnLock_ l
  DeleteFile_ "RAM:panic.dat"
!EndTestCase
