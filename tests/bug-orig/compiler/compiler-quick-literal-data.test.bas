; compiler-quick-literal-data (#1070) - a decimal literal is read as an
; FFP value (24 bits of mantissa), so a Data.q value close to 32768
; (32767.9999, -32767.9999, -32768.00001) is rounded to 32768.0 first;
; its integer part 32768 does not fit a Quick, the value becomes a Float,
; and Data.q refuses it as bad data.
; Correct: the nearest Quick that fits, 32767.99998 ($7FFFFFFF), and
; -32768 ($80000000) for the negative values; the values in range keep
; theirs.
.quick_data
  Data.q 32767.9999,-32767.9999,-32768.00001,32767.5

!TestCase{literal_data}
  Dim buf.l(1) : m.l=&buf(0)
  Restore quick_data
  Read q.q
  Poke.q m,q : s$=Hex$(Peek.l(m))
  !CheckStrEq{s$,"7FFFFFFF","d_32767_9999"}
  Read q.q
  Poke.q m,q : s$=Hex$(Peek.l(m))
  !CheckStrEq{s$,"80000000","d_m32767_9999"}
  Read q.q
  Poke.q m,q : s$=Hex$(Peek.l(m))
  !CheckStrEq{s$,"80000000","d_m32768_00001"}
  Read q.q
  Poke.q m,q : s$=Hex$(Peek.l(m))
  !CheckStrEq{s$,"7FFF8000","d_32767_5"}
!EndTestCase
