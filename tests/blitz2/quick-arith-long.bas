; @runerrs off
; Pins the rule of a Quick operand in Long arithmetic, fixed in #1178 (spec
; compiler/expression-types): an expression is evaluated from left to
; right with one "type so far" (the type it is wanted in, e.g. the
; destination).  A Quick fetched while that type is Long keeps its
; fraction; each operation is computed with it and rounded at the
; operator, down for + - * and negation, towards zero for /: r.l=q*2 gives
; 3, r.l=l-q 8, r.l=q/h 3.  A Long constant that makes the type Long
; itself takes the fraction under * and / (35, 36).  Without a wanted
; type (NPrint) each operand keeps its own type since the fix of #1216
; (rule P+): 1+1+1+q gives 4.5, w+v+q 8.5 (20-22); shifts round the Quick
; down (09).
; blitz2 1.95 cut the Quick at fetch (r.l=q*2 gave 2).
; Arithmetic only: comparisons are #1177, array indexes are left out.
Function.q fq{a.q}
  Function Return a
End Function
Statement sl{t$,a.l}
  NPrint t$,a
End Statement
q.q=1.5 : p.q=2.75 : n.q=-1.5 : l.l=10 : w.w=3 : v.w=4 : h.q=0.5
r.l=q*2 : NPrint "01 r.l=q*2 ",r
r.l=q+p : NPrint "02 r.l=q+p ",r
r.l=l-q : NPrint "03 r.l=l-q ",r
r.l=l+q : NPrint "04 r.l=l+q ",r
r.l=n*2 : NPrint "05 r.l=n*2 ",r
r.l=n+0 : NPrint "06 r.l=n+0 ",r
r.l=q/h : NPrint "07 r.l=q/h ",r
r.l=l*q : NPrint "08 r.l=l*q ",r
r.l=q ASL 4 : NPrint "09 r.l=q ASL 4 ",r
r.l=fq{1.5}*2 : NPrint "10 r.l=fq{1.5}*2 ",r
r.l=2.0*q : NPrint "11 r.l=2.0*q ",r
r.l=Int(q*2) : NPrint "12 r.l=Int(q*2) ",r
t.q=q*2 : r.l=t : NPrint "13 t.q=q*2:r.l=t ",r
NPrint "17 Mid$ ",Mid$("abcdefghij",q*2,1)
sl{"18 sl{q*2} ",q*2}
NPrint "19 1+1+q ",1+1+q
NPrint "20 1+1+1+q ",1+1+1+q
NPrint "21 w+v+q ",w+v+q
NPrint "22 w*v+q ",w*v+q
NPrint "23 w-v+q ",w-v+q
x.w=w+v+q*2 : NPrint "24 x.w=w+v+q*2 ",x
x.w=q*2+w+v : NPrint "25 x.w=q*2+w+v ",x
t.q=w+v+q : NPrint "26 t.q=w+v+q ",t
r.l=l : r+q : NPrint "31 r.l=10:r+q ",r
r.l=l : r*q : NPrint "32 r.l=10:r*q ",r
r.l=l : r-q : NPrint "33 r.l=10:r-q ",r
r.l=q*100000 : NPrint "35 r.l=q*100000 ",r
t.q=q*40000 : NPrint "36 t.q=q*40000 ",t
r.l=h+h : NPrint "37 r.l=h+h ",r
r.l=w+q*2 : NPrint "38 r.l=w+q*2 ",r
End
