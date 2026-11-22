; @runerrs on
; array-index-code-runerrs - the program of array-index-code.bas (#1179)
; with runtime error checking: a later index is a Long expression with the
; size check (arrszchk); the code of a(iw,jw), m(5,7), b(iw,i2,i3) and
; a(iq,jq) keeps its size, a * or / of two Word values calls no library
; routine (muls.w; divs.w with the fall-back for -32768 / -1) where the
; original called lmullib and ldivlib, a Long * still does; the other
; calls are the checks of the access.
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
DEFTYPE .l
Dim a.b(1,60000),m.b(9,9),b.b(9,9,9)
a0=&a(0,0) : m0=&m(0,0) : b0=&b(0,0,0) : o=0
iw.w=1 : jw.w=300 : kw.w=200 : wn.w=-32768 : wm.w=-1
il.l=1 : jl.l=300 : kl.l=200 : iq.q=1 : jq.q=2 : i2.w=2 : i3.w=3
.a1
o=&a(iw,jw)
.a2
NPrint "a(iw,jw) ",(o-a0-1)/2," bytes ",?a2-?a1
.b1
o=&m(5,7)
.b2
NPrint "m(5,7) offset ",o-m0," bytes ",?b2-?b1
.c1
o=&b(iw,i2,i3)
.c2
NPrint "b(iw,i2,i3) offset ",o-b0," bytes ",?c2-?c1
.d1
o=&a(iq,jq)
.d2
NPrint "a(iq,jq) ",(o-a0-1)/2," bytes ",?d2-?d1
.e1
o=&a(iw,jw*kw)
.e2
NPrint "a(iw,jw*kw) ",(o-a0-1)/2," calls ",jsrs{?e1,?e2}
.f1
o=&a(iw,jw/2)
.f2
NPrint "a(iw,jw/2) ",(o-a0-1)/2," calls ",jsrs{?f1,?f2}
.g1
o=&a(iw,wn/wm)
.g2
NPrint "a(iw,wn/wm) ",(o-a0-1)/2," calls ",jsrs{?g1,?g2}
.h1
o=&a(il,jl*kl)
.h2
NPrint "a(il,jl*kl) ",(o-a0-1)/2," calls ",jsrs{?h1,?h2}
End
