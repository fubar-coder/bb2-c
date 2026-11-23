; compiler-incdir-unquoted (#17) - an unquoted path of IncDir and an
; unquoted name of Include are the rest of the line, also after ':' (the
; fix of #17 changes only quoted names): IncDir CORPUS:src/ sets the path
; "CORPUS:src/", Include zz.bb2 opens "CORPUS:src/zz.bb2".
; Correct: the included file defines #k as 7.
; A regression test of the fix of #17: the original already gives this
; (correct) result.
; (The Include stands in the case: a source holds no Include outside the
; blocks.)

!TestCase{incdir_unquoted}
  IncDir CORPUS:src/
  Include zz.bb2
  !CheckEq{#k,7,"constant_of_included_file"}
!EndTestCase
