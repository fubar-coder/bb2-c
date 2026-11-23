; compiler-pow-negative-base (#1189) - ^ with a negative base drops the
; sign of the power: the compiler computes x^y in Float with mathtrans
; SPPow (mathtranslib's first entry, inline jsr -90(a6)), which gives
; |x|^y, also for a whole-number odd exponent: (-2)^3 gives 8, (-2)^-1
; 0.5.  The cases the expression matrix lacks: an even exponent, a
; negative odd exponent, a whole-valued Float and Quick exponent, an
; exponent of 0 and 1, the odd exponent 16777215 (the largest whole odd
; Float: no fraction bits left) and the even one 16777216 above it, a
; constant expression and a Long destination.
; Correct: the power's magnitude as before (SPPow's), negative when the
; exponent is a whole odd number and positive when it is a whole even one
; (the manual: "LHS to the power of RHS").  A negative base with a
; fractional exponent is tests/bug-orig/regression/compiler-pow-fraction-exponent.
!TestCase{pow_negative_base}
  vm.w=-2 : vf.f=-2.5 : vo.w=-1 : vh.f=-0.5
  nw.w=3 : exf.f=3 : exq.q=3 : nl.l=0
  rf.f=0 : rl.l=0
  rf=vm^2 : !CheckFltNear{rf,4,0.001,"p01_vm_2"}
  rf=vm^3 : !CheckFltNear{rf,-8,0.001,"p02_vm_3"}
  rf=vm^-1 : !CheckFltNear{rf,-0.5,0.001,"p03_vm_m1"}
  rf=vm^-2 : !CheckFltNear{rf,0.25,0.001,"p04_vm_m2"}
  rf=vm^-3 : !CheckFltNear{rf,-0.125,0.001,"p05_vm_m3"}
  rf=vf^exf : !CheckFltNear{rf,-15.625,0.001,"p06_vf_float3"}
  rf=vf^exq : !CheckFltNear{rf,-15.625,0.001,"p07_vf_quick3"}
  rf=vf^nw : !CheckFltNear{rf,-15.625,0.001,"p08_vf_word3"}
  rl=vf^nw : !CheckEq{rl,-15,"p09_long_destination"}
  rf=vm^0 : !CheckFltNear{rf,1,0.001,"p10_vm_0"}
  rf=vm^1 : !CheckFltNear{rf,-2,0.001,"p11_vm_1"}
  rf=vh^3 : !CheckFltNear{rf,-0.125,0.001,"p12_vh_3"}
  nl=16777215 : rf=vo^nl : !CheckFltNear{rf,-1,0.001,"p13_odd_16777215"}
  nl=16777216 : rf=vo^nl : !CheckFltNear{rf,1,0.001,"p14_even_16777216"}
  nl=8388609 : rf=vo^nl : !CheckFltNear{rf,-1,0.001,"p15_odd_8388609"}
  rf=(-1.5)^3 : !CheckFltNear{rf,-3.375,0.001,"p16_constant"}
  rf=-3^nw : !CheckFltNear{rf,-27,0.001,"p17_unary_minus"}
  rf=2.5^nw : !CheckFltNear{rf,15.625,0.001,"p18_positive_base"}
  rf=-(2.5^nw) : !CheckFltNear{rf,-15.625,0.001,"p19_negated_power"}
!EndTestCase
