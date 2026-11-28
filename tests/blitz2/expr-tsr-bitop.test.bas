; expr-tsr-bitop (#1235, type-system-rework) - bit operators take whole
; operands (spec compiler/expression-types, "Bit operators take whole
; operands"): AND, OR, EOR, NOT, & and | round a Quick or 48.16 operand
; down to a whole number and give a whole result; a Float operand stays
; refused with "Illegal Operator for Type" (the data source
; expr-tsr-bitop.test/float.bas, `r.l=l AND f`).
; The original's values, where they differ, are the declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{print_quick_operand}
  q=7.5
  NPrint q AND 3
!EndTestCase

!TestCase{not_of_a_quick}
  h=-1
  h=Not h
  !CheckQEq{h,0,"h_not"}
!EndTestCase
