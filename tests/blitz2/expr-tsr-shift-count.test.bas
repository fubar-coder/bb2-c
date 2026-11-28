; expr-tsr-shift-count (#1235, type-system-rework) - a shift count is a
; whole number (spec compiler/expression-types, "A shift count is a whole
; number", modified): the count of LSL, ASL, LSR and ASR is a constant
; without a point or a variable or expression of a Byte, Word, Long or
; Quick type; a Quick count is its whole part, rounded down; a constant
; with a point as a count is refused with "Illegal Operator for Type"
; whatever its value (the data sources of expr-tsr-shift-count.test/: a
; Byte, a Quick and a Word shifted by 2.0 or 2.5).  A constant with a point
; as the shifted operand is expr-tsr-shift-point.
; The original's values, where they differ, are the declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{print_quick_variables_as_count}
  x.w=3 : q2=2.0 : qh=2.5 : qn=2.99
  NPrint x LSL q2
  NPrint x LSL qh
  NPrint x LSL qn
!EndTestCase

!TestCase{print_a_quick_count_beside_a_whole_operand}
  x.w=3 : qh=2.5
  NPrint x LSR qh
!EndTestCase

!TestCase{a_quick_count_beside_a_quick_operand}
  q=1.5 : qh=2.5
  rl.l=q LSL qh
  !CheckEq{rl,6,"rl_long"}
!EndTestCase
