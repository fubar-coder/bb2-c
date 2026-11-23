; compiler-float-trailing-dot (#10) - fetchpoint2 wants a digit after the
; decimal point: "1." is a syntax error.
; Correct: "1." is the float 1.
!TestCase{trailing_dot}
  a.f=1.
  !CheckFltNear{a,1,0.001,"one"}
  b.f=2.+1
  !CheckFltNear{b,3,0.001,"sum"}
!EndTestCase
