; riamosfunclib-bload-name-result (#486) - BLoad(name$) loads a file into
; a new bank and returns "the number of the bank ... or -1 for failure"
; (RIAmosFuncLib's manual).  When Reserve fails it returns 0, the number
; of the first bank; when the load fails it ignores that and returns the
; bank, which stays reserved with undefined contents.  The program saves
; 4321 bytes of the ROM; inject makes the first AllocMem of 4321 bytes
; fail (the first BLoad's Reserve) and the first Read of the file fail
; (the second BLoad's load).
; Correct: both BLoads return -1, bank 0 stays free.
DEFTYPE .l

!TestCase{bload_name_result}
  r=BSave("CORPUS:bug-orig-bload4",$F80000,4321)
  !Require{r=-1,"saved"}
  b=BLoad("CORPUS:bug-orig-bload4")
  !CheckEq{b,-1,"reserve_fails"}
  b=BLoad("CORPUS:bug-orig-bload4")
  !CheckEq{b,-1,"read_fails"}
  !CheckEq{Start(0),0,"bank_0_free"}
!EndTestCase

!TestBatchCleanup
  KillFile "CORPUS:bug-orig-bload4"
!EndTestBatchCleanup
