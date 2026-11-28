; expr-tsr-div-whole (#1235, type-system-rework) - a whole division rounds
; towards zero (spec compiler/expression-types, "A whole division rounds
; towards zero"): `/` of two whole operands gives a whole number, whatever
; the destination (the exceptions under a Quick or Float destination and
; beside a Quick are expr-tsr-div-exception and expr-tsr-div-beside-quick);
; with a Quick operand the fraction stays.
; The original's values, where they differ, are the declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{negative_and_positive}
  i.l=7
  rl.l=-7/2
  !CheckEq{rl,-3,"r_negative"}
  r2l.l=i/2*2
  !CheckEq{r2l,6,"r2_product"}
  xq.q=(7/2=3)
  !CheckQEq{xq,-1,"x_compare"}
!EndTestCase

!TestCase{print_negative_and_positive}
  NPrint 7/2
!EndTestCase

!TestCase{quick_operand}
  q=7
  rl.l=q/2*2
  !CheckEq{rl,7,"r_quick_quotient"}
  xq.q=7.0/2
  !CheckQEq{xq,3.5,"x_point_quotient"}
!EndTestCase
