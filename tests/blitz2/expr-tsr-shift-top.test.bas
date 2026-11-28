; expr-tsr-shift-top (#1235, type-system-rework) - a shift at the top under
; a Quick destination (spec compiler/expression-types, "A shift at the top
; under a Quick destination", "A left shift at the top under a Quick
; destination wraps"): a shift of a whole left operand that is the whole
; expression or is reached from it through binary + and - only (not
; through a unary minus) is the shift exception: a right shift shifts the
; value with 16 fraction bits (on the 16.16 bits inside the Quick range,
; in 48.16 beyond it), a left shift shifts it as a Long.  A left shift that
; is the whole expression, of a whole or of a Quick left operand, converts
; to the Quick wrapped (the low 32 bits of its 16.16 form); one reached
; through + or - is a Long and the sum converts saturated.
; The original's values (the spec names 1.95's -(1 LSR 7) as -1/128), where
; they differ, are the declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{right_shifts}
  b.b=3
  xq.q=1 LSR 7
  !CheckEq{Peek.l(&xq),$00000200,"x_one_over_128"}
  yq.q=b LSR 3
  !CheckQEq{yq,0.375,"y_byte"}
  zq.q=1 LSR 4
  !CheckQEq{zq,0.0625,"z_one_sixteenth"}
!EndTestCase

!TestCase{beyond_the_range_and_negative}
  vb.b=-3
  xq.q=100000 LSR 2
  !CheckQEq{xq,25000,"x_beyond_the_range"}
  yq.q=vb LSR 3
  !CheckEq{Peek.l(&yq),$1FFFA000,"y_negative"}
!EndTestCase

!TestCase{through_plus_minus}
  qa=1.25 : v.b=63
  xq.q=qa+3 LSR 3
  !CheckQEq{xq,1.625,"x_sum"}
  yq.q=(v LSR 3)+14
  !CheckQEq{yq,21.875,"y_sum"}
!EndTestCase

!TestCase{below_another_operator}
  qa=1.25
  xq.q=qa*(3 LSR 3)
  !CheckQEq{xq,0,"x_product"}
  yq.q=-(1 LSR 7)
  !CheckQEq{yq,0,"y_unary_minus"}
!EndTestCase

!TestCase{left_shifts}
  xq.q=1 LSL 4
  !CheckQEq{xq,16,"x_sixteen"}
  yq.q=1 LSL 16
  !CheckQEq{yq,0,"y_wrapped"}
!EndTestCase

!TestCase{a_quick_left_operand_and_a_sum}
  q=10000 : v2.w=4096 : p=0.5
  zq.q=q LSL 2
  !CheckEq{Peek.l(&zq),$9C400000,"z_wrapped"}
  uq.q=(v2 LSL 4)+p
  !CheckEq{Peek.l(&uq),$7FFFFFFF,"u_saturated"}
!EndTestCase
