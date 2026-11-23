; ldivlib-div0-check-long (#200) - a Long division by a variable 0, with
; runtime error checking.  ldivlib's longdiv has no error check: a
; divisor 0 gives $7FFFFFFF (the dividend's sign since the fix of #200)
; without a message, also with runtime error checking, while a Word
; division by 0 is the debugger's "GURU! - Division by Zero".
; Correct: with runtime error checking the division by 0 is "Division by
; Zero" (the error check of longdiv); a valid division first.
DEFTYPE .l

!TestCase{divisor_zero}
  a=7 : b=2
  !CheckEq{a/b,3,"valid"}
  b=0
  !ExpectError{"Division by Zero","refused"}
  c=a/b
  !Check{0,"after"}
!EndTestCase
