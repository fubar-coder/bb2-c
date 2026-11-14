; floatquicklib-zero-negative (#146) - the conversion of a Quick to a
; Float tests the sign on the negated value (NEG.l d0:BMI): 0 stays 0 and
; takes the path of the negative numbers, so Quick 0 becomes the FFP
; number $00000080, a "-0".
; Correct: Quick 0 gives the Float 0 ($00000000); other values as before.
; @fixed #146
; @configs clib
q.q=0 : f.f=q : NPrint "a ",Hex$(Peek.l(&f))
q=1.5 : f=q : NPrint "b ",Hex$(Peek.l(&f))
q=-1.5 : f=q : NPrint "c ",Hex$(Peek.l(&f))
End
