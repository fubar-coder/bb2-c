; compiler-loadres-clash-off (#36) - the residents of
; compiler-loadres-clash-offset (#c: 1 and 2) with the warnings setting
; "off" (word +24 of the .xtra $0000): the warning resclash is disabled.
; Correct: no message (the compile log has no XTRAERR line); #c is the
; first resident's, 1.
; A regression test of the fix of #36: the original already gives this
; (correct) result.

!TestCase{clash_warning_off}
  !CheckEq{#c,1,"constant_of_first_resident"}
!EndTestCase
