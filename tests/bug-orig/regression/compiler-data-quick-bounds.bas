; compiler-data-quick-bounds (#1074) - the Data.q values next to -32768,
; which the original already reads right: Data reads the '-' itself, and
; -32767, -1, 0 and 32767 fit a Quick as integers; -32768 in an
; expression is the long 32768, negated and converted afterwards, also
; when the expression goes on computing with the long.
; Correct: $80010000, $FFFF0000, 0, $7FFF0000; q=-32768 is $80000000,
; q=-32768*4/8 is -16384 ($C0000000).
; A regression test of the fix of #1074: the original already gives this (correct) result.
Dim buf.l(1) : m.l=&buf(0)
For i=1 To 4
  Read q.q
  Poke.q m,q : NPrint Hex$(Peek.l(m))
Next
q=-32768 : Poke.q m,q : NPrint Hex$(Peek.l(m))
q=-32768*4/8 : Poke.q m,q : NPrint Hex$(Peek.l(m))
End
Data.q -32767,-1,0,32767
