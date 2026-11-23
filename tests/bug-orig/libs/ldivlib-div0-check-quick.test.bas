; ldivlib-div0-check-quick (#200) - a Quick division by a variable 0,
; with runtime error checking.  ldivlib's quickdiv has no error check: a
; divisor 0 gives $7FFFFFFF (the dividend's sign since the fix of #200)
; without a message, also with runtime error checking, while a Word
; division by 0 is the debugger's "GURU! - Division by Zero".
; Correct: with runtime error checking the division by 0 is "Division by
; Zero" (the error check of quickdiv); a valid division first.
DEFTYPE .l

!TestCase{divisor_zero}
  a.q=7 : b.q=2
  t.q=a/b
  !CheckQEq{t,3.5,"valid"}
  b=0
  !ExpectError{"Division by Zero","refused"}
  c.q=a/b
  !Check{0,"after"}
!EndTestCase
