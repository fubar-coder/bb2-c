; compiler-quick-literal-data (#1070) - a decimal literal is read as an
; FFP value (24 bits of mantissa), so a Data.q value close to 32768
; (32767.9999, -32767.9999, -32768.00001) is rounded to 32768.0 first;
; its integer part 32768 does not fit a Quick, the value becomes a Float,
; and Data.q refuses it as bad data.
; Correct: the nearest Quick that fits, 32767.99998 ($7FFFFFFF), and
; -32768 ($80000000) for the negative values; the values in range keep
; theirs.
; @fixed #1070
Dim buf.l(1) : m.l=&buf(0)
For i=1 To 4
  Read q.q
  Poke.q m,q : NPrint Hex$(Peek.l(m))
Next
End
Data.q 32767.9999,-32767.9999,-32768.00001,32767.5
