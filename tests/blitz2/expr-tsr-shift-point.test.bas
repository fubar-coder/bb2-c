; expr-tsr-shift-point (#1235, type-system-rework) - a constant with a
; point as the shifted operand is a Quick of its value (spec compiler/
; expression-types, "A shift count is a whole number", scenario "A
; constant with a point as the shifted operand"): 1.5 LSL 2 is 6 under a
; Byte and a Long, 1.5 LSR 1 is .75, 2.0 LSL 1 is 4, 1.5 LSL qh with a
; Quick count 2.5 is 6.  The original refuses the constant as the left
; operand of a shift ("Illegal Operator for Type"): its compile result is
; the declared deviation of its cells.
; Correct: the values of the spec's scenarios.
!TestCase{a_constant_with_a_point_as_the_shifted_operand}
  rb.b=1.5 LSL 2
  !CheckEq{rb,6,"rb_byte"}
  rl.l=1.5 LSL 2
  !CheckEq{rl,6,"r_long"}
!EndTestCase

!TestCase{print_a_constant_with_a_point_shifted}
  qh=2.5
  NPrint 1.5 LSR 1
  NPrint 2.0 LSL 1
  NPrint 1.5 LSL qh
!EndTestCase
