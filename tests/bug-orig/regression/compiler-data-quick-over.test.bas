; compiler-data-quick-over (#1074) - the integer 32768 without a '-' is
; too big for a Quick: Data.q 32768 is bad data, as in the original.  The
; fix of #1074 takes only the integer 32768 that Data's own '-' negates.
; Correct: the compile error "Bad Data" at the Data line.
; A regression test of the fix of #1074: the original already gives this (correct) result.
.quickdata
Data.q 32768

!TestCase{quick_over}
  Restore quickdata
  Read q.q
  !CheckEq{Int(q),0,"after_read"}
!EndTestCase
