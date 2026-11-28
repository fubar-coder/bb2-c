; expr-tsr-float (#1235, type-system-rework) - an operation with a Float
; operand is computed in Float (spec compiler/expression-types, "An
; operation with a Float is computed in Float"): the other operand is
; converted to Float (a Quick or 48.16 value with its fraction); a Quick
; quotient below the top, before a Float, is the 48.16 quotient (cut down
; to 1/65536) converted to Float, not the Float quotient.
; The original gives the Float quotient (0.20875); its value is the
; declared deviation.
; Correct: the values of the spec's scenarios.
!TestCase{quick_and_float}
  q=1.5 : f.f=0.5
  rl.l=q+f
  !CheckEq{rl,2,"r_long"}
!EndTestCase

!TestCase{quotient_before_float}
  q=20.875 : f.f=0
  xf.f=q/100+f
  !CheckFltNear{xf,0.2087402,0.000001,"x_float"}
!EndTestCase
