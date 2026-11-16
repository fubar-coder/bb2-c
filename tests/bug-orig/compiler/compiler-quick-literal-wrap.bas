; compiler-quick-literal-wrap (#1070) - a decimal literal is read as an
; FFP value (24 bits of mantissa), so a Quick literal close to 32768
; (32767.9999, 32767.99998) is rounded to 32768.0 and its assignment to a
; Quick wraps to -32768: the sign of the value is lost.
; Correct: the Quick holds 32767 and a fraction (its upper word 32767,
; the value positive).
Dim buf.l(1) : m.l=&buf(0)
q.q=32767.9999
Poke.q m,q
NPrint "32767.9999: int ",Peek.w(m)," positive ",q>0
q=32767.99998
Poke.q m,q
NPrint "32767.99998: int ",Peek.w(m)," positive ",q>0
End
