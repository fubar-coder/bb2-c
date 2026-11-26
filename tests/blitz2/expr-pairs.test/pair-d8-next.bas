; expr-pairs.test/pair-d8-next.bas: the register pair of pair-d8.bas, and
; an error in the next statement: the pair's error (line 4) is reported
q.q=1.5 : l.l=2 : c1.l=1 : c2.l=2 : e.l=3 : h.l=4 : i.l=5 : j.l=6 : a$="x"
r.l=c1+(c2+(e+(h+(i+(j+(q*l*l))))))
r=a$*2
NPrint r
End
