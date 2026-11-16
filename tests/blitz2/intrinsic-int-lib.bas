; @runerrs off
; intrinsic-int-lib - Int where it is not an intrinsic (compiler-intrinsics,
; openspec/specs/compiler/intrinsics): a Float, Long, Word or Byte
; argument compiles the library call as before ("calls 1": the JSR of
; Int), with the library's values; a Quick expression and an operand with
; a partial result in a register are intrinsics ("calls 0").  The same
; program with runtime error checking (and a string argument):
; intrinsic-int-lib-runerrs.bas.
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
c.l=0 : f.f=-2.75 : l.l=-70000 : w.w=-300 : b.b=-5 : q.q=-1.75 : q1.q=2.5
.a1
c=Int(f)
.a2
NPrint "Int(f) ",c," calls ",jsrs{?a1,?a2}
f=2.75
c=Int(f) : NPrint "Int(f) ",c
.b1
c=Int(l)
.b2
NPrint "Int(l) ",c," calls ",jsrs{?b1,?b2}
.c1
c=Int(w)
.c2
NPrint "Int(w) ",c," calls ",jsrs{?c1,?c2}
.d1
c=Int(b)
.d2
NPrint "Int(b) ",c," calls ",jsrs{?d1,?d2}
; a Quick expression: an intrinsic (the argument is in d0 either way)
.e1
c=Int(q-q1)
.e2
NPrint "Int(q-q1) ",c," calls ",jsrs{?e1,?e2}
; a partial result in d0 while the argument is compiled (movem d0)
c=10
.g1
c=c+Int(q)
.g2
NPrint "c=c+Int(q) ",c," calls ",jsrs{?g1,?g2}
.h1
c=c+Int(f)
.h2
NPrint "c=c+Int(f) ",c," calls ",jsrs{?h1,?h2}
End
