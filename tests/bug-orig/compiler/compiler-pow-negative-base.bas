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
; @fixed #1189
vm.w=-2 : vf.f=-2.5 : vo.w=-1 : vh.f=-0.5
nw.w=3 : ef.f=3 : eq.q=3 : nl.l=0
rf.f=0 : rl.l=0
rf=vm^2 : NPrint "01 vm^2 ",rf
rf=vm^3 : NPrint "02 vm^3 ",rf
rf=vm^-1 : NPrint "03 vm^-1 ",rf
rf=vm^-2 : NPrint "04 vm^-2 ",rf
rf=vm^-3 : NPrint "05 vm^-3 ",rf
rf=vf^ef : NPrint "06 vf^ef(.f 3) ",rf
rf=vf^eq : NPrint "07 vf^eq(.q 3) ",rf
rf=vf^nw : NPrint "08 vf^nw ",rf
rl=vf^nw : NPrint "09 rl.l=vf^nw ",rl
rf=vm^0 : NPrint "10 vm^0 ",rf
rf=vm^1 : NPrint "11 vm^1 ",rf
rf=vh^3 : NPrint "12 vh^3 ",rf
nl=16777215 : rf=vo^nl : NPrint "13 vo^16777215 ",rf
nl=16777216 : rf=vo^nl : NPrint "14 vo^16777216 ",rf
nl=8388609 : rf=vo^nl : NPrint "15 vo^8388609 ",rf
rf=(-1.5)^3 : NPrint "16 (-1.5)^3 ",rf
rf=-3^nw : NPrint "17 -3^nw ",rf
rf=2.5^nw : NPrint "18 2.5^nw ",rf
rf=-(2.5^nw) : NPrint "19 -(2.5^nw) ",rf
End
