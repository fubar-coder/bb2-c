; rievallib-spaces-only-return-address (#644) - PFEvaluate of a string of
; spaces only sets error 4 "No result value!" but returns the long above
; its empty stack, the return address into the program, instead of 0.
; Library 121 is the acidlibs' polylib on the CD: the cells with the CD's
; libraries get the acidlibs of the variant rwe-1999-eval too (C build,
; without polylib).
; Correct: the result is 0, the error 4.
DEFTYPE .l

!TestCase{spaces_only}
  r=PFEvaluate("   ")
  t=PFErrorType
  !CheckEq{r,0,"result"}
  !CheckEq{t,4,"error"}
!EndTestCase
