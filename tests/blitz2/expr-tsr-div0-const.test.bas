; expr-tsr-div0-const (#1235, type-system-rework) - a constant divisor 0
; stays the compile error "Division by Zero" (spec compiler/expression-
; types, "A division or Mod by a run-time 0 raises the division trap",
; scenario "A constant divisor 0"): the data sources of
; expr-tsr-div0-const.test/ each divide by a constant 0 (a Long, a Quick
; and a Float destination, a Mod); each compile ends with the error at the
; statement's line (asserted in the configuration).  The case here divides
; by constants that are not 0.
; The original gives the same error.
; Correct: the case gives the values of its checks.
!TestCase{nonzero_constants}
  rl.l=7/1
  !CheckEq{rl,7,"r_long"}
  r2l.l=7 Mod 4
  !CheckEq{r2l,3,"r2_mod"}
!EndTestCase
