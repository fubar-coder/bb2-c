; expr-q48-paths - the code paths of the 48.16 operations of rule P+
; (#1216; OpenSpec change expr-tree-5-operand-typing, task 4.3): the
; operands moved into d0-d3 without the stack, + and - and the comparisons
; written inline, a whole number and a 16.16 value under a wanted Long by
; the routines of #1178 (mixedop), the low 32 bits under a wanted Quick in
; 32 bits, a constant Case of a 48.16 Select compared as immediates, a
; 16.16 value divided by a whole constant, times a constant below 1 or
; times a Word inline (the fast paths), a 48.16 division by a 16.16
; value that is whole at run time inline, Mod by a Quick that is whole at
; run time or by a whole constant inline (a Quick or a 48.16 dividend),
; a whole dividend of such a division or Mod not widened, a / at the top
; under a wanted Long as its Long, the operations in parameters (registers
; d1 to d5).  Each value is the one the code of task 4.2 gives (all its
; 48.16 operations through compilerlib's routines, measured once; the Mod
; checks on main ad8396cf9, every such Mod through q48mod; the checks of
; the speed review's candidates, wd2_ and after, on main 8f9a8bee1); a
; regression test of the fixed port, no key, as select-fraction.test.bas.
; Since type-system-rework (#1235, task 4.2; spec compiler/expression-types,
; "An operation with a Quick is computed in 48.16", "The conversion to a
; Quick rounds down and saturates") every Quick operation is 48.16 cut
; down, a value beyond the Quick range under a wanted Quick saturates
; ($7FFFFFFF, $80000000) instead of keeping its low 32 bits, a / under a
; wanted Long rounds down instead of towards zero, and a whole Long
; operation wraps (lb+lb is -2); each check whose value changed names the
; one before in its comment.  Since task 4.7 Mod is the floor Mod (#1197:
; the remainder has the divisor's sign) and a divisor 0 raises the
; division trap (expr-tsr-div0-trap): those checks left, the Mod checks
; with a negative divisor at their floor values.
Function.l pl1{a.l,b.l,c.l,d.l,e.l,f.l}
  Function Return a
End Function
Function.l pl2{a.l,b.l,c.l,d.l,e.l,f.l}
  Function Return b
End Function
Function.l pl3{a.l,b.l,c.l,d.l,e.l,f.l}
  Function Return c
End Function
Function.l pl4{a.l,b.l,c.l,d.l,e.l,f.l}
  Function Return d
End Function
Function.l pl5{a.l,b.l,c.l,d.l,e.l,f.l}
  Function Return e
End Function
Function.l pl6{a.l,b.l,c.l,d.l,e.l,f.l}
  Function Return f
End Function
Function.l pq1{a.q,b.q,c.q,d.q,e.q,f.q}
  Function Return Peek.l(&a)
End Function
Function.l pq2{a.q,b.q,c.q,d.q,e.q,f.q}
  Function Return Peek.l(&b)
End Function
Function.l pq3{a.q,b.q,c.q,d.q,e.q,f.q}
  Function Return Peek.l(&c)
End Function
Function.l pq4{a.q,b.q,c.q,d.q,e.q,f.q}
  Function Return Peek.l(&d)
End Function
Function.l pq5{a.q,b.q,c.q,d.q,e.q,f.q}
  Function Return Peek.l(&e)
End Function
Function.l pq6{a.q,b.q,c.q,d.q,e.q,f.q}
  Function Return Peek.l(&f)
End Function
!TestCase{paths}
  l.l=100 : lx.l=100000 : ln.l=-100000 : lm.l=2147483647 : lz.l=0 : lb.l=2147483647
  q.q=2.5 : qn.q=-1.75 : qs.q=1 : p.q=1.25 : qm.q=-32768
  w.w=3 : wn.w=-7 : b.b=-3 : r.l=0 : x.q=0 : w2.w=0 : b2.b=0 : k.l=0 : acc.l=0
  qs=qs/65536
  r=l*q
  !CheckEq{r,250,"r_l_mul_q"}
  r=q*l
  !CheckEq{r,250,"r_q_mul_l"}
  r=lx*q
  !CheckEq{r,250000,"r_lx_mul_q"}
  r=ln*qn
  !CheckEq{r,175000,"r_ln_mul_qn"}
  r=ln*qs
  !CheckEq{r,-2,"r_ln_mul_qs"}
  r=lm*q
  !CheckEq{r,1073741821,"r_lm_mul_q"}
  r=lm*qn
  !CheckEq{r,536870913,"r_lm_mul_qn"}
  r=l*qm
  !CheckEq{r,-3276800,"r_l_mul_qm"}
  r=l*2.5
  !CheckEq{r,250,"r_l_mul_c"}
  r=3*q
  !CheckEq{r,7,"r_c_mul_q"}
  r=w*q
  !CheckEq{r,7,"r_w_mul_q"}
  r=wn*q
  !CheckEq{r,-18,"r_wn_mul_q"}
  r=b*q
  !CheckEq{r,-8,"r_b_mul_q"}
  r=q*-l
  !CheckEq{r,-250,"r_q_mul_negl"}
  r=-l*qn
  !CheckEq{r,175,"r_negl_mul_qn"}
  r=lz*qn
  !CheckEq{r,0,"r_lz_mul_qn"}
  r=-1*qs
  !CheckEq{r,-1,"r_c_mul_qs"}
  r=l+q
  !CheckEq{r,102,"r_l_add_q"}
  r=q+l
  !CheckEq{r,102,"r_q_add_l"}
  r=l-q
  !CheckEq{r,97,"r_l_sub_q"}
  r=q-l
  !CheckEq{r,-98,"r_q_sub_l"}
  r=ln-qn
  !CheckEq{r,-99999,"r_ln_sub_qn"}
  r=qn-ln
  !CheckEq{r,99998,"r_qn_sub_ln"}
  r=q+p
  !CheckEq{r,3,"r_q_add_p"}
  r=q-p
  !CheckEq{r,1,"r_q_sub_p"}
  r=qn-p
  !CheckEq{r,-3,"r_qn_sub_p"}
  r=qn+qs
  !CheckEq{r,-2,"r_qn_add_qs"}
  r=l+1.5
  !CheckEq{r,101,"r_l_add_c"}
  r=1.5-l
  !CheckEq{r,-98,"r_c_sub_l"}
  r=lm+q
  !CheckEq{r,-2147483647,"r_lm_add_q"}
  r=lm-qn
  !CheckEq{r,-2147483648,"r_lm_sub_qn"}
  r=-lm-q
  !CheckEq{r,2147483646,"r_neglm_sub_q"}
  r=qs-l
  !CheckEq{r,-100,"r_qs_sub_l"}
  r=w+qn
  !CheckEq{r,1,"r_w_add_qn"}
  r=wn-q
  !CheckEq{r,-10,"r_wn_sub_q"}
  r=qm-lm
  !CheckEq{r,2147450881,"r_qm_sub_lm"}
  r=lx/q
  !CheckEq{r,40000,"r_lx_div_q"}
  r=l+q+p
  !CheckEq{r,103,"r_l_add_q_add_p"}
  r=l*q+p
  !CheckEq{r,251,"r_l_mul_q_add_p"}
  w2=l*q
  !CheckEq{w2,250,"w_l_mul_q"}
  w2=lx*qn
  !CheckEq{w2,21608,"w_lx_mul_qn"}
  b2=l+q
  !CheckEq{b2,102,"b_l_add_q"}
  b2=ln-q
  !CheckEq{b2,93,"b_ln_sub_q"}
  w2=l-qn
  !CheckEq{w2,101,"w_l_sub_qn"}
  x=l*q
  !CheckEq{Peek.l(&x),16384000,"x_l_mul_q"}
  x=lx*q
  ; before: -795869184
  !CheckEq{Peek.l(&x),2147483647,"x_lx_mul_q"}
  x=ln*qn
  ; before: -1416101888
  !CheckEq{Peek.l(&x),2147483647,"x_ln_mul_qn"}
  x=q*w
  !CheckEq{Peek.l(&x),491520,"x_q_mul_w"}
  x=wn*qn
  !CheckEq{Peek.l(&x),802816,"x_wn_mul_qn"}
  x=l*2.5
  !CheckEq{Peek.l(&x),16384000,"x_l_mul_c"}
  x=3.5*l
  !CheckEq{Peek.l(&x),22937600,"x_c_mul_l"}
  x=lm*q
  ; before: -163840
  !CheckEq{Peek.l(&x),2147483647,"x_lm_mul_q"}
  x=lm*qn
  ; before: 114688
  !CheckEq{Peek.l(&x),-2147483648,"x_lm_mul_qn"}
  x=ln*qs
  !CheckEq{Peek.l(&x),-100000,"x_ln_mul_qs"}
  x=l*qm
  ; before: 0
  !CheckEq{Peek.l(&x),-2147483648,"x_l_mul_qm"}
  x=-l*q
  !CheckEq{Peek.l(&x),-16384000,"x_negl_mul_q"}
  x=lz*q
  !CheckEq{Peek.l(&x),0,"x_lz_mul_q"}
  x=b*q
  !CheckEq{Peek.l(&x),-491520,"x_b_mul_q"}
  x=l+q
  !CheckEq{Peek.l(&x),6717440,"x_l_add_q"}
  x=l+q+p
  !CheckEq{Peek.l(&x),6799360,"x_l_add_q_add_p"}
  x=l-q-p
  !CheckEq{Peek.l(&x),6307840,"x_l_sub_q_sub_p"}
  x=q-l+p
  !CheckEq{Peek.l(&x),-6307840,"x_q_sub_l_add_p"}
  x=lx+q
  ; before: -2036170752
  !CheckEq{Peek.l(&x),2147483647,"x_lx_add_q"}
  x=l+q-lx
  ; before: 2043052032
  !CheckEq{Peek.l(&x),-2147483648,"x_l_add_q_sub_lx"}
  x=lm+qn
  ; before: -180224
  !CheckEq{Peek.l(&x),2147483647,"x_lm_add_qn"}
  x=qn-lm
  ; before: -49152
  !CheckEq{Peek.l(&x),-2147483648,"x_qn_sub_lm"}
  x=l+qs-p
  !CheckEq{Peek.l(&x),6471681,"x_l_add_qs_sub_p"}
  x=-l+q
  !CheckEq{Peek.l(&x),-6389760,"x_negl_add_q"}
  x=(l+q)-(lx-p)
  ; before: 2043133952
  !CheckEq{Peek.l(&x),-2147483648,"x_brackets_sub"}
  x=(l-q)+(p-ln)
  ; before: -2029862912
  !CheckEq{Peek.l(&x),2147483647,"x_brackets_add"}
  x=w+l+q
  !CheckEq{Peek.l(&x),6914048,"x_w_add_l_add_q"}
  x=l+1.25+q
  !CheckEq{Peek.l(&x),6799360,"x_l_add_c_add_q"}
  x=l+q*p
  !CheckEq{Peek.l(&x),6758400,"x_l_add_q_mul_p"}
  x=l*q+p
  !CheckEq{Peek.l(&x),16465920,"x_l_mul_q_add_p"}
  x=q+lx*p
  ; before: -397770752
  !CheckEq{Peek.l(&x),2147483647,"x_q_add_lx_mul_p"}
  x=l/q+p
  !CheckEq{Peek.l(&x),2703360,"x_l_div_q_add_p"}
  x=lb+q
  ; before: 98304
  !CheckEq{Peek.l(&x),2147483647,"x_lb_add_q"}
  x=lb*qn
  ; before: 114688
  !CheckEq{Peek.l(&x),-2147483648,"x_lb_mul_qn"}
  r=0 : If l+q>p Then r=1
  !CheckEq{r,1,"if_gt"}
  r=0 : If p<l+q Then r=1
  !CheckEq{r,1,"if_lt"}
  r=0 : If l+q=102.5 Then r=1
  !CheckEq{r,1,"if_eq"}
  r=0 : If l*q<>250 Then r=1
  !CheckEq{r,0,"if_ne"}
  r=0 : If lx*q>=250000 Then r=1
  !CheckEq{r,1,"if_ge"}
  r=0 : If l+q<=p Then r=1
  !CheckEq{r,0,"if_le"}
  r=0 : If ln-q<-100002 Then r=1
  !CheckEq{r,1,"if_lt2"}
  r=0 : If lm+q>0 Then r=1
  !CheckEq{r,1,"if_gt0"}
  r=0 : If ln*qn=175000 Then r=1
  !CheckEq{r,1,"if_eq2"}
  r=0 : If l*qn<0 Then r=1
  !CheckEq{r,1,"if_lt0"}
  r=0 : If l+q<>102.5 Then r=1
  !CheckEq{r,0,"if_ne2"}
  r=0 : If lx*qn<ln Then r=1
  !CheckEq{r,1,"if_lt3"}
  r=0 : If l+q-102=0.5 Then r=1
  !CheckEq{r,1,"if_eq3"}
  r=(l+q>p)+(l*q<>250)
  !CheckEq{r,-1,"r_cmp_sum"}
  acc=0
  For k=1 To 3
  Select l*q+k
    Case 251 : acc=acc*10+1
    Case 252 : acc=acc*10+2
    Case -1 : acc=acc*10+3
    Case 70000 : acc=acc*10+4
    Default : acc=acc*10+5
  End Select
  Next
  !CheckEq{acc,125,"sel_whole"}
  acc=0
  For k=-2 To 2
  Select ln*qn/1000+k*0.5
    Case 174 : acc=acc*10+1
    Case 175 : acc=acc*10+2
    Case 176 : acc=acc*10+3
    Case -3 : acc=acc*10+4
    Case 174.5 : acc=acc*10+5
    Case k : acc=acc*10+6
    Default : acc=acc*10+7
  End Select
  Next
  !CheckEq{acc,15273,"sel_frac"}
  acc=0
  For k=-1 To 1
  Select l*k+q
    Case -97.5 : acc=acc*10+1
    Case 2.5 : acc=acc*10+2
    Case 102 : acc=acc*10+3
    Case 102.5 : acc=acc*10+4
    Case -98 : acc=acc*10+5
    Case 32768 : acc=acc*10+6
    Case 2147483647 : acc=acc*10+7
    Default : acc=acc*10+8
  End Select
  Next
  !CheckEq{acc,124,"sel_neg"}
  acc=0
  For k=0 To 1
  Select lb*q+k
    Case -2 : acc=acc*10+1
    Case 5368709117 : acc=acc*10+2
    Case 5368709118 : acc=acc*10+3
    Default : acc=acc*10+4
  End Select
  Next
  !CheckEq{acc,44,"sel_big"}
  acc=0
  For k=-4 To -2
  Select b*q+k*0.5+l
    Case 90 : acc=acc*10+1
    Case 89.5 : acc=acc*10+2
    Case 90.5 : acc=acc*10+3
    Default : acc=acc*10+4
  End Select
  Next
  !CheckEq{acc,344,"sel_byte"}
  r=pl1{l*q,lx*q,l+q,q-l,lm+q,ln*qn}
  !CheckEq{r,250,"pl_a_1"}
  r=pl2{l*q,lx*q,l+q,q-l,lm+q,ln*qn}
  !CheckEq{r,250000,"pl_a_2"}
  r=pl3{l*q,lx*q,l+q,q-l,lm+q,ln*qn}
  !CheckEq{r,102,"pl_a_3"}
  r=pl4{l*q,lx*q,l+q,q-l,lm+q,ln*qn}
  !CheckEq{r,-98,"pl_a_4"}
  r=pl5{l*q,lx*q,l+q,q-l,lm+q,ln*qn}
  !CheckEq{r,-2147483647,"pl_a_5"}
  r=pl6{l*q,lx*q,l+q,q-l,lm+q,ln*qn}
  !CheckEq{r,175000,"pl_a_6"}
  r=pq1{l*q,lx*q,l+q,q-l,l*qn,lx*qn}
  !CheckEq{r,16384000,"pq_a_1"}
  r=pq2{l*q,lx*q,l+q,q-l,l*qn,lx*qn}
  ; before: -795869184
  !CheckEq{r,2147483647,"pq_a_2"}
  r=pq3{l*q,lx*q,l+q,q-l,l*qn,lx*qn}
  !CheckEq{r,6717440,"pq_a_3"}
  r=pq4{l*q,lx*q,l+q,q-l,l*qn,lx*qn}
  !CheckEq{r,-6389760,"pq_a_4"}
  r=pq5{l*q,lx*q,l+q,q-l,l*qn,lx*qn}
  !CheckEq{r,-11468800,"pq_a_5"}
  r=pq6{l*q,lx*q,l+q,q-l,l*qn,lx*qn}
  ; before: 1416101888
  !CheckEq{r,-2147483648,"pq_a_6"}
  r=pq1{q,p,l*q,lx*q,ln*qn,lx+q}
  !CheckEq{r,163840,"pq_b_1"}
  r=pq2{q,p,l*q,lx*q,ln*qn,lx+q}
  !CheckEq{r,81920,"pq_b_2"}
  r=pq3{q,p,l*q,lx*q,ln*qn,lx+q}
  !CheckEq{r,16384000,"pq_b_3"}
  r=pq4{q,p,l*q,lx*q,ln*qn,lx+q}
  ; before: -795869184
  !CheckEq{r,2147483647,"pq_b_4"}
  r=pq5{q,p,l*q,lx*q,ln*qn,lx+q}
  ; before: -1416101888
  !CheckEq{r,2147483647,"pq_b_5"}
  r=pq6{q,p,l*q,lx*q,ln*qn,lx+q}
  ; before: -2036170752
  !CheckEq{r,2147483647,"pq_b_6"}
  r=pq1{q,p,qn,l+q-p,lx-q,l*q}
  !CheckEq{r,163840,"pq_c_1"}
  r=pq2{q,p,qn,l+q-p,lx-q,l*q}
  !CheckEq{r,81920,"pq_c_2"}
  r=pq3{q,p,qn,l+q-p,lx-q,l*q}
  !CheckEq{r,-114688,"pq_c_3"}
  r=pq4{q,p,qn,l+q-p,lx-q,l*q}
  !CheckEq{r,6635520,"pq_c_4"}
  r=pq5{q,p,qn,l+q-p,lx-q,l*q}
  ; before: -2036498432
  !CheckEq{r,2147483647,"pq_c_5"}
  r=pq6{q,p,qn,l+q-p,lx-q,l*q}
  !CheckEq{r,16384000,"pq_c_6"}
  r=pl1{1,l+q>p,l*q=250,lx*qn<ln,lx+q>lx,5}
  !CheckEq{r,1,"pl_cmp_1"}
  r=pl2{1,l+q>p,l*q=250,lx*qn<ln,lx+q>lx,5}
  !CheckEq{r,-1,"pl_cmp_2"}
  r=pl3{1,l+q>p,l*q=250,lx*qn<ln,lx+q>lx,5}
  !CheckEq{r,-1,"pl_cmp_3"}
  r=pl4{1,l+q>p,l*q=250,lx*qn<ln,lx+q>lx,5}
  !CheckEq{r,-1,"pl_cmp_4"}
  r=pl5{1,l+q>p,l*q=250,lx*qn<ln,lx+q>lx,5}
  !CheckEq{r,-1,"pl_cmp_5"}
  r=pl6{1,l+q>p,l*q=250,lx*qn<ln,lx+q>lx,5}
  !CheckEq{r,5,"pl_cmp_6"}
  l=100 : lx=100000 : ln=-100000 : lm=2147483647 : lz=0 : lb=2147483647
  q=2.5 : qn=-1.75 : qs=1 : p=1.25 : qm=-32768
  w=3 : wn=-7 : b=-3 : r=0 : x=0
  qs=qs/65536
  ; the fast paths of a 16.16 value with a constant divisor or factor, or a Word
  r=q/2+l
  !CheckEq{r,101,"fast_r_q_div2_add_l"}
  r=qn/2+l
  ; before: 100
  !CheckEq{r,99,"fast_r_qn_div2_add_l"}
  r=qm/3+l
  ; before: -10822
  !CheckEq{r,-10823,"fast_r_qm_div3_add_l"}
  r=qs/-2+l
  ; before: 100
  !CheckEq{r,99,"fast_r_qs_divm2_add_l"}
  r=q/10+lx
  !CheckEq{r,100000,"fast_r_q_div10_add_lx"}
  r=qn/-3+ln
  !CheckEq{r,-100000,"fast_r_qn_divm3_add_ln"}
  r=qm/65535+l
  ; before: 100
  !CheckEq{r,99,"fast_r_qm_div65535_add_l"}
  r=qn/7-l
  ; before: -100
  !CheckEq{r,-101,"fast_r_qn_div7_sub_l"}
  r=q*0.5+l
  !CheckEq{r,101,"fast_r_q_mulh_add_l"}
  r=qn*-0.5+l
  !CheckEq{r,100,"fast_r_qn_mulmh_add_l"}
  r=qm*0.25+l
  !CheckEq{r,-8092,"fast_r_qm_mulq_add_l"}
  r=qn*0.3+l
  !CheckEq{r,99,"fast_r_qn_mul03_add_l"}
  r=0.75*qn+l
  !CheckEq{r,98,"fast_r_c_mul_qn_add_l"}
  r=q*w+l
  !CheckEq{r,107,"fast_r_q_mul_w_add_l"}
  r=wn*qn+l
  !CheckEq{r,112,"fast_r_wn_mul_qn_add_l"}
  r=qm*w+lx
  !CheckEq{r,1696,"fast_r_qm_mul_w_add_lx"}
  r=q*3+l
  !CheckEq{r,107,"fast_r_q_mul3_add_l"}
  r=-7*qn+l
  !CheckEq{r,112,"fast_r_c7_mul_qn_add_l"}
  r=qm*-32768+l
  !CheckEq{r,1073741924,"fast_r_qm_mulm32768_add_l"}
  r=b*q+l
  !CheckEq{r,92,"fast_r_b_mul_q_add_l"}
  r=qm*wn-lb
  !CheckEq{r,-2147254271,"fast_r_qm_mul_wn_sub_lb"}
  r=q/2
  !CheckEq{r,1,"fast_r_q_div2"}
  r=qn/2
  ; before: 0
  !CheckEq{r,-1,"fast_r_qn_div2"}
  r=qm/3
  ; before: -10922
  !CheckEq{r,-10923,"fast_r_qm_div3"}
  r=qn/-2
  !CheckEq{r,0,"fast_r_qn_divm2"}
  r=qs/2
  !CheckEq{r,0,"fast_r_qs_div2"}
  r=q*w
  !CheckEq{r,7,"fast_r_q_mul_w"}
  r=qn*wn
  !CheckEq{r,12,"fast_r_qn_mul_wn"}
  r=q*0.5
  !CheckEq{r,1,"fast_r_q_mulh"}
  r=qn*0.3
  !CheckEq{r,0,"fast_r_qn_mul03"}
  r=qm*w
  !CheckEq{r,-98304,"fast_r_qm_mul_w"}
  x=q/2+l
  !CheckEq{Peek.l(&x),6635520,"fast_x_q_div2_add_l"}
  x=qn/3-l
  ; before: -6591829
  !CheckEq{Peek.l(&x),-6591830,"fast_x_qn_div3_sub_l"}
  x=q*w+lx
  ; before: -2035843072
  !CheckEq{Peek.l(&x),2147483647,"fast_x_q_mul_w_add_lx"}
  x=qn*0.5+l
  !CheckEq{Peek.l(&x),6496256,"fast_x_qn_mulh_add_l"}
  x=q*w
  !CheckEq{Peek.l(&x),491520,"fast_x_q_mul_w"}
  x=qm/2+lb
  ; before: -1073807360
  !CheckEq{Peek.l(&x),2147483647,"fast_x_qm_div2_add_lb"}
  x=qn*wn+l
  !CheckEq{Peek.l(&x),7356416,"fast_x_qn_mul_wn_add_l"}
  x=(qn/2)*l
  !CheckEq{Peek.l(&x),-5734400,"fast_x_qn_div2_mul_l"}
  x=-(q/2)+l
  !CheckEq{Peek.l(&x),6471680,"fast_x_neg_q_div2_add_l"}
  r=0 : If qn/2=Int(qn/2) Then r=1
  !CheckEq{r,0,"fast_if_inteq"}
  r=0 : If q*2/2=Int(q) Then r=1
  !CheckEq{r,0,"fast_if_inteq2"}
  r=0 : If q/2<l Then r=1
  !CheckEq{r,1,"fast_if_div_lt"}
  r=0 : If q*w>l Then r=1
  !CheckEq{r,0,"fast_if_mulw_gt"}
  r=0 : If qn*0.5<qn Then r=1
  !CheckEq{r,0,"fast_if_mulh_lt"}
  r=0 : If qm/2=-16384 Then r=1
  !CheckEq{r,1,"fast_if_div_eq"}
  ; a whole 16.16 divisor at run time
  l.l=100 : lx.l=100000 : ln.l=-100000 : lb.l=2147483647
  q.q=2.5 : qn.q=-1.75 : qm.q=-32768 : r.l=0 : x.q=0
  qw.q=4 : qnw.q=-3 : qone.q=1 : qz.q=0 : qbig.q=32767
  r=l/qw
  !CheckEq{r,25,"wdiv_r_l_div_qw"}
  r=ln/qnw
  !CheckEq{r,33333,"wdiv_r_ln_div_qnw"}
  r=lx/qone
  !CheckEq{r,100000,"wdiv_r_lx_div_qone"}
  r=lb/qw
  !CheckEq{r,536870911,"wdiv_r_lb_div_qw"}
  r=lb/qm
  ; before: -65535
  !CheckEq{r,-65536,"wdiv_r_lb_div_qm"}
  r=ln/qbig
  ; before: -3
  !CheckEq{r,-4,"wdiv_r_ln_div_qbig"}
  r=(l+q)/qnw
  ; before: -34
  !CheckEq{r,-35,"wdiv_r_lpq_div_qnw"}
  r=(lb+q)/qnw
  ; before: -715827883
  !CheckEq{r,-715827884,"wdiv_r_lbpq_div_qnw"}
  r=l/q
  !CheckEq{r,40,"wdiv_r_l_div_q"}
  ; a divisor 0 (r=l/qz, before: 2147483647 and -2147483647 for ln) raises
  ; the division trap since task 4.7: expr-tsr-div0-trap
  r=q/qw+l
  !CheckEq{r,100,"wdiv_r_q_div_qw_add_l"}
  r=l/qw+q
  !CheckEq{r,27,"wdiv_r_l_div_qw_add_q"}
  x=l/qw
  !CheckEq{Peek.l(&x),1638400,"wdiv_x_l_div_qw"}
  x=q/qw
  !CheckEq{Peek.l(&x),40960,"wdiv_x_q_div_qw"}
  x=qn/qnw
  !CheckEq{Peek.l(&x),38229,"wdiv_x_qn_div_qnw"}
  x=qm/qm
  !CheckEq{Peek.l(&x),65536,"wdiv_x_qm_div_qm"}
  ; x=l/qz (before: $7FFFFFFF, -$7FFFFFFF for ln): the division trap
  x=l/q
  !CheckEq{Peek.l(&x),2621440,"wdiv_x_l_div_q"}
  x=lx/qnw
  ; before: 2110433963
  !CheckEq{Peek.l(&x),-2147483648,"wdiv_x_lx_div_qnw"}
  x=(l+q)/qw
  !CheckEq{Peek.l(&x),1679360,"wdiv_x_lpq_div_qw"}
  x=l/qbig
  !CheckEq{Peek.l(&x),200,"wdiv_x_l_div_qbig"}
  r=0 : If l/qw=25 Then r=1
  !CheckEq{r,1,"wdiv_if_div_eq"}
  r=0 : If ln/qnw<33334 Then r=1
  !CheckEq{r,1,"wdiv_if_div_lt"}
  r=0 : If q/qw=0.625 Then r=1
  !CheckEq{r,1,"wdiv_if_div_frac"}
  ; a constant with a fraction beyond the Quick range beside a 48.16 value (#1220)
  lc.l=100 : lxc.l=100000 : qc.q=2.5
  r=0 : If lc+qc-lxc>=-99898.5 Then r=1
  !CheckEq{r,1,"bigconst_if_ge"}
  r=(lc+qc-lxc>=-99898.5)
  !CheckEq{r,-1,"bigconst_val_ge"}
  r=(lc+qc+lxc<100103.5)
  !CheckEq{r,-1,"bigconst_val_lt"}
  ; Mod by a whole divisor: a Quick at run time, a constant, a 48.16 dividend
  qx.q=0 : Poke.l &qx,$7FFFFFFF
  x=q Mod qw
  !CheckEq{Peek.l(&x),163840,"wmod_x_q_mod_qw"}
  x=qn Mod qw
  !CheckEq{Peek.l(&x),147456,"wmod_x_qn_mod_qw"}
  x=q Mod qnw
  ; before task 4.7: 163840
  !CheckEq{Peek.l(&x),-32768,"wmod_x_q_mod_qnw"}
  x=qn Mod qnw
  ; before task 4.7: -311296
  !CheckEq{Peek.l(&x),-114688,"wmod_x_qn_mod_qnw"}
  x=qm Mod qw
  !CheckEq{Peek.l(&x),0,"wmod_x_qm_mod_qw"}
  x=qm Mod qnw
  ; before task 4.7: -327680
  !CheckEq{Peek.l(&x),-131072,"wmod_x_qm_mod_qnw"}
  x=qm Mod qm
  !CheckEq{Peek.l(&x),0,"wmod_x_qm_mod_qm"}
  x=qbig Mod qm
  ; before task 4.7: 2147418112
  !CheckEq{Peek.l(&x),-65536,"wmod_x_qbig_mod_qm"}
  x=qn Mod qm
  ; before task 4.7: 2147368960
  !CheckEq{Peek.l(&x),-114688,"wmod_x_qn_mod_qm"}
  x=qx Mod qbig
  !CheckEq{Peek.l(&x),65535,"wmod_x_qx_mod_qbig"}
  x=qx Mod qone
  !CheckEq{Peek.l(&x),65535,"wmod_x_qx_mod_qone"}
  x=qn Mod qone
  !CheckEq{Peek.l(&x),16384,"wmod_x_qn_mod_qone"}
  x=qm Mod qone
  !CheckEq{Peek.l(&x),0,"wmod_x_qm_mod_qone"}
  x=qn Mod p
  !CheckEq{Peek.l(&x),49152,"wmod_x_qn_mod_p"}
  x=qn Mod qs
  !CheckEq{Peek.l(&x),0,"wmod_x_qn_mod_qs"}
  r=qn Mod qw
  !CheckEq{r,2,"wmod_r_qn_mod_qw"}
  r=qn Mod qnw
  ; before task 4.7: -5
  !CheckEq{r,-2,"wmod_r_qn_mod_qnw"}
  x=q Mod 3
  !CheckEq{Peek.l(&x),163840,"wmod_x_q_mod_c3"}
  x=qn Mod 3
  !CheckEq{Peek.l(&x),81920,"wmod_x_qn_mod_c3"}
  x=qn Mod -3
  ; before task 4.7: -311296
  !CheckEq{Peek.l(&x),-114688,"wmod_x_qn_mod_cn3"}
  x=qm Mod 10
  !CheckEq{Peek.l(&x),131072,"wmod_x_qm_mod_c10"}
  x=qm Mod -32768
  !CheckEq{Peek.l(&x),0,"wmod_x_qm_mod_cm"}
  x=qn Mod -32768
  ; before task 4.7: 2147368960
  !CheckEq{Peek.l(&x),-114688,"wmod_x_qn_mod_cm"}
  x=qx Mod 32767
  !CheckEq{Peek.l(&x),65535,"wmod_x_qx_mod_cbig"}
  x=qn Mod 1
  !CheckEq{Peek.l(&x),16384,"wmod_x_qn_mod_c1"}
  x=qn Mod -1
  ; before task 4.7: -114688
  !CheckEq{Peek.l(&x),-49152,"wmod_x_qn_mod_cn1"}
  x=q Mod 2.0
  !CheckEq{Peek.l(&x),32768,"wmod_x_q_mod_c2q"}
  x=qn Mod w
  !CheckEq{Peek.l(&x),81920,"wmod_x_qn_mod_w"}
  x=qn Mod wn
  ; before task 4.7: -573440
  !CheckEq{Peek.l(&x),-114688,"wmod_x_qn_mod_wn"}
  r=qn Mod 3
  !CheckEq{r,1,"wmod_r_qn_mod_c3"}
  x=l Mod qw
  !CheckEq{Peek.l(&x),0,"wmod_x_l_mod_qw"}
  x=lx Mod qnw
  ; before task 4.7: 65536
  !CheckEq{Peek.l(&x),-131072,"wmod_x_lx_mod_qnw"}
  x=ln Mod qw
  !CheckEq{Peek.l(&x),0,"wmod_x_ln_mod_qw"}
  x=ln Mod qnw
  ; before task 4.7: -262144
  !CheckEq{Peek.l(&x),-65536,"wmod_x_ln_mod_qnw"}
  x=lb Mod qbig
  !CheckEq{Peek.l(&x),65536,"wmod_x_lb_mod_qbig"}
  x=lb Mod qm
  ; before task 4.7: 2147418112
  !CheckEq{Peek.l(&x),-65536,"wmod_x_lb_mod_qm"}
  x=(l+q) Mod qnw
  ; before task 4.7: 32768
  !CheckEq{Peek.l(&x),-163840,"wmod_x_lpq_mod_qnw"}
  x=(ln-q) Mod qw
  !CheckEq{Peek.l(&x),98304,"wmod_x_lnmq_mod_qw"}
  x=(ln-q) Mod qnw
  ; before task 4.7: -229376
  !CheckEq{Peek.l(&x),-32768,"wmod_x_lnmq_mod_qnw"}
  x=(lb+lb+q) Mod qnw
  ; before: 98304
  ; before task 4.7: 32768
  !CheckEq{Peek.l(&x),-163840,"wmod_x_lblbq_mod_qnw"}
  x=(lb*qbig) Mod qm
  ; before task 4.7: 65536
  !CheckEq{Peek.l(&x),-2147418112,"wmod_x_lbmulqbig_mod_qm"}
  ; x=l Mod qz (before: 6553600, the dividend): the division trap
  x=l Mod q
  !CheckEq{Peek.l(&x),0,"wmod_x_l_mod_q"}
  x=lx Mod qone
  !CheckEq{Peek.l(&x),0,"wmod_x_lx_mod_qone"}
  r=ln Mod qnw
  ; before task 4.7: -4
  !CheckEq{r,-1,"wmod_r_ln_mod_qnw"}
  r=(ln-q) Mod qw
  !CheckEq{r,1,"wmod_r_lnmq_mod_qw"}
  r=(l+q) Mod 3
  !CheckEq{r,0,"wmod_r_lpq_mod_c3"}
  r=pq3{q,p,qn Mod qw,ln Mod qnw,qn Mod 3,l}
  !CheckEq{r,147456,"wmod_pq3"}
  r=pq4{q,p,qn Mod qw,ln Mod qnw,qn Mod 3,l}
  ; before task 4.7: -262144
  !CheckEq{r,-65536,"wmod_pq4"}
  r=pq5{q,p,qn Mod qw,ln Mod qnw,qn Mod 3,l}
  !CheckEq{r,81920,"wmod_pq5"}
  r=pq6{q,p,qn Mod qw,ln Mod qnw,qn Mod 3,qm Mod qnw}
  ; before task 4.7: -327680
  !CheckEq{r,-131072,"wmod_pq6"}
  r=0 : If qn Mod qw=2.25 Then r=1
  !CheckEq{r,1,"wmod_if_eq"}
  r=0 : If ln Mod qnw<-3 Then r=1
  ; before task 4.7: 1
  !CheckEq{r,0,"wmod_if_lt"}
  ; a whole divisor at run time: a whole dividend not widened, a leading
  ; zero digit skipped, a / at the top under a wanted Long as a Long
  lmn.l=-2147483647 : lmn=lmn-1
  r=lx/qw
  !CheckEq{r,25000,"wd2_r_lx_div_qw"}
  r=lx/qnw
  ; before: -33333
  !CheckEq{r,-33334,"wd2_r_lx_div_qnw"}
  r=w/qw
  !CheckEq{r,0,"wd2_r_w_div_qw"}
  r=wn/qnw
  !CheckEq{r,2,"wd2_r_wn_div_qnw"}
  r=b/qnw
  !CheckEq{r,1,"wd2_r_b_div_qnw"}
  r=lm/qone
  !CheckEq{r,2147483647,"wd2_r_lm_div_qone"}
  r=lmn/qone
  !CheckEq{r,-2147483648,"wd2_r_lmn_div_qone"}
  r=lmn/qm
  !CheckEq{r,65536,"wd2_r_lmn_div_qm"}
  r=(lb+lb+q)/qnw
  ; before: 0
  !CheckEq{r,-1,"wd2_r_lblbq_div_qnw"}
  r=(lx*qbig)/qw
  !CheckEq{r,819175000,"wd2_r_lxqbig_div_qw"}
  r=(l+q)/qw
  !CheckEq{r,25,"wd2_r_lpq_div_qw"}
  r=(ln-q)/qw
  ; before: -25000
  !CheckEq{r,-25001,"wd2_r_lnmq_div_qw"}
  r=(ln-q)/q
  !CheckEq{r,-40001,"wd2_r_lnmq_div_q"}
  r=ln/q
  !CheckEq{r,-40000,"wd2_r_ln_div_q"}
  x=lx/qw
  !CheckEq{Peek.l(&x),1638400000,"wd2_x_lx_div_qw"}
  x=ln/qw
  !CheckEq{Peek.l(&x),-1638400000,"wd2_x_ln_div_qw"}
  x=lmn/qnw
  ; before: -1431655766
  !CheckEq{Peek.l(&x),2147483647,"wd2_x_lmn_div_qnw"}
  x=wn/qnw
  !CheckEq{Peek.l(&x),152917,"wd2_x_wn_div_qnw"}
  x=(lb+lb+q)/qnw
  ; before: -1431666688
  !CheckEq{Peek.l(&x),-10923,"wd2_x_lblbq_div_qnw"}
  x=(lx*qbig)/qnw
  ; before: -678778197
  !CheckEq{Peek.l(&x),-2147483648,"wd2_x_lxqbig_div_qnw"}
  r=pl3{l,l,lx/qw,l,l,l}
  !CheckEq{r,25000,"wd2_pl3"}
  r=pl4{l,l,l,ln/qnw,l,l}
  !CheckEq{r,33333,"wd2_pl4"}
  r=pl5{l,l,l,l,(lb+lb+q)/qnw,l}
  ; before: 0
  !CheckEq{r,-1,"wd2_pl5"}
  r=pl6{l,l,l,l,l,ln/q}
  !CheckEq{r,-40000,"wd2_pl6"}
  r=pq4{q,p,q,lx/qnw,q,q}
  ; before: 2110433963
  !CheckEq{r,-2147483648,"wd2_pq4"}
  x=ln Mod qm
  ; before: 2036334592
  ; before task 4.7: -2147483648
  !CheckEq{Peek.l(&x),-111149056,"wm2_x_ln_mod_qm"}
  r=ln Mod qm
  ; before task 4.7: -34464
  !CheckEq{r,-1696,"wm2_r_ln_mod_qm"}
  x=wn Mod qnw
  ; before task 4.7: -262144
  !CheckEq{Peek.l(&x),-65536,"wm2_x_wn_mod_qnw"}
  r=b Mod qw
  !CheckEq{r,1,"wm2_r_b_mod_qw"}
  x=lmn Mod qnw
  ; before task 4.7: -327680
  !CheckEq{Peek.l(&x),-131072,"wm2_x_lmn_mod_qnw"}
  x=lb Mod qnw
  ; before task 4.7: 65536
  !CheckEq{Peek.l(&x),-131072,"wm2_x_lb_mod_qnw"}
  r=lmn Mod qm
  !CheckEq{r,0,"wm2_r_lmn_mod_qm"}
  x=lx Mod qbig
  !CheckEq{Peek.l(&x),111345664,"wm2_x_lx_mod_qbig"}
  x=(lb+lb+q) Mod qbig
  ; before: 294912
  !CheckEq{Peek.l(&x),32768,"wm2_x_lblbq_mod_qbig"}
  x=(lx*qbig) Mod qnw
  ; before task 4.7: 65536
  !CheckEq{Peek.l(&x),-131072,"wm2_x_lxqbig_mod_qnw"}
  r=pq5{q,p,q,p,ln Mod qm,q}
  ; before: 2036334592
  ; before task 4.7: -2147483648
  !CheckEq{r,-111149056,"wm2_pq5"}
  r=pl4{l,l,l,lx Mod qnw,l,l}
  ; before task 4.7: 1
  !CheckEq{r,-2,"wm2_pl4"}
  ; Mod with a fraction: the exact 16.16 Mod without q48mod
  q1.q=12.5 : q2.q=3.25 : h75.q=0.75 : hn75.q=-0.75 : qmx.q=0 : hn.q=-0.5 : qt.q=1 : qt=qt/65536
  Poke.l &qmx,$7FFFFFFF : qbh.q=0 : Poke.l &qbh,$40004000 : qmn.q=0 : Poke.l &qmn,$80000001
  x=q1 Mod q2
  !CheckEq{Peek.l(&x),180224,"qm_x_q1_mod_q2"}
  x=q Mod p
  !CheckEq{Peek.l(&x),0,"qm_x_q_mod_p"}
  x=qn Mod hn
  ; before task 4.7: -49152
  !CheckEq{Peek.l(&x),-16384,"qm_x_qn_mod_hn"}
  x=qn Mod h75
  !CheckEq{Peek.l(&x),32768,"qm_x_qn_mod_h75"}
  x=qn Mod hn75
  ; before task 4.7: -65536
  !CheckEq{Peek.l(&x),-16384,"qm_x_qn_mod_hn75"}
  x=qm Mod p
  !CheckEq{Peek.l(&x),49152,"qm_x_qm_mod_p"}
  x=qm Mod qt
  !CheckEq{Peek.l(&x),0,"qm_x_qm_mod_qt"}
  x=qbig Mod qt
  !CheckEq{Peek.l(&x),0,"qm_x_qbig_mod_qt"}
  x=qm Mod hn
  !CheckEq{Peek.l(&x),0,"qm_x_qm_mod_hn"}
  x=qm Mod qmx
  !CheckEq{Peek.l(&x),2147483646,"qm_x_qm_mod_qmx"}
  x=qmx Mod qmn
  !CheckEq{Peek.l(&x),0,"qm_x_qmx_mod_qmn"}
  x=qmx Mod qbh
  !CheckEq{Peek.l(&x),1073725439,"qm_x_qmx_mod_qbh"}
  x=qm Mod qbh
  !CheckEq{Peek.l(&x),32768,"qm_x_qm_mod_qbh"}
  x=qmn Mod qmx
  !CheckEq{Peek.l(&x),0,"qm_x_qmn_mod_qmx"}
  x=qbh Mod qn
  ; before task 4.7: 49152
  !CheckEq{Peek.l(&x),-65536,"qm_x_qbh_mod_qn"}
  x=q Mod 0.75
  !CheckEq{Peek.l(&x),16384,"qm_x_q_mod_c075"}
  x=qn Mod 0.75
  !CheckEq{Peek.l(&x),32768,"qm_x_qn_mod_c075"}
  x=qn Mod -0.75
  ; before task 4.7: -65536
  !CheckEq{Peek.l(&x),-16384,"qm_x_qn_mod_cn075"}
  x=q Mod 1.5
  !CheckEq{Peek.l(&x),65536,"qm_x_q_mod_c15"}
  x=qn Mod 1.5
  !CheckEq{Peek.l(&x),81920,"qm_x_qn_mod_c15"}
  x=qm Mod 1.5
  !CheckEq{Peek.l(&x),65536,"qm_x_qm_mod_c15"}
  x=qbig Mod 2.5
  !CheckEq{Peek.l(&x),131072,"qm_x_qbig_mod_c25"}
  x=qmx Mod 16384.25
  !CheckEq{Peek.l(&x),1073725439,"qm_x_qmx_mod_c16384"}
  x=qm Mod -1.5
  ; before task 4.7: -131072
  !CheckEq{Peek.l(&x),-32768,"qm_x_qm_mod_cn15"}
  r=qn Mod p
  !CheckEq{r,0,"qm_r_qn_mod_p"}
  r=q1 Mod q2
  !CheckEq{r,2,"qm_r_q1_mod_q2"}
  r=pq3{q,p,q1 Mod q2,q,q,q}
  !CheckEq{r,180224,"qm_pq3"}
  r=pq4{q,p,q,qn Mod 0.75,q,q}
  !CheckEq{r,32768,"qm_pq4"}
  r=pq6{q,p,q,p,q,qm Mod qbh}
  !CheckEq{r,32768,"qm_pq6"}
  ; a Long times a Quick in 48.16 not rounded: compilerlib's q48mull
  r=0 : If l*q>p Then r=1
  !CheckEq{r,1,"lq_if_l_mul_q"}
  r=0 : If lx*q>p Then r=1
  !CheckEq{r,1,"lq_if_lx_mul_q"}
  r=0 : If ln*qn>p Then r=1
  !CheckEq{r,1,"lq_if_ln_mul_qn"}
  r=0 : If l*qn<p Then r=1
  !CheckEq{r,1,"lq_if_l_mul_qn"}
  r=0 : If lx*qm<p Then r=1
  !CheckEq{r,1,"lq_if_lx_mul_qm"}
  r=0 : If lb*qm<-1000000 Then r=1
  !CheckEq{r,1,"lq_if_lb_mul_qm"}
  r=0 : If lmn*qm>1000000 Then r=1
  !CheckEq{r,1,"lq_if_lmn_mul_qm"}
  r=0 : If qt*lx>1.5 Then r=1
  !CheckEq{r,1,"lq_if_qt_mul_lx"}
  r=lx*q+p
  !CheckEq{r,250001,"lq_r_lx_mul_q_add_p"}
  r=ln*qn-p
  !CheckEq{r,174998,"lq_r_ln_mul_qn_sub_p"}
  r=lb*qm+p
  !CheckEq{r,32769,"lq_r_lb_mul_qm_add_p"}
  r=(lx*qn)/qw
  !CheckEq{r,-43750,"lq_r_lx_mul_qn_div_qw"}
  r=l*q*p
  !CheckEq{r,312,"lq_r_l_mul_q_mul_p"}
  r=w*2.5*p
  !CheckEq{r,9,"lq_r_w_mul_c_mul_p"}
  x=lx*q+p
  ; before: -795787264
  !CheckEq{Peek.l(&x),2147483647,"lq_x_lx_mul_q_add_p"}
  x=ln*qt+p
  !CheckEq{Peek.l(&x),-18080,"lq_x_ln_mul_qt_add_p"}
  r=pl3{l,l,lx*q+p,l,l,l}
  !CheckEq{r,250001,"lq_pl3"}
  r=pl5{l,l,l,l,ln*qn-p,l}
  !CheckEq{r,174998,"lq_pl5"}
  ; two Quicks multiplied under a wanted Long: compilerlib's qqmult
  ; (the exact floor; qt*hn has the values of #1221)
  hn.q=-0.5 : qt.q=1 : qt=qt/65536
  r=q*p
  !CheckEq{r,3,"qq_r_q_mul_p"}
  r=qn*p
  !CheckEq{r,-3,"qq_r_qn_mul_p"}
  r=qn*q
  !CheckEq{r,-5,"qq_r_qn_mul_q"}
  r=q*qw
  !CheckEq{r,10,"qq_r_q_mul_qw"}
  r=qn*qw
  !CheckEq{r,-7,"qq_r_qn_mul_qw"}
  r=qw*qnw
  !CheckEq{r,-12,"qq_r_qw_mul_qnw"}
  r=qm*qm
  !CheckEq{r,1073741824,"qq_r_qm_mul_qm"}
  r=qm*qbig
  !CheckEq{r,-1073709056,"qq_r_qm_mul_qbig"}
  r=qt*hn
  !CheckEq{r,-1,"qq_r_qt_mul_hn"}
  r=hn*qt
  !CheckEq{r,-1,"qq_r_hn_mul_qt"}
  r=qt*qn
  !CheckEq{r,-1,"qq_r_qt_mul_qn"}
  r=q*1.5
  !CheckEq{r,3,"qq_r_q_mul_c"}
  r=qn*1.5
  !CheckEq{r,-2,"qq_r_qn_mul_c"}
  r=q*0.5
  !CheckEq{r,1,"qq_r_q_mul_half"}
  r=q*p+l
  !CheckEq{r,103,"qq_r_q_mul_p_add_l"}
  r=qn*p+l
  !CheckEq{r,97,"qq_r_qn_mul_p_add_l"}
  r=l+qn*p
  !CheckEq{r,97,"qq_r_l_add_qn_mul_p"}
  r=qn*p-l
  !CheckEq{r,-103,"qq_r_qn_mul_p_sub_l"}
  r=l-qn*p
  !CheckEq{r,102,"qq_r_l_sub_qn_mul_p"}
  r=qt*hn+l
  !CheckEq{r,99,"qq_r_qt_mul_hn_add_l"}
  r=qm*qm+lb
  !CheckEq{r,-1073741825,"qq_r_qm_mul_qm_add_lb"}
  x=q*p+l
  !CheckEq{Peek.l(&x),6758400,"qq_x_q_mul_p_add_l"}
  x=qn*p-l
  !CheckEq{Peek.l(&x),-6696960,"qq_x_qn_mul_p_sub_l"}
  x=l+qm*qm
  ; before: 6553600
  !CheckEq{Peek.l(&x),2147483647,"qq_x_l_add_qm_mul_qm"}
  r=pl3{l,l,q*p,l,l,l}
  !CheckEq{r,3,"qq_pl3"}
  r=pl4{l,l,l,qn*p+l,l,l}
  !CheckEq{r,97,"qq_pl4"}
  r=pq5{q,p,q,p,qn*p+l,q}
  !CheckEq{r,6410240,"qq_pq5"}
!EndTestCase
