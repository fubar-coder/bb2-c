; compiler-quick-literal-wrap (#1070) - a decimal literal is read as an
; FFP value (24 bits of mantissa), so a Quick literal close to 32768
; (32767.9999, 32767.99998) is rounded to 32768.0 and its assignment to a
; Quick wraps to -32768: the sign of the value is lost.
; Correct: the nearest Quick that fits, 32767.99998 ($7FFFFFFF), -32768
; ($80000000) for a negated literal; the literals in range around the
; limits keep their value (32767.998 is the FFP value 32767.998046875;
; -32768.00001 and -32767.9999 round to -32768.0); "--" negates twice;
; "-(32767.9999)" negates the bracket's value 32767.99998.
!TestCase{literal_wrap}
  Dim buf.l(1) : m.l=&buf(0)
  q.q=32767.9999
  Poke.q m,q : i.l=Peek.w(m) : r.l=(q>0) : s$=Hex$(Peek.l(m))
  !CheckEq{i,32767,"a_int"}
  !CheckEq{r,-1,"a_positive"}
  !CheckStrEq{s$,"7FFFFFFF","a_hex"}
  q=32767.99998
  Poke.q m,q : i.l=Peek.w(m) : r.l=(q>0) : s$=Hex$(Peek.l(m))
  !CheckEq{i,32767,"b_int"}
  !CheckEq{r,-1,"b_positive"}
  !CheckStrEq{s$,"7FFFFFFF","b_hex"}
  q=32768.0
  Poke.q m,q : s$=Hex$(Peek.l(m))
  !CheckStrEq{s$,"7FFFFFFF","c_32768_0"}
  q=32767.998
  Poke.q m,q : s$=Hex$(Peek.l(m))
  !CheckStrEq{s$,"7FFFFF80","d_32767_998"}
  q=32767.5
  Poke.q m,q : s$=Hex$(Peek.l(m))
  !CheckStrEq{s$,"7FFF8000","e_32767_5"}
  q=-32768
  Poke.q m,q : s$=Hex$(Peek.l(m))
  !CheckStrEq{s$,"80000000","f_m32768"}
  q=-32768.00001
  Poke.q m,q : s$=Hex$(Peek.l(m))
  !CheckStrEq{s$,"80000000","g_m32768_00001"}
  q=-32767.9999
  Poke.q m,q : s$=Hex$(Peek.l(m))
  !CheckStrEq{s$,"80000000","h_m32767_9999"}
  q=-32767.5
  Poke.q m,q : s$=Hex$(Peek.l(m))
  !CheckStrEq{s$,"80008000","i_m32767_5"}
  q=--32767.9999
  Poke.q m,q : s$=Hex$(Peek.l(m))
  !CheckStrEq{s$,"7FFFFFFF","j_double_negation"}
  q=-(32767.9999)
  Poke.q m,q : s$=Hex$(Peek.l(m))
  !CheckStrEq{s$,"80000001","k_negated_bracket"}
  q=1-32767.9999
  Poke.q m,q : s$=Hex$(Peek.l(m))
  !CheckStrEq{s$,"80010001","l_one_minus"}
!EndTestCase
