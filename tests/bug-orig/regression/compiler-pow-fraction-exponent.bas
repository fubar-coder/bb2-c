; compiler-pow-fraction-exponent (#1189) - ^ with a negative base and an
; exponent with a fraction gives the magnitude |x|^y (mathtrans SPPow), as
; the original: there is no real power, and whether this becomes a
; runtime error is open in #1189.  Also an exponent below 1 and a Quick
; exponent with a fraction.
; A regression test of the fix of #1189, which sets the sign only for a
; whole-number exponent: these values must not change.
vm.w=-3 : vq.q=-4 : vf.f=-2.5
ef.f=3.5 : eq.q=0.5 : eh.f=-0.5 : ex.f=0.25
rf.f=0 : rl.l=0
rl=(-3)^3.5 : NPrint "01 rl.l=(-3)^3.5 ",rl
rf=vm^ef : NPrint "02 vm^ef(3.5) ",rf
rf=vq^eq : NPrint "03 vq^eq(.q 0.5) ",rf
rf=vf^eh : NPrint "04 vf^eh(-0.5) ",rf
rf=vf^ex : NPrint "05 vf^ex(0.25) ",rf
rf=vm^1.5 : NPrint "06 vm^1.5 ",rf
End
