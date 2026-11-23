; intrinsic-frac-lib - Frac where it is not an intrinsic (compiler-
; intrinsics, openspec/specs/compiler/intrinsics): outside the whole right
; side of an assignment to a Quick, or with an argument that is not a
; Quick, Frac is the library's Float as before ("calls": the JSRs of the
; statement: the call of Frac (with runtime error checking also of
; intchk), and of the conversions, NPrint and the index check around
; it).  A Float result is checked by its bits.  The cases: a Float
; target, a Long target, a Float argument, an operand (*2.5, 1+, a unary
; minus), a parenthesis, the argument of Poke.q, a condition, NPrint, an
; element as the argument of an operand.  The counts and values are those
; of the build before the intrinsics.  Without and with runtime error
; checking (the axis runtime-errors of intrinsic-frac-lib.test.toml).
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
; runerrs{a,b}: 1 if the code from label a to label b holds a trap #1
; ($4E41, the debugger's step in front of every statement): the program
; is compiled with runtime error checking
Function.l runerrs{a.l,b.l}
  While a<b
    If Peek.w(a)=$4E41 Then Function Return 1
    a=a+2
  Wend
  Function Return 0
End Function
; rv{a,b}: a without runtime error checking, b with it
Function.l rv{a.l,b.l}
  Shared re.l
  If re Then Function Return b
  Function Return a
End Function

!TestBatchInit
.re1
  re.l=0
.re2
  re=runerrs{?re1,?re2}
!EndTestBatchInit

!TestCase{calls}
  Dim buf.l(2) : m.l=&buf(0)
  q1.q=-1.75 : q2.q=0 : q3.q=0.5 : f.f=0 : g.f=-2.375 : c.l=0
  Dim qa.q(3) : qa(1)=7.75
.a1
  f=Frac(q1)
.a2
  Poke.f m,f
  !CheckEq{Peek.l(m),$C00000C0,"float_target"}
  !CheckEq{jsrs{?a1,?a2},rv{1,2},"float_target_calls"}
.b1
  c=Frac(q1)
.b2
  !CheckEq{c,0,"long_target"}
  !CheckEq{jsrs{?b1,?b2},rv{1,2},"long_target_calls"}
.c1
  q2=Frac(g)
.c2
  !CheckQEq{q2,-.375,"float_argument"}
  !CheckEq{jsrs{?c1,?c2},rv{2,3},"float_argument_calls"}
.d1
  q2=Frac(q1)*2.5
.d2
  !CheckQEq{q2,-1.875,"times"}
  !CheckEq{jsrs{?d1,?d2},rv{2,3},"times_calls"}
.e1
  q2=1+Frac(q1)
.e2
  !CheckQEq{q2,.25,"plus"}
  !CheckEq{jsrs{?e1,?e2},rv{3,4},"plus_calls"}
.f1
  q2=(Frac(q1))
.f2
  !CheckQEq{q2,-.75,"parenthesis"}
  !CheckEq{jsrs{?f1,?f2},rv{2,3},"parenthesis_calls"}
.g1
  q2=-Frac(q1)
.g2
  !CheckQEq{q2,.75,"minus"}
  !CheckEq{jsrs{?g1,?g2},rv{2,3},"minus_calls"}
.h1
  Poke.q m,Frac(q1)
.h2
  !CheckEq{Peek.l(m),$FFFF4000,"poke"}
  !CheckEq{jsrs{?h1,?h2},rv{3,4},"poke_calls"}
.i1
  If Frac(q1)<q3 Then c=1 Else c=0
.i2
  !CheckEq{c,1,"condition"}
  !CheckEq{jsrs{?i1,?i2},rv{2,3},"condition_calls"}
.j1
  NPrint Frac(q1)
.j2
  !CheckEq{jsrs{?j1,?j2},rv{2,4},"nprint_calls"}
.k1
  q2=Frac(qa(1))*2
.k2
  !CheckQEq{q2,1.5,"element"}
  !CheckEq{jsrs{?k1,?k2},rv{2,5},"element_calls"}
!EndTestCase
