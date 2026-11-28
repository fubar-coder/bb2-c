; expr-tsr-const-point (#1235, type-system-rework; #1230, #1231) - a
; constant with a point is read by its operands (spec compiler/expression-
; types, "A constant with a point is read by its operands"): never whole;
; a Quick when its value is exact in 16.16 or when a Quick is the other
; operand of the same operator (also under a unary minus), else the Float
; of the written value; read as a Quick it is the FFP value of its text cut
; down to 1/65536.  The original's values (3/2.0 is 1, #1231) are the
; declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{print_point_and_no_fraction}
  NPrint 3/2.0
  NPrint 7/2.0+0.5
!EndTestCase

!TestCase{beside_a_quick}
  q=0.1
  rl.l=0
  If q=0.1 Then rl=1
  !CheckEq{rl,1,"if_equal"}
  xq.q=(0.1=0.1)
  !CheckQEq{xq,-1,"x_two_constants"}
  yq.q=(1.3<1.3)
  !CheckQEq{yq,0,"y_less"}
!EndTestCase

!TestCase{under_a_unary_minus}
  q=3 : q2=1
  xq.q=q+(-0.1)
  !CheckEq{Peek.l(&xq),$0002E667,"x_minus_constant"}
  yq.q=q2+30000.3
  !CheckEq{Peek.l(&yq),$75314D00,"y_beyond_exact"}
!EndTestCase

!TestCase{print_under_a_unary_minus}
  q3=157
  NPrint q3*(-0.3)
!EndTestCase

!TestCase{beside_whole_numbers}
  l.l=-3 : l2.l=100
  rl.l=l*0.5
  !CheckEq{rl,-2,"r_exact_half"}
  r2l.l=l2*0.1
  !CheckEq{r2l,10,"r2_tenth"}
!EndTestCase

!TestCase{print_beside_whole_numbers}
  l3.l=1000000
  NPrint l3*0.001
!EndTestCase

!TestCase{beside_a_quick_variable}
  q=1
  xq.q=q+0.1
  !CheckEq{Peek.l(&xq),$00011999,"x_sum"}
!EndTestCase
