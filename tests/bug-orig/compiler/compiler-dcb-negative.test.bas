; compiler-dcb-negative (#37) - dodcb takes the count as unsigned: a count
; of -1 makes 2^32-1 copies, the compiler runs for hours (here: the
; session's time limit).
; Correct: a negative count is an error.
!TestCase{dcb_negative}
  Goto skip
  Dcb.w -1,0
  .skip
  !CheckEq{1,1,"after_dcb"}
!EndTestCase
