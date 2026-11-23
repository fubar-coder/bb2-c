; compiler-loadres-clash-same (#36) - two residents define the constant
; #c with the same value (both made of the source #c=1 of
; compiler-loadres-clash-offset): the same definition included twice.
; Correct: no message (only differing values warn; the compile log has no
; XTRAERR line); #c is 1.
; A regression test of the fix of #36: the original already gives this
; (correct) result.

!TestCase{clash_same_value}
  !CheckEq{#c,1,"constant"}
!EndTestCase
