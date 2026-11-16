; @runerrs on
; intrinsic-frac-lib-runerrs - Frac where it is not an intrinsic (compiler-
; intrinsics, openspec/specs/compiler/intrinsics): outside the whole right
; side of an assignment to a Quick, or with an argument that is not a
; Quick, Frac is the library's Float as before ("calls": the JSRs of the
; statement: the call of Frac (with runtime error checking also of
; intchk), and of the conversions, NPrint and the index check around
; it).  A Float result is printed with its bits.  The cases: a Float
; target, a Long target, a Float argument, an operand (*2.5, 1+, a unary
; minus), a parenthesis, the argument of Poke.q, a condition, NPrint, an
; element as the argument of an operand.  The counts and values are those
; of the build before the intrinsics.
; The program of intrinsic-frac-lib.bas with runtime error checking (the
; error check intchk of Frac is one call more).
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
Dim buf.l(2) : m.l=&buf(0)
q1.q=-1.75 : q2.q=0 : q3.q=0.5 : f.f=0 : g.f=-2.375 : c.l=0
Dim qa.q(3) : qa(1)=7.75
.a1
f=Frac(q1)
.a2
Poke.f m,f
NPrint "f=Frac(q1) ",Hex$(Peek.l(m))," calls ",jsrs{?a1,?a2}
.b1
c=Frac(q1)
.b2
NPrint "c=Frac(q1) ",c," calls ",jsrs{?b1,?b2}
.c1
q2=Frac(g)
.c2
NPrint "q2=Frac(g) ",q2," calls ",jsrs{?c1,?c2}
.d1
q2=Frac(q1)*2.5
.d2
NPrint "q2=Frac(q1)*2.5 ",q2," calls ",jsrs{?d1,?d2}
.e1
q2=1+Frac(q1)
.e2
NPrint "q2=1+Frac(q1) ",q2," calls ",jsrs{?e1,?e2}
.f1
q2=(Frac(q1))
.f2
NPrint "q2=(Frac(q1)) ",q2," calls ",jsrs{?f1,?f2}
.g1
q2=-Frac(q1)
.g2
NPrint "q2=-Frac(q1) ",q2," calls ",jsrs{?g1,?g2}
.h1
Poke.q m,Frac(q1)
.h2
NPrint "Poke.q m,Frac(q1) ",Hex$(Peek.l(m))," calls ",jsrs{?h1,?h2}
.i1
If Frac(q1)<q3 Then c=1 Else c=0
.i2
NPrint "If Frac(q1)<q3 ",c," calls ",jsrs{?i1,?i2}
.j1
NPrint Frac(q1)
.j2
NPrint "NPrint Frac(q1) calls ",jsrs{?j1,?j2}
.k1
q2=Frac(qa(1))*2
.k2
NPrint "q2=Frac(qa(1))*2 ",q2," calls ",jsrs{?k1,?k2}
End
