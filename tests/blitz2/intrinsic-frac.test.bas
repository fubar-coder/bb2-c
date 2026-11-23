; intrinsic-frac - Frac of a Quick assigned to a Quick is compiled inline
; (compiler-intrinsics, openspec/specs/compiler/intrinsics): the exact
; fraction (the low word of the magnitude, with the sign of the value),
; with no call (no qutofl, no Frac, no fltoqu: "calls 0"), equal to the
; library's Float converted back (f=Frac(q1) : q3=f, #1064).  Without and
; with runtime error checking (the axis runtime-errors of
; intrinsic-frac.test.toml), with the same values.
; jsrs{a,b}: the JSR instructions ($4EB9, every library call) of the code
; from label a to label b
Function.l jsrs{a.l,b.l}
  n.l=0
  While a<b
    If Peek.w(a)=$4EB9
      n=n+1 : a=a+4
    EndIf
    a=a+2
  Wend
  Function Return n
End Function

!TestBatchInit
  Dim buf.l(2) : m.l=&buf(0)
  q1.q=0 : q2.q=0 : q3.q=0 : f.f=0
!EndTestBatchInit

; checkfrac{bits,want}: Frac of the Quick with these bits, inline and
; through the library, both the bits want
Statement checkfrac{bits.l,want.l}
  SHARED m,q1,q2,q3,f
  Poke.l m,bits : q1=Peek.q(m)
  q2=Frac(q1)
  f=Frac(q1) : q3=f
  Poke.q m,q2 : a.l=Peek.l(m)
  Poke.q m,q3 : b.l=Peek.l(m)
  !CheckEq{a,want,"inline"}
  !CheckEq{b,want,"lib"}
End Statement

; the values of the spec: 1.25, -1.25, .5, -.5, 3.0, -3.0, 0, 30000.0001,
; -30000.0001 (30000 and 7/65536), 255.00002 (255 and 1/65536), and the
; ends of the range
!TestCase{values}
  checkfrac{$00014000,$00004000} : checkfrac{$FFFEC000,$FFFFC000}
  checkfrac{$00008000,$00008000} : checkfrac{$FFFF8000,$FFFF8000}
  checkfrac{$00030000,0} : checkfrac{$FFFD0000,0} : checkfrac{0,0}
  checkfrac{$75300007,7} : checkfrac{-$75300007,$FFFFFFF9} : checkfrac{$00FF0001,1}
  checkfrac{$7FFFFFFF,$0000FFFF} : checkfrac{$80000000,0}
  checkfrac{$80000001,$FFFF0001} : checkfrac{$FFFFFFFF,$FFFFFFFF}
!EndTestCase

; the forms: a variable, an element as the argument and as the target
; (a comment behind), an expression, a statement behind.  "extra calls":
; the JSRs of the statement minus those of the same statement without
; Frac (the index checks with runtime error checking)
!TestCase{forms}
  q1=-1.25
.a1
  q2=Frac(q1)
.a2
  !CheckQEq{q2,-.25,"variable"}
  !CheckEq{jsrs{?a1,?a2},0,"variable_calls"}
  Dim qa.q(3) : qa(1)=7.75
.b3
  q2=qa(1)
.b4
.b1
  q2=Frac(qa(1))
.b2
  !CheckQEq{q2,.75,"element"}
  !CheckEq{jsrs{?b1,?b2}-jsrs{?b3,?b4},0,"element_extra_calls"}
.c3
  qa(2)=q1 ; a comment ends the statement
.c4
.c1
  qa(2)=Frac(q1) ; a comment ends the statement
.c2
  !CheckQEq{qa(2),-.25,"target"}
  !CheckEq{jsrs{?c1,?c2}-jsrs{?c3,?c4},0,"target_extra_calls"}
.d3
  q2=q1+qa(1) : q3=0
.d4
.d1
  q2=Frac(q1+qa(1)) : q3=0
.d2
  !CheckQEq{q2,.5,"expression"}
  !CheckEq{jsrs{?d1,?d2}-jsrs{?d3,?d4},0,"expression_extra_calls"}
!EndTestCase

; the sweep against the library: integer parts at fraction words in steps
; of 251 and the ends
!TestCase{sweep_library}
  n.l=0 : bad.l=0
  Restore ips
  For i.l=1 To 11
    Read ip.l
    For k.l=-2 To 261
      fw.l=k*251
      If k=-2 Then fw=1
      If k=-1 Then fw=$FFFF
      bits.l=ip*65536+fw
      Poke.l m,bits : q1=Peek.q(m)
      q2=Frac(q1)
      f=Frac(q1) : q3=f
      n=n+1
      If q2<>q3
        If bad=0 Then NPrint "first wrong: bits ",Hex$(bits)
        bad=bad+1
      EndIf
    Next
  Next
  !CheckEq{n,2904,"values"}
  !CheckEq{bad,0,"wrong"}
!EndTestCase

; the sweep against the definition: every fraction word of 0, -1, 255 and
; -32768 (its magnitude's low word, the sign restored)
!TestCase{sweep_definition}
  n.l=0 : bad.l=0
  Restore ips2
  For i.l=1 To 4
    Read ip.l
    For fw.l=0 To $FFFF
      bits.l=ip*65536+fw
      Poke.l m,bits : q1=Peek.q(m)
      If bits<0
        e.l=-((-bits)&$FFFF)
      Else
        e.l=bits&$FFFF
      EndIf
      q2=Frac(q1)
      Poke.q m,q2
      n=n+1
      If Peek.l(m)<>e
        If bad=0 Then NPrint "first wrong: bits ",Hex$(bits)
        bad=bad+1
      EndIf
    Next
  Next
  !CheckEq{n,262144,"values"}
  !CheckEq{bad,0,"wrong"}
!EndTestCase

.ips
Data.l 0,1,-1,255,-256,300,-300,30000,-30000,32767,-32768
.ips2
Data.l 0,-1,255,-32768
