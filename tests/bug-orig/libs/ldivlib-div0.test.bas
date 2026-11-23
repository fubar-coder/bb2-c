; ldivlib-div0 (#200) - the Long and Quick division of the 68000 (ldivlib)
; tests the divisor for 0 before it looks at the signs and returns
; $7FFFFFFF, the largest positive number, for any dividend: -5/0 is
; positive.
; Correct: the result has the sign of the dividend: -$7FFFFFFF for a
; negative dividend, $7FFFFFFF for a positive one and for 0.
DEFTYPE .l

!TestCase{long_div0}
  a=-5 : b=0
  !CheckEq{a/b,-2147483647,"negative"}
  a=5
  !CheckEq{a/b,2147483647,"positive"}
  a=0
  !CheckEq{a/b,2147483647,"zero"}
!EndTestCase

!TestCase{quick_div0}
  q.q=-5 : r.q=0 : s.q=q/r
  !CheckEq{Peek.l(&s),$80000001,"negative"}
  q=5 : s=q/r
  !CheckEq{Peek.l(&s),$7FFFFFFF,"positive"}
!EndTestCase
