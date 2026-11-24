; compiler-compare-right-operand (#1182) - a comparison as the right
; operand of an operator.
; BUG(orig) (#1182): the comparison's boolean is a Word, and the left
; operand is converted down to Word before the operation: a Long loses
; its high word, a Float or a Quick its fraction.  With lw=65537
; rl.l=lw AND (l1=l1) gives 1, with fh=0.5 rf.f=fh+(l1=l1) gives -1, and
; If $10000 AND (l1=l1) is false.
; Correct: the boolean (-1 or 0) is converted up to the left operand's
; type, the left operand keeps its type and value and the operation is
; done in that type (in brackets, bound by precedence, nested, after Not
; or a minus); a Byte or Word left operand, a comparison on the left and
; one passed through a function are as before (spec
; compiler/expression-types).  AND, OR and EOR of a Quick take its whole
; part (#1209): If qh OR (l1<>l1) is false for qh=0.5 (c33, c34, c44), as
; in the original.
Function.f tf{x.f}
  Function Return x
End Function
!TestCase{right_operand}
  l1.l=1 : lw.l=65537 : lb.l=100000 : q.q=1.5 : fh.f=0.5 : f25.f=2.5 : fz.f=0
  fbig.f=100000.5 : w.w=3 : b.b=3 : qh.q=0.5 : x.w=5
  rl.l=0 : rf.f=0 : rq.q=0 : rw.w=0
  rl=lw+(l1=l1)
  !CheckEq{rl,65536,"c01"}   ; rl.l=lw+(l1=l1)
  rl=lw-(l1=l1)
  !CheckEq{rl,65538,"c02"}   ; rl.l=lw-(l1=l1)
  rl=lw*(l1=l1)
  !CheckEq{rl,-65537,"c03"}   ; rl.l=lw*(l1=l1)
  rl=lw AND (l1=l1)
  !CheckEq{rl,65537,"c04"}   ; rl.l=lw AND (l1=l1)
  rl=lw AND l1=l1
  !CheckEq{rl,65537,"c05"}   ; rl.l=lw AND l1=l1
  rl=lw OR (l1<>l1)
  !CheckEq{rl,65537,"c06"}   ; rl.l=lw OR (l1<>l1)
  rl=(l1=l1) AND lw
  !CheckEq{rl,65537,"c07"}   ; rl.l=(l1=l1) AND lw
  rl=lw+l1=l1
  !CheckEq{rl,0,"c08"}   ; rl.l=lw+l1=l1
  rl=0 : If lw AND (l1=l1) Then rl=-1
  !CheckEq{rl,-1,"c09"}   ; If lw AND (l1=l1)
  rl=0 : If $10000 AND (l1=l1) Then rl=-1
  !CheckEq{rl,-1,"c10"}   ; If $10000 AND (l1=l1)
  rl=0 : lw=$10000 : If lw AND l1=l1 Then rl=-1
  !CheckEq{rl,-1,"c11"}   ; lw=$10000 : If lw AND l1=l1
  lw=65537
  rl=lb+(l1=l1)
  !CheckEq{rl,99999,"c12"}   ; rl.l=lb+(l1=l1)
  rf=f25+(l1=l1)
  !CheckFltNear{rf,1.5,0.001,"c13"}   ; rf.f=f25+(l1=l1)
  rf=f25-(l1=l1)
  !CheckFltNear{rf,3.5,0.001,"c14"}   ; rf.f=f25-(l1=l1)
  rf=f25*(l1=l1)
  !CheckFltNear{rf,-2.5,0.001,"c15"}   ; rf.f=f25*(l1=l1)
  rf=fbig+(l1=l1)
  !CheckFltNear{rf,99999.5,0.001,"c16"}   ; rf.f=fbig+(l1=l1)
  rf=fh+((l1=l1))
  !CheckFltNear{rf,-0.5,0.001,"c17"}   ; rf.f=fh+((l1=l1))
  rf=fh+(l1=l1)+fz
  !CheckFltNear{rf,-0.5,0.001,"c18"}   ; rf.f=fh+(l1=l1)+fz
  rf=fz+fh+(l1=l1)
  !CheckFltNear{rf,-0.5,0.001,"c19"}   ; rf.f=fz+fh+(l1=l1)
  rf=fh+(l1=l1)*1
  !CheckFltNear{rf,-0.5,0.001,"c20"}   ; rf.f=fh+(l1=l1)*1
  rf=fh+(0+(l1=l1))
  !CheckFltNear{rf,-0.5,0.001,"c21"}   ; rf.f=fh+(0+(l1=l1))
  rq=q+(l1=l1)
  !CheckQEq{rq,0.5,"c22"}   ; rq.q=q+(l1=l1)
  rq=q*(l1=l1)
  !CheckQEq{rq,-1.5,"c23"}   ; rq.q=q*(l1=l1)
  rq=(l1=l1)+q
  !CheckQEq{rq,0.5,"c24"}   ; rq.q=(l1=l1)+q
  rl=0 : If q+(l1=l1)>0 Then rl=-1
  !CheckEq{rl,-1,"c25"}   ; If q+(l1=l1)>0
  rl=0 : If f25*(l1=l1) < -2.25 Then rl=-1
  !CheckEq{rl,-1,"c26"}   ; If f25*(l1=l1) < -2.25
  rf=w+(l1=l1)
  !CheckFltNear{rf,2,0.001,"c27"}   ; rf.f=w+(l1=l1)
  rl=lb+(w=w)
  !CheckEq{rl,99999,"c28"}   ; rl.l=lb+(w=w)
  rl=lb+(q=q)
  !CheckEq{rl,99999,"c29"}   ; rl.l=lb+(q=q)
  rl=lb+(fh=fh)
  !CheckEq{rl,99999,"c30"}   ; rl.l=lb+(fh=fh)
  rl=lw AND NOT(l1<>l1)
  !CheckEq{rl,65537,"c31"}   ; rl.l=lw AND NOT(l1<>l1)
  rl=lb+-(l1=l1)
  !CheckEq{rl,100001,"c32"}   ; rl.l=lb+-(l1=l1)
  rl=0 : If qh OR (l1<>l1) Then rl=-1
  !CheckEq{rl,0,"c33"}   ; If qh OR (l1<>l1)
  rl=0 : If qh EOR (l1<>l1) Then rl=-1
  !CheckEq{rl,0,"c34"}   ; If qh EOR (l1<>l1)
  flag=1 : rl=0 : If flag AND x=5 Then rl=-1
  !CheckEq{rl,-1,"c35"}   ; flag=1 : If flag AND x=5
  flag=1.5 : rl=0 : If flag AND x=5 Then rl=-1
  !CheckEq{rl,-1,"c36"}   ; flag=1.5 : If flag AND x=5
  flag=0.5 : rl=0 : If flag AND x=5 Then rl=-1
  !CheckEq{rl,0,"c37"}   ; flag=0.5 : If flag AND x=5
  rl=q+(l1=l1)
  !CheckEq{rl,0,"c38"}   ; rl.l=q+(l1=l1)
  rl=q*(l1=l1)
  !CheckEq{rl,-2,"c39"}   ; rl.l=q*(l1=l1)
  rw=b+(l1=l1)
  !CheckEq{rw,2,"c40"}   ; rw.w=b+(l1=l1)
  rw=w AND (l1=l1)
  !CheckEq{rw,3,"c41"}   ; rw.w=w AND (l1=l1)
  rf=fh+tf{l1=l1}
  !CheckFltNear{rf,-0.5,0.001,"c42"}   ; rf.f=fh+tf{l1=l1}
  rf=fh+(l1=l1)*100000
  !CheckFltNear{rf,-99999.5,0.001,"c43"}   ; rf.f=fh+(l1=l1)*100000
  rq=qh OR (l1<>l1)
  !CheckQEq{rq,0,"c44"}   ; rq.q=qh OR (l1<>l1)
!EndTestCase
