; rievallib-indirect-register-stale-d1 (#637) - PFEvaluate checks the "}"
; of an indirection {...} in d1, the last character its number loops
; read: a register operand ({d0}) does not go through them, d1 is stale,
; and the check mostly fails with error 5 "No close bracket on
; indirect!" (if d1 happens to be "}", the "}" is then read as a value 0).
; Library 121 is the acidlibs' polylib on the CD: the cells with the CD's
; libraries get the acidlibs of the variant rwe-1999-eval too (C build,
; without polylib).
; Correct: {d0} is the long at the address in d0, here 42; "{d0} 1 +" 43.
DEFTYPE .l

!TestCase{register_indirect}
  x=42
  Dim r.l(16)
  r(0)=&x
  PFRegisters &r(0)
  v1=PFEvaluate("{d0}")
  t1=PFErrorType
  v2=PFEvaluate("{d0} 1 +")
  t2=PFErrorType
  !CheckEq{v1,42,"indirect_result"}
  !CheckEq{t1,0,"indirect_error"}
  !CheckEq{v2,43,"sum_result"}
  !CheckEq{t2,0,"sum_error"}
!EndTestCase
