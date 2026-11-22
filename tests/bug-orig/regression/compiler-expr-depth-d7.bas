; compiler-expr-depth-d7 (#1185) - expressions that reach d7 and stay
; there compile and give the right values (the fix of #1185 refuses only
; an operand beyond d7): an argument from the 7th on is evaluated in d6,
; its right operand in d7; inside six brackets the innermost left operand
; is in d6 as well.  A chain of one precedence, the tighter operator
; first, brackets around the right part, a comparison and a 6th argument
; (in d5).
a.l=1 : b.l=5 : c.l=6 : d.l=31 : aw.w=1 : bw.w=5 : cw.w=6
k.l=-1 : m.l=3 : l.l=2 : c1.l=1 : c2.l=2 : e.l=3 : h.l=4 : i.l=5 : j.l=6
NPrint "01 b*c+a ",RectsHit(20,0,10,10,0,0,b*c+a,10)
NPrint "02 a+(b*c) ",RectsHit(20,0,10,10,0,0,a+(b*c),10)
NPrint "03 a+b+c+d ",RectsHit(20,0,10,10,0,0,a+b+c+d,10)
NPrint "04 d-a+a ",RectsHit(20,0,10,10,0,0,d-a+a,10)
NPrint "05 bw*cw+aw ",RectsHit(20,0,10,10,0,0,bw*cw+aw,10)
NPrint "06 5*6+1 ",RectsHit(20,0,10,10,0,0,5*6+1,10)
NPrint "07 8th b*c+a ",RectsHit(0,20,10,10,0,0,10,b*c+a)
NPrint "08 6th a+b*c ",RectsHit(20,31,10,10,0,a+b*c,40,10)
r.l=c1+(c2+(e+(h+(i+(j+(m>l)))))) : NPrint "09 m>l ",r
r=c1+(c2+(e+(h+(i+(j+(k+m+l)))))) : NPrint "10 k+m+l ",r
r=c1+(c2+(e+(h+(i+(j+(m*l+k)))))) : NPrint "11 m*l+k ",r
r=c1+(c2+(e+(h+(i+(j+(k+(m AND l))))))) : NPrint "12 k+(m AND l) ",r
r=c1+(c2+(e+(h+(i+(j+(1+2+3)))))) : NPrint "13 1+2+3 ",r
r=c1+(c2+(e+(h+(i+(j+(k-(m-l))))))) : NPrint "14 k-(m-l) ",r
End
