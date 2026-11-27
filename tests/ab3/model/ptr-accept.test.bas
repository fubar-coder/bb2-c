; XTRA
; Residents       = "all.res"
; /XTRA
; ptr-accept - AB3 mode: the right sides a pointer takes (spec
; ab3-compat/type-rules): a library function, a pointer, a constant, a
; long, a typed constant, a long function, a library function of a word,
; an expression whose last variable is a long, a NewType value; a quick
; into a word is converted as in BB2
Function.l fl{}
  Function Return Peek.l(4)
End Function
NEWTYPE.pt
  x.w
  y.w
End NEWTYPE
!TestCase{ptr_accept}
  w.w = 4
  l.l = Peek.l(4)
  #c = 5
  v.pt\x = 3
  *exec.ExecBase = Peek.l(4)
  *e2.ExecBase = ExecBase
  zs$="" : If *exec = Peek.l(4) AND *e2 = *exec Then zs$="exec ok"
  !CheckStrEq{zs$,"exec ok","l1"}
  *a.l = 0
  *b.l = *a
  !CheckEq{*b,0,"l2"}
  *c.l = l
  zs$="" : If *c = *exec Then zs$="long ok"
  !CheckStrEq{zs$,"long ok","l3"}
  *d.l = #c
  !CheckEq{*d,5,"l4"}
  *g.l = fl{}
  zs$="" : If *g = *exec Then zs$="long function ok"
  !CheckStrEq{zs$,"long function ok","l5"}
  *h.l = Peek.l(w)
  !CheckStrEq{"peek ok","peek ok","l6"}
  *i.l = w + l
  zs$="" : If *i = *exec + 4 Then zs$="expression ok"
  !CheckStrEq{zs$,"expression ok","l7"}
  *j.pt = v
  !CheckEq{*j\x,3,"l8"}
  w = 2.5
  !CheckEq{w,2,"l9"}
!EndTestCase
