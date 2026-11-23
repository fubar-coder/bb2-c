; compiler-pow-fraction-exponent (#1189) - ^ with a negative base and an
; exponent with a fraction gives the magnitude |x|^y (mathtrans SPPow), as
; the original: there is no real power, and whether this becomes a
; runtime error is open in #1189.  Also an exponent below 1 and a Quick
; exponent with a fraction.
; A regression test of the fix of #1189, which sets the sign only for a
; whole-number exponent: these values must not change.

!TestCase{pow_fraction_exponent}
  vm.w=-3 : vq.q=-4 : vf.f=-2.5
  ef.f=3.5 : eq.q=0.5 : eh.f=-0.5 : ex.f=0.25
  rf.f=0 : rl.l=0
  rl=(-3)^3.5
  !CheckEq{rl,46,"long_negative_literal_base"}
  rf=vm^ef
  !CheckFltNear{rf,46.76537,0.0005,"word_base_float_exponent"}
  rf=vq^eq
  !CheckFltNear{rf,2,0.0005,"quick_base_quick_exponent"}
  rf=vf^eh
  !CheckFltNear{rf,0.6324556,0.0005,"float_base_negative_exponent"}
  rf=vf^ex
  !CheckFltNear{rf,1.257433,0.0005,"float_base_quarter_exponent"}
  rf=vm^1.5
  !CheckFltNear{rf,5.196152,0.0005,"word_base_literal_exponent"}
!EndTestCase
