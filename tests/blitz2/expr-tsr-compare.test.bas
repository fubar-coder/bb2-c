; expr-tsr-compare (#1235, type-system-rework) - comparisons compare the
; full value (spec compiler/expression-types, "Comparisons compare the
; full value"): a comparison, If, While, Until and Select/Case compare the
; exact values of both operands (whole, 48.16 or Float, the wider of the
; two) without converting them first; the result of a comparison is a
; Word, -1 or 0, converted at the destination.
; The original's values, where they differ, are the declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{a_quick_sum_beyond_the_range}
  q=20000.5
  rl.l=0
  If q+q>40000 Then rl=1
  !CheckEq{rl,1,"if_true"}
  r2l.l=(q+q>40000)
  !CheckEq{r2l,-1,"value_true"}
!EndTestCase

; the other statements that compare
!TestCase{while_until_select}
  q=20000.5
  rl.l=0
  While q+q>40000 AND rl<1
    rl+1
  Wend
  !CheckEq{rl,1,"while_true"}
  r2l.l=0
  Repeat
    r2l+1
  Until q+q>40000 OR r2l>1
  !CheckEq{r2l,1,"until_true"}
  q=20500.5
  r3l.l=0
  Select q+q
    Case 41001
      r3l=1
    Default
      r3l=2
  End Select
  !CheckEq{r3l,1,"select_case"}
!EndTestCase

!TestCase{a_whole_quotient_and_a_fraction}
  l.l=10
  rl.l=(l/3>3.2)
  !CheckEq{rl,0,"r_whole_quotient"}
  r2l.l=(l/3.0>3.2)
  !CheckEq{r2l,-1,"r2_point_quotient"}
!EndTestCase
