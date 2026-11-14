; lmullib-cross-swap (#199) - the Long and Quick multiplication of the
; 68000 (lmullib) adds the partial products SWAPped instead of shifted by
; 16 bits: when the product does not fit in 32 bits, the high word of a
; partial product also lands in the low word of the result, instead of
; the product being cut to 32 bits.
; Correct: the product modulo 2^32: 196608*30000 = 1603272704,
; -196608*30000 = -1603272704, the Quick 300*300 = $5F900000.
; @configs clib
a.l=196608 : b.l=30000
NPrint "a ",a*b
a=-196608 : NPrint "b ",a*b
c.q=300 : d.q=300 : e.q=c*d : NPrint "c ",Hex$(Peek.l(&e))
c=-300 : e=c*d : NPrint "d ",Hex$(Peek.l(&e))
End
