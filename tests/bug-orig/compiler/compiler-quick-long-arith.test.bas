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

Function.q fq{x.q}
  Function Return x
End Function
NEWTYPE .tq
  v.q
End NEWTYPE
DEFTYPE .tq s

!TestCase{quick_long_arith}
  Dim aq.q(2)
  l.l=10 : m.l=9 : k.l=-10 : big.l=100000 : base.l=1000
  q.q=1.5 : n.q=-1.5 : g.q=-3 : h.q=2 : x.q=-3 : p.q=2.75 : e.q=-32768
  a.q=20000 : b.q=20000 : c.q=300 : d.q=200 : z.q=30000.5
  w.w=3 : v.w=4 : f.f=2.5 : i=20000
  aq(1)=1.5 : s\v=1.5
  r.l=0 : t.w=0 : adr.l=0
  r=l/n
  !CheckEq{r,-6,"c01"} ; r.l=l/n
  r=m/n
  !CheckEq{r,-6,"c02"} ; r.l=m/n
  r=k/q
  !CheckEq{r,-6,"c03"} ; r.l=k/q
  r=n/l
  !CheckEq{r,0,"c04"} ; r.l=n/l
  r=n/2
  !CheckEq{r,0,"c05"} ; r.l=n/2
  r=g/h
  !CheckEq{r,-1,"c06"} ; r.l=g/h
  r=l/q
  !CheckEq{r,6,"c07"} ; r.l=l/q
  r=k/n
  !CheckEq{r,6,"c08"} ; r.l=k/n
  r=l/x
  !CheckEq{r,-3,"c09"} ; r.l=l/x
  r=-q*2
  !CheckEq{r,-3,"c10"} ; r.l=-q*2
  r=0-q
  !CheckEq{r,-2,"c11"} ; r.l=0-q
  r=a+b
  !CheckEq{r,40000,"c12"} ; r.l=a+b
  r=c*d
  !CheckEq{r,60000,"c13"} ; r.l=c*d
  !CheckEq{q*100000,150000,"c14"} ; q*100000
  !CheckEq{q/100000,0,"c15"} ; q/100000
  !CheckEq{q+&l-&l,1,"c16"} ; q+&l-&l
  !CheckEq{Int(q)*100000,100000,"c17"} ; Int(q)*100000
  r=l+q*f
  !CheckEq{r,13,"c18"} ; r.l=l+q*f
  r=l+q^2
  !CheckEq{r,12,"c19"} ; r.l=l+q^2
  r=big*z
  !CheckEq{r,-1294917296,"c20"} ; r.l=big*z
  r=(q+p)*l
  !CheckEq{r,40,"c21"} ; r.l=(q+p)*l
  r=l*(q+p)
  !CheckEq{r,40,"c22"} ; r.l=l*(q+p)
  r=l-(q)
  !CheckEq{r,8,"c23"} ; r.l=l-(q)
  r=aq(1)*2
  !CheckEq{r,3,"c24"} ; r.l=aq(1)*2
  r=s\v*2
  !CheckEq{r,3,"c25"} ; r.l=s\v*2
  r=fq{1.5}*2
  !CheckEq{r,3,"c26"} ; r.l=fq{1.5}*2
  adr=base+i*4
  !CheckEq{adr,81000,"c27"} ; adr.l=base+i*4
  t=w+v+q*2
  !CheckEq{t,10,"c28"} ; x.w=w+v+q*2
  t=q*2+w+v
  !CheckEq{t,10,"c29"} ; x.w=q*2+w+v
  r=l+q+p
  !CheckEq{r,13,"c30"} ; r.l=l+q+p
  r=l-n
  !CheckEq{r,11,"c31"} ; r.l=l-n
  r=l-e
  !CheckEq{r,32778,"c32"} ; r.l=l-e
  r=q-l
  !CheckEq{r,-9,"c33"} ; r.l=q-l
!EndTestCase
