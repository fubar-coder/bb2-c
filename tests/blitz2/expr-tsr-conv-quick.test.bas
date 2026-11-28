; expr-tsr-conv-quick (#1235, type-system-rework) - the conversion to a
; Quick rounds down and saturates (spec compiler/expression-types, "The
; conversion to a Quick rounds down and saturates"): a whole, 48.16 or
; Float value converts to a Quick rounded down to 1/65536 and gives
; $7FFFFFFF above the Quick range and $80000000 below it.
; The original wraps the whole and the 48.16 values and rounds a negative
; Float quotient towards zero; its values are the declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{saturation}
  b.b=3 : q=1.5 : l.l=100000
  xq.q=b+100000
  !CheckEq{Peek.l(&xq),$7FFFFFFF,"x_whole_sum"}
  yq.q=q*40000
  !CheckEq{Peek.l(&yq),$7FFFFFFF,"y_product"}
  zq.q=l
  !CheckEq{Peek.l(&zq),$7FFFFFFF,"z_long"}
!EndTestCase

!TestCase{a_float_rounded_down}
  f.f=-144
  xq.q=f/10
  !CheckEq{Peek.l(&xq),$FFF19999,"x_float_quotient"}
!EndTestCase
