; @runerrs on
; intrinsic-len-lib-runerrs - Len and Asc where they are not intrinsics
; (compiler-intrinsics, openspec/specs/compiler/intrinsics): an argument
; that is not a plain string variable is built in the work area and the library is
; called, as before ("calls": the JSRs of the statement, the copies
; copstring and the calls of the libraries; Len itself is inline code of
; the library, two instructions without a JSR).  The cases: an
; expression, a function result, a literal, an array element, the current
; element of a List, a field of a NewType variable, a field through
; UsePath, a string variable name with a type suffix .s.  The counts and
; values are those of the build before the intrinsics.
; The program of intrinsic-len-lib.bas with runtime error checking.
; jsrs{a,b}: the JSR instructions ($4EB9) of the code from label a to
; label b
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
NEWTYPE .rec
  id.w
  name$
End NEWTYPE
DEFTYPE .l
c=0
a$="hello" : b$="world!"
Dim s$(5) : s$(3)="three"
Dim List ls$(5) : If AddItem(ls$()) Then ls$()="item"
t.rec\name="field"
v.s="dot"
.a1
c=Len(a$+b$)
.a2
NPrint "c=Len(a$+b$) ",c," calls ",jsrs{?a1,?a2}
.b1
c=Len(Left$(a$,2))
.b2
NPrint "c=Len(Left$(a$,2)) ",c," calls ",jsrs{?b1,?b2}
.c1
c=Len("literal")
.c2
NPrint "c=Len(",Chr$(34),"literal",Chr$(34),") ",c," calls ",jsrs{?c1,?c2}
.d1
c=Len(s$(3))
.d2
NPrint "c=Len(s$(3)) ",c," calls ",jsrs{?d1,?d2}
.e1
c=Len(ls$())
.e2
NPrint "c=Len(ls$()) ",c," calls ",jsrs{?e1,?e2}
.f1
c=Len(t\name)
.f2
NPrint "c=Len(t\name) ",c," calls ",jsrs{?f1,?f2}
USEPATH t
.g1
c=Len(\name)
.g2
NPrint "c=Len(\name) ",c," calls ",jsrs{?g1,?g2}
.h1
c=Len(v)
.h2
NPrint "c=Len(v) ",c," calls ",jsrs{?h1,?h2}
.i1
c=Asc(a$+b$)
.i2
NPrint "c=Asc(a$+b$) ",c," calls ",jsrs{?i1,?i2}
.j1
c=Asc(Mid$(b$,2))
.j2
NPrint "c=Asc(Mid$(b$,2)) ",c," calls ",jsrs{?j1,?j2}
.k1
c=Asc("literal")
.k2
NPrint "c=Asc(",Chr$(34),"literal",Chr$(34),") ",c," calls ",jsrs{?k1,?k2}
.l1
c=Asc(s$(3))
.l2
NPrint "c=Asc(s$(3)) ",c," calls ",jsrs{?l1,?l2}
.m1
c=Asc(ls$())
.m2
NPrint "c=Asc(ls$()) ",c," calls ",jsrs{?m1,?m2}
.n1
c=Asc(t\name)
.n2
NPrint "c=Asc(t\name) ",c," calls ",jsrs{?n1,?n2}
.o1
c=Asc(\name)
.o2
NPrint "c=Asc(\name) ",c," calls ",jsrs{?o1,?o2}
.p1
c=Asc(v)
.p2
NPrint "c=Asc(v) ",c," calls ",jsrs{?p1,?p2}
s$(4)="" : c=Len(s$(4)) : NPrint "empty element ",c," ",Asc(s$(4))
c=Len(s$(5)) : NPrint "unassigned element ",c," ",Asc(s$(5))
End
