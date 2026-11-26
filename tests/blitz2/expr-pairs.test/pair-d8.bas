; expr-pairs.test/pair-d8.bas: six nested parentheses, the 48.16 product
; q*l a register pair in d6/d7, the right operand of the next * would be
; in "d8" (#1185 for register pairs, design decision 10)
q.q=1.5 : l.l=2 : c1.l=1 : c2.l=2 : e.l=3 : h.l=4 : i.l=5 : j.l=6
r.l=c1+(c2+(e+(h+(i+(j+(q*l*l))))))
NPrint r
End
