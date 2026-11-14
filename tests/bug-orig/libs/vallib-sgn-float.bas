; vallib-sgn-float (#301) - Sgn of a float calls mathffp's SPTst, which
; tests d1, not d0 - and d1 holds the type word (5) of the argument: Sgn
; of every float is 1.
; Correct: -1, 0 and 1 for a negative float, 0 and a positive float.
; @fixed #301
; @configs clib
f.f=-2.5 : NPrint "a ",Sgn(f)
f=0 : NPrint "b ",Sgn(f)
f=2.5 : NPrint "c ",Sgn(f)
f=-0.001 : NPrint "d ",Sgn(f)
End
