; expr-limits.test/operand-d8.bas: seven nested parentheses, the right
; operand of * would be in "d8" (#1185)
c1.l=1 : c2.l=2 : e.l=3 : h.l=4 : i.l=5 : j.l=6 : k.l=-1 : m.l=3 : l.l=2
r.l=c1+(c2+(e+(h+(i+(j+(k+m*l))))))
NPrint r
End
