; @runerrs on
; intrinsic-int-runerrs - Int of a Quick is compiled inline (compiler-intrinsics,
; openspec/specs/compiler/intrinsics): the integer part truncated towards
; zero, as the library's Quick case, with no library call and no call of
; the error check intchk.  The same program as intrinsic-int.bas, with
; runtime error checking.
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
DEFTYPE .q
c.l=0 : q2.q=0
; the values of the decision (r5-decisions.md: truncated towards zero,
; recorded from the original) and of the spec; 32767.99998 from its bits
; ($7FFFFFFF: the literal is rounded to 32768 and wraps to -32768)
q=3.5 : c=Int(q) : NPrint "3.5 ",c
q=-3.5 : c=Int(q) : NPrint "-3.5 ",c
q=2.5 : c=Int(q) : NPrint "2.5 ",c
q=-2.5 : c=Int(q) : NPrint "-2.5 ",c
q=1.5 : c=Int(q) : NPrint "1.5 ",c
q=1.75 : c=Int(q) : NPrint "1.75 ",c
q=-1.75 : c=Int(q) : NPrint "-1.75 ",c
q=-0.5 : c=Int(q) : NPrint "-0.5 ",c
q=0 : c=Int(q) : NPrint "0 ",c
Poke.l m,$7FFFFFFF : q=Peek.q(m) : c=Int(q) : NPrint "32767.99998 ",c
q=-32768 : c=Int(q) : NPrint "-32768 ",c
; the forms: a Long, a Quick and an unknown target, an expression, an
; element, a nested Int, an operand with a partial result in d0 (movem
; around the argument).  "extra calls": the JSRs of the statement minus
; those of the same statement without Int (0: no call of Int)
q=-1.75 : q1.q=2.25
.a3
c=q
.a4
.a1
c=Int(q)
.a2
NPrint "c=Int(q) ",c," extra calls ",jsrs{?a1,?a2}-jsrs{?a3,?a4}
.b3
q2=q
.b4
.b1
q2=Int(q)
.b2
NPrint "q2=Int(q) ",q2," extra calls ",jsrs{?b1,?b2}-jsrs{?b3,?b4}
c=0
.c3
NPrint c
.c4
.c1
NPrint Int(q)
.c2
NPrint "NPrint Int(q) extra calls ",jsrs{?c1,?c2}-jsrs{?c3,?c4}
.d3
q2=q*q1
.d4
.d1
c=Int(q*q1)
.d2
NPrint "c=Int(q*q1) ",c," extra calls ",jsrs{?d1,?d2}-jsrs{?d3,?d4}
Dim qa.q(3) : qa(2)=-7.5
.e3
q2=qa(2)
.e4
.e1
c=Int(qa(2))
.e2
NPrint "c=Int(qa(2)) ",c," extra calls ",jsrs{?e1,?e2}-jsrs{?e3,?e4}
.f3
q2=q1+c
.f4
.f1
c=Int(q1+Int(q))
.f2
NPrint "c=Int(q1+Int(q)) ",c," extra calls ",jsrs{?f1,?f2}-jsrs{?f3,?f4}
.g3
c=c*3+c
.g4
.g1
c=Int(q)*3+Int(q1)
.g2
NPrint "c=Int(q)*3+Int(q1) ",c," extra calls ",jsrs{?g1,?g2}-jsrs{?g3,?g4}
; the sweep: every integer part at the fractions 0, 1/65536, .5 and
; 65535/65536, the value from the Long bits (magnitude shifted right by
; 16, the sign restored)
n.l=0 : bad.l=0
For ip.l=-32768 To 32767
  For k.l=0 To 3
    fw.l=0
    If k=1 Then fw=1
    If k=2 Then fw=$8000
    If k=3 Then fw=$FFFF
    bits.l=ip*65536+fw
    Poke.l m,bits : q=Peek.q(m)
    If bits<0
      e.l=-((-bits) LSR 16)
    Else
      e.l=bits LSR 16
    EndIf
    c=Int(q)
    n=n+1
    If c<>e
      If bad=0 Then NPrint "first wrong: bits ",Hex$(bits)," Int ",c," expected ",e
      bad=bad+1
    EndIf
  Next
Next
NPrint "sweep ",n," values, ",bad," wrong"
End
