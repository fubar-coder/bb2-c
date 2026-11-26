; expr-pairs.test/pair-d8-later.bas: the register pair of pair-d8.bas,
; and later in the same expression a String beside a number: the
; pass decides the pairs when the expression is built, so the later error
; is the one reported
q.q=1.5 : l.l=2 : c1.l=1 : c2.l=2 : e.l=3 : h.l=4 : i.l=5 : j.l=6 : a$="x"
r.l=c1+(c2+(e+(h+(i+(j+(q*l*l))))))+a$*2
NPrint r
End
