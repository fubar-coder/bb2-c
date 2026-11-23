; riamosfunclib-bload-read-error (#488) - after Read, BLoad tests only
; for 0 and returns -1 (success) otherwise; Read returns -1 on an error.
; The program saves 100 bytes with BSave and loads them again; inject
; makes the first Read of the file fail (readfail=).
; Correct: BLoad reports the failed read with 0.
DEFTYPE .l

!TestCase{bload_read_error}
  a=AllocMem_(100,$10001)
  !Require{a<>0,"memory"}
  r=BSave("CORPUS:bug-orig-bload",a,100)
  r=BLoad("CORPUS:bug-orig-bload",a,100)
  !CheckEq{r,0,"result"}
  FreeMem_ a,100
!EndTestCase

!TestBatchCleanup
  KillFile "CORPUS:bug-orig-bload"
!EndTestBatchCleanup
