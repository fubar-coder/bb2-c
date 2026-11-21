; compiler-quick-compare (#1177) - the type an expression is wanted in
; (the "type so far") reaches the operands of a comparison, and a Quick
; fetched while that type is Long is cut to its whole part (blitz2.s:10491,
; varcont): with q=1.5 and a Long destination q>1 compares 1 with 1 (0),
; q=1 is true (-1), Hex$(q>1) is 00000000 (the parameter is a Long), and
; q>l differs from l<q.  A Long against a Quick is also compared after the
; Long is converted to Quick (range +-32768).
; Correct: the operands are compared with their own values (the manual:
; the comparison operators "generate a boolean result"), the boolean is
; -1 or 0 in every destination and the order of the operands does not
; matter; a Long and a Quick are compared exactly (the whole part
; decides, on equality the fraction).
; The last line shows the decided limit: a compound left operand
; (q*2>3) is still cut inside the expression until the expression tree
; (#1180); its value 0 is not the mathematical one (-1) and is expected
; here, so the line changes with #1180.
; @fixed #1177
Function.q fq{a.q}
  Function Return a
End Function
Dim aq.q(3) : aq(1)=1.5
q.q=1.5 : n.q=-1.5 : g.q=3 : l.l=1 : r.l=0
b.l=100000 : bn.l=-100000 : w.w=1 : y.b=2
r=(q>1) : NPrint "01 r.l=(q>1) ",r
r=(q=1) : NPrint "02 r.l=(q=1) ",r
r=(q>1.4) : NPrint "03 r.l=(q>1.4) ",r
r=(1.4<q) : NPrint "04 r.l=(1.4<q) ",r
r=(1<q) : NPrint "05 r.l=(1<q) ",r
NPrint "06 Hex$(q>1) ",Hex$(q>1)
NPrint "07 Hex$(q=1) ",Hex$(q=1)
If q>l Then NPrint "08 q>l true" Else NPrint "08 q>l false"
If l<q Then NPrint "09 l<q true" Else NPrint "09 l<q false"
x.w=(q>1) : NPrint "10 x.w=(q>1) ",x
x=(q=1) : NPrint "11 x.w=(q=1) ",x
f.q=(q>1) : NPrint "12 f.q=(q>1) ",f
r=(q=1.5) : NPrint "13 r.l=(q=1.5) ",r
r=(q<>1) : NPrint "14 r.l=(q<>1) ",r
r=(q>=2) : NPrint "15 r.l=(q>=2) ",r
r=(q<2) : NPrint "16 r.l=(q<2) ",r
r=(q<=1) : NPrint "17 r.l=(q<=1) ",r
r=(q>=1) : NPrint "18 r.l=(q>=1) ",r
r=(q<=2) : NPrint "19 r.l=(q<=2) ",r
r=(n<-1) : NPrint "20 r.l=(n<-1) ",r
r=(n>-2) : NPrint "21 r.l=(n>-2) ",r
r=(n=-1) : NPrint "22 r.l=(n=-1) ",r
r=(n<-2) : NPrint "23 r.l=(n<-2) ",r
r=(n>-1) : NPrint "24 r.l=(n>-1) ",r
r=(n<=-2) : NPrint "25 r.l=(n<=-2) ",r
r=(n>=-2) : NPrint "26 r.l=(n>=-2) ",r
r=(b>q) : NPrint "27 r.l=(b>q) ",r
r=(q<b) : NPrint "28 r.l=(q<b) ",r
r=(q>b) : NPrint "29 r.l=(q>b) ",r
r=(b=q) : NPrint "30 r.l=(b=q) ",r
r=(bn<q) : NPrint "31 r.l=(bn<q) ",r
r=(bn>n) : NPrint "32 r.l=(bn>n) ",r
r=(q<>b) : NPrint "33 r.l=(q<>b) ",r
If b>q Then NPrint "34 b>q true" Else NPrint "34 b>q false"
If q<b Then NPrint "35 q<b true" Else NPrint "35 q<b false"
If bn>q Then NPrint "36 bn>q true" Else NPrint "36 bn>q false"
r=(q>w) : NPrint "37 r.l=(q>w) ",r
r=(w<q) : NPrint "38 r.l=(w<q) ",r
r=(w=q) : NPrint "39 r.l=(w=q) ",r
r=(q<y) : NPrint "40 r.l=(q<y) ",r
r=(y>q) : NPrint "41 r.l=(y>q) ",r
y=1
r=(y=q) : NPrint "42 r.l=(y=q) ",r
r=(q>y) : NPrint "43 r.l=(q>y) ",r
r=(fq{1.5}>1) : NPrint "44 r.l=(fq{1.5}>1) ",r
r=(fq{1.5}=1) : NPrint "45 r.l=(fq{1.5}=1) ",r
r=(1<fq{1.5}) : NPrint "46 r.l=(1<fq{1.5}) ",r
r=(aq(1)>1) : NPrint "47 r.l=(aq(1)>1) ",r
r=(aq(1)=1) : NPrint "48 r.l=(aq(1)=1) ",r
r=(l<aq(1)) : NPrint "49 r.l=(l<aq(1)) ",r
r=l+(q>1) : NPrint "50 r.l=l+(q>1) ",r
r=l+(q=1) : NPrint "51 r.l=l+(q=1) ",r
r=(q>1) OR (q<1) : NPrint "52 r.l=(q>1) OR (q<1) ",r
r=(q>1) AND (q<2) : NPrint "53 r.l=(q>1) AND (q<2) ",r
r=(q=1) OR (q>2) : NPrint "54 r.l=(q=1) OR (q>2) ",r
r=(q>1) AND (q>2) : NPrint "55 r.l=(q>1) AND (q>2) ",r
r=(g>2) : NPrint "56 r.l=(g>2) ",r
r=(g=3) : NPrint "57 r.l=(g=3) ",r
r=(g<3) : NPrint "58 r.l=(g<3) ",r
r=(g=2) : NPrint "59 r.l=(g=2) ",r
r=(3=g) : NPrint "60 r.l=(3=g) ",r
a.b=$90
r=(a=$90) : NPrint "61 r.l=(a=$90) ",r
r=($90=a) : NPrint "62 r.l=($90=a) ",r
r=(a<>144) : NPrint "63 r.l=(a<>144) ",r
r=(a<200) : NPrint "64 r.l=(a<200) ",r
a$="x" : b$="x"
r=(a$=b$) : NPrint "65 r.l=(a$=b$) ",r
r=(a$<>b$) : NPrint "66 r.l=(a$<>b$) ",r
r=(a$="y") : NPrint "67 r.l=(a$=y) ",r
r=(q*2>3) : NPrint "99 limit #1180 r.l=(q*2>3) ",r
End
