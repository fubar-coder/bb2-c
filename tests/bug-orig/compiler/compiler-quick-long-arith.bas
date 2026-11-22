; compiler-quick-long-arith (#1178) - Quick arithmetic under a Long type
; so far, the cases the expression matrix does not hold (three operands,
; brackets, other values, array elements, fields, function values, the
; late cut of a Long operand that sets the type itself).
; BUG(orig) (#1178): a Quick fetched while the type so far is Long is
; cut to a whole number at once, and the operation is done with that
; whole number: with q=1.5 r.l=(q+p)*l computes (1+2)*10.
; Correct: the operation is computed with the fraction and rounded at the
; operator, down for + - * and negation, towards zero for /; a Float
; operand or ^ takes the Quick with its fraction (spec
; compiler/expression-types).
; @fixed #1178
Function.q fq{x.q}
  Function Return x
End Function
NEWTYPE .tq
  v.q
End NEWTYPE
DEFTYPE .tq s
Dim aq.q(2)
l.l=10 : m.l=9 : k.l=-10 : big.l=100000 : base.l=1000
q.q=1.5 : n.q=-1.5 : g.q=-3 : h.q=2 : x.q=-3 : p.q=2.75 : e.q=-32768
a.q=20000 : b.q=20000 : c.q=300 : d.q=200 : z.q=30000.5
w.w=3 : v.w=4 : f.f=2.5 : i=20000
aq(1)=1.5 : s\v=1.5
r.l=0 : t.w=0 : adr.l=0
r=l/n : NPrint "01 r.l=l/n ",r
r=m/n : NPrint "02 r.l=m/n ",r
r=k/q : NPrint "03 r.l=k/q ",r
r=n/l : NPrint "04 r.l=n/l ",r
r=n/2 : NPrint "05 r.l=n/2 ",r
r=g/h : NPrint "06 r.l=g/h ",r
r=l/q : NPrint "07 r.l=l/q ",r
r=k/n : NPrint "08 r.l=k/n ",r
r=l/x : NPrint "09 r.l=l/x ",r
r=-q*2 : NPrint "10 r.l=-q*2 ",r
r=0-q : NPrint "11 r.l=0-q ",r
r=a+b : NPrint "12 r.l=a+b ",r
r=c*d : NPrint "13 r.l=c*d ",r
NPrint "14 q*100000 ",q*100000
NPrint "15 q/100000 ",q/100000
NPrint "16 q+&l-&l ",q+&l-&l
NPrint "17 Int(q)*100000 ",Int(q)*100000
r=l+q*f : NPrint "18 r.l=l+q*f ",r
r=l+q^2 : NPrint "19 r.l=l+q^2 ",r
r=big*z : NPrint "20 r.l=big*z ",r
r=(q+p)*l : NPrint "21 r.l=(q+p)*l ",r
r=l*(q+p) : NPrint "22 r.l=l*(q+p) ",r
r=l-(q) : NPrint "23 r.l=l-(q) ",r
r=aq(1)*2 : NPrint "24 r.l=aq(1)*2 ",r
r=s\v*2 : NPrint "25 r.l=s\v*2 ",r
r=fq{1.5}*2 : NPrint "26 r.l=fq{1.5}*2 ",r
adr=base+i*4 : NPrint "27 adr.l=base+i*4 ",adr
t=w+v+q*2 : NPrint "28 x.w=w+v+q*2 ",t
t=q*2+w+v : NPrint "29 x.w=q*2+w+v ",t
r=l+q+p : NPrint "30 r.l=l+q+p ",r
r=l-n : NPrint "31 r.l=l-n ",r
r=l-e : NPrint "32 r.l=l-e ",r
r=q-l : NPrint "33 r.l=q-l ",r
End
