; expr-tsr-fold (#1235, type-system-rework; #1225, #1229, #1233) - a
; constant expression gives the value of its variable form (spec compiler/
; expression-types, "A constant expression gives the value of its variable
; form"): folding gives for every operator, operand order and destination
; the value of the same expression with each constant replaced by a
; variable of the constant's type (the whole matrix: expr-matrix).  Each
; case checks the constant form and the variable form of the scenario.
; The original's values (the folded values), where they differ, are the
; declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{division_1225}
  q=1.25
  xl.l=1-1.25/3
  !CheckEq{xl,0,"x_constants"}
  yl.l=1-q/3
  !CheckEq{yl,0,"y_variable"}
!EndTestCase

!TestCase{mod_over_a_division_1229}
  a.w=7 : b.w=-3 : c.w=100 : d.w=3
  xl.l=(7/-3) Mod 3
  !CheckEq{xl,1,"x_constants"}
  yl.l=100 Mod (7/3)
  !CheckEq{yl,0,"y_constants"}
  x2l.l=(a/b) Mod d
  !CheckEq{x2l,1,"x2_variables"}
  y2l.l=c Mod (a/d)
  !CheckEq{y2l,0,"y2_variables"}
!EndTestCase

!TestCase{print_shifts_1233}
  qh=0.5 : bn.b=-8 : b40.b=40
  NPrint (-8 ASR 40)+qh
  NPrint (bn ASR b40)+qh
!EndTestCase
