; floatquicklib-min-positive (#147) - the conversion of a Quick to a
; Float tests the sign on the negated value (NEG.l d0:BMI); NEG of
; $80000000 overflows and stays negative, so the smallest Quick, -32768.0,
; is converted as the positive magnitude: +32768.0.
; Correct: -32768.0 gives the Float -32768.0 ($800000D0).
; @fixed #147
; @configs clib
q.q=-32768 : f.f=q : NPrint "a ",f," ",Hex$(Peek.l(&f))
q=-32767 : f=q : NPrint "b ",f," ",Hex$(Peek.l(&f))
End
