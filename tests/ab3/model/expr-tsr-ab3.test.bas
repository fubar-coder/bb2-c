; expr-tsr-ab3 (#1235, type-system-rework) - AB3 mode follows the rework's
; rules as blitz2 does (spec compiler/expression-types, "Both compiler
; modes follow the rework's rules" and the AB3 scenario of "The type and
; format of an operation do not depend on the order of its operands"):
; the whole division below a comparison stays whole (x.q=(7/2=3) is -1), a
; Quick left of a shift keeps its fraction (r.l=q LSL 2 is 6), a chain of
; a Long and Quicks gives the same value in both orders; and a few of the
; rules of the expr-tsr tests of tests/blitz2: a whole operation one step
; wider, a 48.16 product beyond the Quick range, the exception of the
; shift at the top under a Quick, the division exception.
; The cell of AmiBlitz3 v3.14.0 is left out: its values for these cases
; are not measured, the differences from it are rows of docs/ab3/variant.md
; with the rules of the rework (task 4.8).
; Correct: the values of the spec's scenarios.
!TestCase{ab3_scenarios}
  q.q=1.5
  xq.q=(7/2=3)
  !CheckQEq{xq,-1,"x_compare"}
  rl.l=q LSL 2
  !CheckEq{rl,6,"r_shift"}
!EndTestCase

!TestCase{ab3_chain_orders}
  l.l=10 : q.q=1.5 : p.q=1.5
  rl.l=(l+q+p) AND 255
  !CheckEq{rl,13,"and_long_first"}
  r2l.l=(q+p+l) AND 255
  !CheckEq{r2l,13,"and_long_last"}
!EndTestCase

!TestCase{ab3_rules}
  b.b=100 : c.b=100 : w.w=200 : q2.q=300 : p2.q=300 : r3.q=0.25
  xb.b=b+c
  !CheckEq{xb,-56,"b_sum_wraps_at_the_destination"}
  yq.q=w/9
  !CheckEq{Peek.l(&yq),$001638E3,"w_over_9_under_a_quick"}
  zq.q=q2*p2*r3
  !CheckQEq{zq,22500,"product_in_48_16"}
  uq.q=1 LSR 7
  !CheckEq{Peek.l(&uq),$00000200,"shift_at_the_top"}
!EndTestCase
