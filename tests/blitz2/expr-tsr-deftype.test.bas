; expr-tsr-deftype (#1235, type-system-rework) - DEFTYPE has no influence
; on a whole constant (spec compiler/expression-types, "A whole constant
; takes its smallest type", "DEFTYPE has no influence"): with DEFTYPE .f
; the constants 7 and 2 are still whole, so 7/2 is 3 under a Long, and
; (7/2)*2 is 6.  A file of its own: DEFTYPE holds for the whole program.
; The original and the port agree (these values already hold), so the test
; has no declared deviation.
; Correct: the values of the spec's scenario.
DEFTYPE .f

!TestCase{deftype_float}
  xl.l=7/2
  !CheckEq{xl,3,"x_long"}
  zl.l=(7/2)*2
  !CheckEq{zl,6,"z_long"}
!EndTestCase

; no destination: the printed value of the whole division
!TestCase{print_deftype_float}
  NPrint 7/2
!EndTestCase
