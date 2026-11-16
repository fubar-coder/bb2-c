; compiler-data-quick-min (#1074) - the smallest Quick as a Data.q value: Data
; reads the '-' itself and fetches the integer 32768, which does not fit
; a Quick; its type becomes a long, and Data.q refuses it as bad data
; (consttype2 .qint).  In an expression, q=-32768, the value is right.
; Correct: Data.q -32768 is the Quick -32768 ($80000000).
Dim buf.l(1) : m.l=&buf(0)
Read q.q
Poke.q m,q : NPrint Hex$(Peek.l(m))
End
Data.q -32768
