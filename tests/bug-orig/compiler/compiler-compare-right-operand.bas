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
; compiler/expression-types).
; @fixed #1182
Function.f tf{x.f}
  Function Return x
End Function
l1.l=1 : lw.l=65537 : lb.l=100000 : q.q=1.5 : fh.f=0.5 : f25.f=2.5 : fz.f=0
fbig.f=100000.5 : w.w=3 : b.b=3 : qh.q=0.5 : x.w=5
rl.l=0 : rf.f=0 : rq.q=0 : rw.w=0
rl=lw+(l1=l1) : NPrint "01 rl.l=lw+(l1=l1) ",rl
rl=lw-(l1=l1) : NPrint "02 rl.l=lw-(l1=l1) ",rl
rl=lw*(l1=l1) : NPrint "03 rl.l=lw*(l1=l1) ",rl
rl=lw AND (l1=l1) : NPrint "04 rl.l=lw AND (l1=l1) ",rl
rl=lw AND l1=l1 : NPrint "05 rl.l=lw AND l1=l1 ",rl
rl=lw OR (l1<>l1) : NPrint "06 rl.l=lw OR (l1<>l1) ",rl
rl=(l1=l1) AND lw : NPrint "07 rl.l=(l1=l1) AND lw ",rl
rl=lw+l1=l1 : NPrint "08 rl.l=lw+l1=l1 ",rl
rl=0 : If lw AND (l1=l1) Then rl=-1
NPrint "09 If lw AND (l1=l1) ",rl
rl=0 : If $10000 AND (l1=l1) Then rl=-1
NPrint "10 If $10000 AND (l1=l1) ",rl
rl=0 : lw=$10000 : If lw AND l1=l1 Then rl=-1
NPrint "11 lw=$10000 : If lw AND l1=l1 ",rl
lw=65537
rl=lb+(l1=l1) : NPrint "12 rl.l=lb+(l1=l1) ",rl
rf=f25+(l1=l1) : NPrint "13 rf.f=f25+(l1=l1) ",rf
rf=f25-(l1=l1) : NPrint "14 rf.f=f25-(l1=l1) ",rf
rf=f25*(l1=l1) : NPrint "15 rf.f=f25*(l1=l1) ",rf
rf=fbig+(l1=l1) : NPrint "16 rf.f=fbig+(l1=l1) ",rf
rf=fh+((l1=l1)) : NPrint "17 rf.f=fh+((l1=l1)) ",rf
rf=fh+(l1=l1)+fz : NPrint "18 rf.f=fh+(l1=l1)+fz ",rf
rf=fz+fh+(l1=l1) : NPrint "19 rf.f=fz+fh+(l1=l1) ",rf
rf=fh+(l1=l1)*1 : NPrint "20 rf.f=fh+(l1=l1)*1 ",rf
rf=fh+(0+(l1=l1)) : NPrint "21 rf.f=fh+(0+(l1=l1)) ",rf
rq=q+(l1=l1) : NPrint "22 rq.q=q+(l1=l1) ",rq
rq=q*(l1=l1) : NPrint "23 rq.q=q*(l1=l1) ",rq
rq=(l1=l1)+q : NPrint "24 rq.q=(l1=l1)+q ",rq
rl=0 : If q+(l1=l1)>0 Then rl=-1
NPrint "25 If q+(l1=l1)>0 ",rl
rl=0 : If f25*(l1=l1) < -2.25 Then rl=-1
NPrint "26 If f25*(l1=l1) < -2.25 ",rl
rf=w+(l1=l1) : NPrint "27 rf.f=w+(l1=l1) ",rf
rl=lb+(w=w) : NPrint "28 rl.l=lb+(w=w) ",rl
rl=lb+(q=q) : NPrint "29 rl.l=lb+(q=q) ",rl
rl=lb+(fh=fh) : NPrint "30 rl.l=lb+(fh=fh) ",rl
rl=lw AND NOT(l1<>l1) : NPrint "31 rl.l=lw AND NOT(l1<>l1) ",rl
rl=lb+-(l1=l1) : NPrint "32 rl.l=lb+-(l1=l1) ",rl
rl=0 : If qh OR (l1<>l1) Then rl=-1
NPrint "33 If qh OR (l1<>l1) ",rl
rl=0 : If qh EOR (l1<>l1) Then rl=-1
NPrint "34 If qh EOR (l1<>l1) ",rl
flag=1 : rl=0 : If flag AND x=5 Then rl=-1
NPrint "35 flag=1 : If flag AND x=5 ",rl
flag=1.5 : rl=0 : If flag AND x=5 Then rl=-1
NPrint "36 flag=1.5 : If flag AND x=5 ",rl
flag=0.5 : rl=0 : If flag AND x=5 Then rl=-1
NPrint "37 flag=0.5 : If flag AND x=5 ",rl
rl=q+(l1=l1) : NPrint "38 rl.l=q+(l1=l1) ",rl
rl=q*(l1=l1) : NPrint "39 rl.l=q*(l1=l1) ",rl
rw=b+(l1=l1) : NPrint "40 rw.w=b+(l1=l1) ",rw
rw=w AND (l1=l1) : NPrint "41 rw.w=w AND (l1=l1) ",rw
rf=fh+tf{l1=l1} : NPrint "42 rf.f=fh+tf{l1=l1} ",rf
rf=fh+(l1=l1)*100000 : NPrint "43 rf.f=fh+(l1=l1)*100000 ",rf
rq=qh OR (l1<>l1) : NPrint "44 rq.q=qh OR (l1<>l1) ",rq
End
