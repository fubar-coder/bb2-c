; intrinsic-int - Int of a Quick in AB3 mode (change compiler-intrinsics,
; spec compiler/intrinsics): the integer part truncated towards zero,
; compiled inline by blitz2-ab3 ("extra calls 0": the JSRs of the
; statement minus those of the same statement without Int); AmiBlitz3
; calls its library (intrinsic-int.test.toml: its count), with the same
; values
Function.l jsrs{a.l,b.l}
  n.l = 0
  While a < b
    If Peek.w(a) = $4EB9
      n = n + 1 : a = a + 4
    EndIf
    a = a + 2
  Wend
  Function Return n
End Function
!TestCase{intrinsic_int}
  c.l = 0 : q.q = 0
  q = 3.5 : c = Int(q)
  !CheckEq{c,3,"l1"}
  q = -3.5 : c = Int(q)
  !CheckEq{c,-3,"l2"}
  q = 2.5 : c = Int(q)
  !CheckEq{c,2,"l3"}
  q = -2.5 : c = Int(q)
  !CheckEq{c,-2,"l4"}
  q = 1.5 : c = Int(q)
  !CheckEq{c,1,"l5"}
  q = -1.75 : c = Int(q)
  !CheckEq{c,-1,"l6"}
  q = -0.5 : c = Int(q)
  !CheckEq{c,0,"l7"}
  q = -32768 : c = Int(q)
  !CheckEq{c,-32768,"l8"}
  q = -1.75
.a3
  c = q
.a4
.a1
  c = Int(q)
.a2
  k.l=jsrs{?a1, ?a2} - jsrs{?a3, ?a4} : zs$=Str$(c)+" extra calls "+Str$(k)
  !CheckStrEq{zs$,"-1 extra calls 0","l9"}
!EndTestCase
