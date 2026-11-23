; riamosfunclib-changedir-currentdir-0 (#490) - ChangeDir tests the
; result of CurrentDir (the previous current directory) and returns 0 if
; it is 0, although the new lock is already current; the lock is not
; remembered.  The program makes 0 its current directory first (dos
; CurrentDir) and restores its own at the end.
; Correct: ChangeDir succeeds (-1), RAM: is the current directory.
DEFTYPE .l

!TestCase{changedir_currentdir_0}
  o=CurrentDir_(0)
  r=ChangeDir("RAM:")
  c=CurrentDir_(o)
  !CheckEq{r,-1,"result"}
!EndTestCase
