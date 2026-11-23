; compiler-quick-compare (#1177) - the type an expression is wanted in
; (the "type so far") reaches the operands of a comparison, and a Quick
; fetched while that type is Long is cut to its whole part (blitz2.s:10491,
; varcont): with q=1.5 and a Long destination q>1 compares 1 with 1 (0),
; q=1 is true (-1), Hex$(q>1) is 00000000 (the parameter is a Long), and
; q>l differs from l<q.  A Long against a Quick is also compared after the
; Long is converted to Quick (range +-32768).  Under a Float or Quick
; destination or parameter two whole numbers are converted too: the Longs
; 100000001 and 100000000 are the same FFP value, 65537 as a Quick is 1,
; and a Quick 32767.99998 ($7FFFFFFF) and the Long 32768 are both 32768.0.
; Correct: the operands are compared with their own values (the manual:
; the comparison operators "generate a boolean result"), the boolean is
; -1 or 0 in every destination and the order of the operands does not
; matter; a Long and a Quick are compared exactly (the whole part
; decides, on equality the fraction), and two whole numbers are compared
; as whole numbers in every context.  A compound left operand such as
; q*2>3 is the open bug of compiler-compare-compound-left (#1183).

Function.q fq{a.q}
  Function Return a
End Function
Function.f tf{x.f}
  Function Return x
End Function

!TestCase{quick_compare}
  Dim aq.q(3) : aq(1)=1.5
  q.q=1.5 : n.q=-1.5 : g.q=3 : l.l=1 : r.l=0
  b.l=100000 : bn.l=-100000 : w.w=1 : y.b=2
  r=(q>1)
  !CheckEq{r,-1,"c01"} ; r.l=(q>1)
  r=(q=1)
  !CheckEq{r,0,"c02"} ; r.l=(q=1)
  r=(q>1.4)
  !CheckEq{r,-1,"c03"} ; r.l=(q>1.4)
  r=(1.4<q)
  !CheckEq{r,-1,"c04"} ; r.l=(1.4<q)
  r=(1<q)
  !CheckEq{r,-1,"c05"} ; r.l=(1<q)
  s$=Hex$(q>1) : !CheckStrEq{s$,"FFFFFFFF","c06"} ; Hex$(q>1)
  s$=Hex$(q=1) : !CheckStrEq{s$,"00000000","c07"} ; Hex$(q=1)
  r=0 : If q>l Then r=-1
  !CheckEq{r,-1,"c08"} ; q>l
  r=0 : If l<q Then r=-1
  !CheckEq{r,-1,"c09"} ; l<q
  x.w=(q>1)
  !CheckEq{x,-1,"c10"} ; x.w=(q>1)
  x=(q=1)
  !CheckEq{x,0,"c11"} ; x.w=(q=1)
  f.q=(q>1)
  !CheckQEq{f,-1,"c12"} ; f.q=(q>1)
  r=(q=1.5)
  !CheckEq{r,-1,"c13"} ; r.l=(q=1.5)
  r=(q<>1)
  !CheckEq{r,-1,"c14"} ; r.l=(q<>1)
  r=(q>=2)
  !CheckEq{r,0,"c15"} ; r.l=(q>=2)
  r=(q<2)
  !CheckEq{r,-1,"c16"} ; r.l=(q<2)
  r=(q<=1)
  !CheckEq{r,0,"c17"} ; r.l=(q<=1)
  r=(q>=1)
  !CheckEq{r,-1,"c18"} ; r.l=(q>=1)
  r=(q<=2)
  !CheckEq{r,-1,"c19"} ; r.l=(q<=2)
  r=(n<-1)
  !CheckEq{r,-1,"c20"} ; r.l=(n<-1)
  r=(n>-2)
  !CheckEq{r,-1,"c21"} ; r.l=(n>-2)
  r=(n=-1)
  !CheckEq{r,0,"c22"} ; r.l=(n=-1)
  r=(n<-2)
  !CheckEq{r,0,"c23"} ; r.l=(n<-2)
  r=(n>-1)
  !CheckEq{r,0,"c24"} ; r.l=(n>-1)
  r=(n<=-2)
  !CheckEq{r,0,"c25"} ; r.l=(n<=-2)
  r=(n>=-2)
  !CheckEq{r,-1,"c26"} ; r.l=(n>=-2)
  r=(b>q)
  !CheckEq{r,-1,"c27"} ; r.l=(b>q)
  r=(q<b)
  !CheckEq{r,-1,"c28"} ; r.l=(q<b)
  r=(q>b)
  !CheckEq{r,0,"c29"} ; r.l=(q>b)
  r=(b=q)
  !CheckEq{r,0,"c30"} ; r.l=(b=q)
  r=(bn<q)
  !CheckEq{r,-1,"c31"} ; r.l=(bn<q)
  r=(bn>n)
  !CheckEq{r,0,"c32"} ; r.l=(bn>n)
  r=(q<>b)
  !CheckEq{r,-1,"c33"} ; r.l=(q<>b)
  r=0 : If b>q Then r=-1
  !CheckEq{r,-1,"c34"} ; b>q
  r=0 : If q<b Then r=-1
  !CheckEq{r,-1,"c35"} ; q<b
  r=0 : If bn>q Then r=-1
  !CheckEq{r,0,"c36"} ; bn>q
  r=(q>w)
  !CheckEq{r,-1,"c37"} ; r.l=(q>w)
  r=(w<q)
  !CheckEq{r,-1,"c38"} ; r.l=(w<q)
  r=(w=q)
  !CheckEq{r,0,"c39"} ; r.l=(w=q)
  r=(q<y)
  !CheckEq{r,-1,"c40"} ; r.l=(q<y)
  r=(y>q)
  !CheckEq{r,-1,"c41"} ; r.l=(y>q)
  y=1
  r=(y=q)
  !CheckEq{r,0,"c42"} ; r.l=(y=q)
  r=(q>y)
  !CheckEq{r,-1,"c43"} ; r.l=(q>y)
  r=(fq{1.5}>1)
  !CheckEq{r,-1,"c44"} ; r.l=(fq{1.5}>1)
  r=(fq{1.5}=1)
  !CheckEq{r,0,"c45"} ; r.l=(fq{1.5}=1)
  r=(1<fq{1.5})
  !CheckEq{r,-1,"c46"} ; r.l=(1<fq{1.5})
  r=(aq(1)>1)
  !CheckEq{r,-1,"c47"} ; r.l=(aq(1)>1)
  r=(aq(1)=1)
  !CheckEq{r,0,"c48"} ; r.l=(aq(1)=1)
  r=(l<aq(1))
  !CheckEq{r,-1,"c49"} ; r.l=(l<aq(1))
  r=l+(q>1)
  !CheckEq{r,0,"c50"} ; r.l=l+(q>1)
  r=l+(q=1)
  !CheckEq{r,1,"c51"} ; r.l=l+(q=1)
  r=(q>1) OR (q<1)
  !CheckEq{r,-1,"c52"} ; r.l=(q>1) OR (q<1)
  r=(q>1) AND (q<2)
  !CheckEq{r,-1,"c53"} ; r.l=(q>1) AND (q<2)
  r=(q=1) OR (q>2)
  !CheckEq{r,0,"c54"} ; r.l=(q=1) OR (q>2)
  r=(q>1) AND (q>2)
  !CheckEq{r,0,"c55"} ; r.l=(q>1) AND (q>2)
  r=(g>2)
  !CheckEq{r,-1,"c56"} ; r.l=(g>2)
  r=(g=3)
  !CheckEq{r,-1,"c57"} ; r.l=(g=3)
  r=(g<3)
  !CheckEq{r,0,"c58"} ; r.l=(g<3)
  r=(g=2)
  !CheckEq{r,0,"c59"} ; r.l=(g=2)
  r=(3=g)
  !CheckEq{r,-1,"c60"} ; r.l=(3=g)
  a.b=$90
  r=(a=$90)
  !CheckEq{r,-1,"c61"} ; r.l=(a=$90)
  r=($90=a)
  !CheckEq{r,-1,"c62"} ; r.l=($90=a)
  r=(a<>144)
  !CheckEq{r,0,"c63"} ; r.l=(a<>144)
  r=(a<200)
  !CheckEq{r,-1,"c64"} ; r.l=(a<200)
  a$="x" : b$="x"
  r=(a$=b$)
  !CheckEq{r,-1,"c65"} ; r.l=(a$=b$)
  r=(a$<>b$)
  !CheckEq{r,0,"c66"} ; r.l=(a$<>b$)
  r=(a$="y")
  !CheckEq{r,0,"c67"} ; r.l=(a$=y)
  l2.l=100000001 : l3.l=100000000 : lw.l=65537 : rf.f=0 : rq.q=0
  rf=(l2=l3)
  !CheckFltNear{rf,0,0.0001,"c68"} ; rf.f=(l2=l3)
  rf=(l2<>l3)
  !CheckFltNear{rf,-1,0.0001,"c69"} ; rf.f=(l2<>l3)
  rf=(l2>l3)
  !CheckFltNear{rf,-1,0.0001,"c70"} ; rf.f=(l2>l3)
  rf=(l2<=l3)
  !CheckFltNear{rf,0,0.0001,"c71"} ; rf.f=(l2<=l3)
  rf=(l3=l2)
  !CheckFltNear{rf,0,0.0001,"c72"} ; rf.f=(l3=l2)
  rf=(l3<l2)
  !CheckFltNear{rf,-1,0.0001,"c73"} ; rf.f=(l3<l2)
  rf=(l3>=l2)
  !CheckFltNear{rf,0,0.0001,"c74"} ; rf.f=(l3>=l2)
  rf=(l2>100000000)
  !CheckFltNear{rf,-1,0.0001,"c75"} ; rf.f=(l2>100000000)
  rf=(l2=100000000)
  !CheckFltNear{rf,0,0.0001,"c76"} ; rf.f=(l2=100000000)
  rf=tf{l2>l3}
  !CheckFltNear{rf,-1,0.0001,"c77"} ; rf.f=tf{l2>l3}
  rf=tf{l2=l3}
  !CheckFltNear{rf,0,0.0001,"c78"} ; rf.f=tf{l2=l3}
  rf=Cos(l2=l3)
  !CheckFltNear{rf,1,0.0001,"c79"} ; rf.f=Cos(l2=l3)
  rq=(lw>1)
  !CheckQEq{rq,-1,"c80"} ; rq.q=(lw>1)
  rq=(lw=1)
  !CheckQEq{rq,0,"c81"} ; rq.q=(lw=1)
  rq=(1<lw)
  !CheckQEq{rq,-1,"c82"} ; rq.q=(1<lw)
  rq=(lw=l)
  !CheckQEq{rq,0,"c83"} ; rq.q=(lw=l)
  rq=(a=$90)
  !CheckQEq{rq,-1,"c84"} ; rq.q=(a=$90)
  rf=(q>1)
  !CheckFltNear{rf,-1,0.0001,"c85"} ; rf.f=(q>1)
  c.l=1 : d.l=2 : e.l=3 : h.l=4 : i.l=5 : j.l=6 : k.l=7
  r=c+(d+(e+(h+(i+(j+(k+(q>l)))))))
  !CheckEq{r,27,"c86"} ; deep q>l
  r=c+(d+(e+(h+(i+(j+(k+(l<q)))))))
  !CheckEq{r,27,"c87"} ; deep l<q
  qx.q=0 : Poke.l &qx,$7FFFFFFF : lx.l=32768
  rf=(qx<lx)
  !CheckFltNear{rf,-1,0.0001,"c88"} ; rf.f=(qx<lx)
  rf=(qx=lx)
  !CheckFltNear{rf,0,0.0001,"c89"} ; rf.f=(qx=lx)
  rf=(lx>qx)
  !CheckFltNear{rf,-1,0.0001,"c90"} ; rf.f=(lx>qx)
  rf=(lx=qx)
  !CheckFltNear{rf,0,0.0001,"c91"} ; rf.f=(lx=qx)
  rf=(qx>=lx)
  !CheckFltNear{rf,0,0.0001,"c92"} ; rf.f=(qx>=lx)
  rf=(lx<=qx)
  !CheckFltNear{rf,0,0.0001,"c93"} ; rf.f=(lx<=qx)
!EndTestCase
