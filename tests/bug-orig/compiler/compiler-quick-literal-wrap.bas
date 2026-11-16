; compiler-quick-literal-wrap (#1070) - a decimal literal is read as an
; FFP value (24 bits of mantissa), so a Quick literal close to 32768
; (32767.9999, 32767.99998) is rounded to 32768.0 and its assignment to a
; Quick wraps to -32768: the sign of the value is lost.
; Correct: the nearest Quick that fits, 32767.99998 ($7FFFFFFF), -32768
; ($80000000) for a negated literal; the literals in range around the
; limits keep their value (32767.998 is the FFP value 32767.998046875;
; -32768.00001 and -32767.9999 round to -32768.0); "--" negates twice;
; "-(32767.9999)" negates the bracket's value 32767.99998.
; @fixed #1070
Dim buf.l(1) : m.l=&buf(0)
q.q=32767.9999
Poke.q m,q
NPrint "32767.9999: int ",Peek.w(m)," positive ",q>0," ",Hex$(Peek.l(m))
q=32767.99998
Poke.q m,q
NPrint "32767.99998: int ",Peek.w(m)," positive ",q>0," ",Hex$(Peek.l(m))
q=32768.0
Poke.q m,q : NPrint "32768.0: ",Hex$(Peek.l(m))
q=32767.998
Poke.q m,q : NPrint "32767.998: ",Hex$(Peek.l(m))
q=32767.5
Poke.q m,q : NPrint "32767.5: ",Hex$(Peek.l(m))
q=-32768
Poke.q m,q : NPrint "-32768: ",Hex$(Peek.l(m))
q=-32768.00001
Poke.q m,q : NPrint "-32768.00001: ",Hex$(Peek.l(m))
q=-32767.9999
Poke.q m,q : NPrint "-32767.9999: ",Hex$(Peek.l(m))
q=-32767.5
Poke.q m,q : NPrint "-32767.5: ",Hex$(Peek.l(m))
q=--32767.9999
Poke.q m,q : NPrint "--32767.9999: ",Hex$(Peek.l(m))
q=-(32767.9999)
Poke.q m,q : NPrint "-(32767.9999): ",Hex$(Peek.l(m))
q=1-32767.9999
Poke.q m,q : NPrint "1-32767.9999: ",Hex$(Peek.l(m))
End
