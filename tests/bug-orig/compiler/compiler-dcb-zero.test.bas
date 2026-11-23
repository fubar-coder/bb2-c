; compiler-dcb-zero (#37) - dodcb counts the copies down in do-while loops:
; a count of 0 makes 2^32 copies, the compiler runs for hours (here: the
; session's time limit).
; Correct: Dcb with a count of 0 emits nothing.
!TestCase{dcb_zero}
  Goto skip
.la
  Dcb.b 0,1
.lb
  Dcb.w 0,1
.lc
  Dcb.l 0,1
.ld
.skip
  n1.l=?lb-?la
  n2.l=?lc-?lb
  n3.l=?ld-?lc
  !CheckEq{n1,0,"bytes"}
  !CheckEq{n2,0,"words"}
  !CheckEq{n3,0,"longs"}
!EndTestCase
