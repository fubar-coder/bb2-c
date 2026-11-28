; expr-tsr-float-top (#1235, type-system-rework) - the top operation under
; a Float destination runs in Float (spec compiler/expression-types, "The
; top operation under a Float destination runs in Float"): the arithmetic
; operation (+ - * /, Mod, ^) that is the whole expression is computed in
; Float when one of its operands is a Quick; its operands are converted to
; Float first, and a constant with a point there is read directly as the
; Float of its written value, not first as a Quick.  A comparison and a
; unary minus at the top are not such an operation (the answer D2): the
; comparison gives -1 and the unary minus negates the 48.16 quotient.
; The original's values (1.95: x is 0, y is -0.20875) are the declared
; deviations.
; Correct: the values of the spec's scenarios.
!TestCase{quick_operands}
  q=20.875 : q2=92 : q3=17.5 : q4=113.5
  xf.f=q/100
  !CheckFltNear{xf,0.20875,0.000001,"x_quotient"}
  yf.f=q2*0.002
  !CheckFltNear{yf,0.184,0.000001,"y_product"}
  zf.f=q3/q4
  !CheckFltNear{zf,0.154185,0.000001,"z_quotient"}
!EndTestCase

!TestCase{comparison_and_unary_minus}
  q=0.1 : q2=20.875
  xf.f=(q=0.1)
  !CheckFltNear{xf,-1,0.000001,"x_comparison"}
  yf.f=-(q2/100)
  !CheckFltNear{yf,-0.2087402,0.000001,"y_unary_minus"}
!EndTestCase
