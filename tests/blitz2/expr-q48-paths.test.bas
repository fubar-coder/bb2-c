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
; the operations in parameters (registers d1 to d5).  Each value is the
; one the code of task 4.2 gives (all its 48.16 operations through
; compilerlib's routines, measured once; the Mod checks on main ad8396cf9,
; every such Mod through q48mod); a regression test of the fixed port, no
; key, as select-fraction.test.bas.
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
  !CheckEq{Peek.l(&x),-795869184,"x_lx_mul_q"}
  x=ln*qn
  !CheckEq{Peek.l(&x),-1416101888,"x_ln_mul_qn"}
  x=q*w
  !CheckEq{Peek.l(&x),491520,"x_q_mul_w"}
  x=wn*qn
  !CheckEq{Peek.l(&x),802816,"x_wn_mul_qn"}
  x=l*2.5
  !CheckEq{Peek.l(&x),16384000,"x_l_mul_c"}
  x=3.5*l
  !CheckEq{Peek.l(&x),22937600,"x_c_mul_l"}
  x=lm*q
  !CheckEq{Peek.l(&x),-163840,"x_lm_mul_q"}
  x=lm*qn
  !CheckEq{Peek.l(&x),114688,"x_lm_mul_qn"}
  x=ln*qs
  !CheckEq{Peek.l(&x),-100000,"x_ln_mul_qs"}
  x=l*qm
  !CheckEq{Peek.l(&x),0,"x_l_mul_qm"}
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
  !CheckEq{Peek.l(&x),-2036170752,"x_lx_add_q"}
  x=l+q-lx
  !CheckEq{Peek.l(&x),2043052032,"x_l_add_q_sub_lx"}
  x=lm+qn
  !CheckEq{Peek.l(&x),-180224,"x_lm_add_qn"}
  x=qn-lm
  !CheckEq{Peek.l(&x),-49152,"x_qn_sub_lm"}
  x=l+qs-p
  !CheckEq{Peek.l(&x),6471681,"x_l_add_qs_sub_p"}
  x=-l+q
  !CheckEq{Peek.l(&x),-6389760,"x_negl_add_q"}
  x=(l+q)-(lx-p)
  !CheckEq{Peek.l(&x),2043133952,"x_brackets_sub"}
  x=(l-q)+(p-ln)
  !CheckEq{Peek.l(&x),-2029862912,"x_brackets_add"}
  x=w+l+q
  !CheckEq{Peek.l(&x),6914048,"x_w_add_l_add_q"}
  x=l+1.25+q
  !CheckEq{Peek.l(&x),6799360,"x_l_add_c_add_q"}
  x=l+q*p
  !CheckEq{Peek.l(&x),6758400,"x_l_add_q_mul_p"}
  x=l*q+p
  !CheckEq{Peek.l(&x),16465920,"x_l_mul_q_add_p"}
  x=q+lx*p
  !CheckEq{Peek.l(&x),-397770752,"x_q_add_lx_mul_p"}
  x=l/q+p
  !CheckEq{Peek.l(&x),2703360,"x_l_div_q_add_p"}
  x=lb+q
  !CheckEq{Peek.l(&x),98304,"x_lb_add_q"}
  x=lb*qn
  !CheckEq{Peek.l(&x),114688,"x_lb_mul_qn"}
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
  !CheckEq{r,-795869184,"pq_a_2"}
  r=pq3{l*q,lx*q,l+q,q-l,l*qn,lx*qn}
  !CheckEq{r,6717440,"pq_a_3"}
  r=pq4{l*q,lx*q,l+q,q-l,l*qn,lx*qn}
  !CheckEq{r,-6389760,"pq_a_4"}
  r=pq5{l*q,lx*q,l+q,q-l,l*qn,lx*qn}
  !CheckEq{r,-11468800,"pq_a_5"}
  r=pq6{l*q,lx*q,l+q,q-l,l*qn,lx*qn}
  !CheckEq{r,1416101888,"pq_a_6"}
  r=pq1{q,p,l*q,lx*q,ln*qn,lx+q}
  !CheckEq{r,163840,"pq_b_1"}
  r=pq2{q,p,l*q,lx*q,ln*qn,lx+q}
  !CheckEq{r,81920,"pq_b_2"}
  r=pq3{q,p,l*q,lx*q,ln*qn,lx+q}
  !CheckEq{r,16384000,"pq_b_3"}
  r=pq4{q,p,l*q,lx*q,ln*qn,lx+q}
  !CheckEq{r,-795869184,"pq_b_4"}
  r=pq5{q,p,l*q,lx*q,ln*qn,lx+q}
  !CheckEq{r,-1416101888,"pq_b_5"}
  r=pq6{q,p,l*q,lx*q,ln*qn,lx+q}
  !CheckEq{r,-2036170752,"pq_b_6"}
  r=pq1{q,p,qn,l+q-p,lx-q,l*q}
  !CheckEq{r,163840,"pq_c_1"}
  r=pq2{q,p,qn,l+q-p,lx-q,l*q}
  !CheckEq{r,81920,"pq_c_2"}
  r=pq3{q,p,qn,l+q-p,lx-q,l*q}
  !CheckEq{r,-114688,"pq_c_3"}
  r=pq4{q,p,qn,l+q-p,lx-q,l*q}
  !CheckEq{r,6635520,"pq_c_4"}
  r=pq5{q,p,qn,l+q-p,lx-q,l*q}
  !CheckEq{r,-2036498432,"pq_c_5"}
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
  !CheckEq{r,100,"fast_r_qn_div2_add_l"}
  r=qm/3+l
  !CheckEq{r,-10822,"fast_r_qm_div3_add_l"}
  r=qs/-2+l
  !CheckEq{r,100,"fast_r_qs_divm2_add_l"}
  r=q/10+lx
  !CheckEq{r,100000,"fast_r_q_div10_add_lx"}
  r=qn/-3+ln
  !CheckEq{r,-100000,"fast_r_qn_divm3_add_ln"}
  r=qm/65535+l
  !CheckEq{r,100,"fast_r_qm_div65535_add_l"}
  r=qn/7-l
  !CheckEq{r,-100,"fast_r_qn_div7_sub_l"}
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
  !CheckEq{r,0,"fast_r_qn_div2"}
  r=qm/3
  !CheckEq{r,-10922,"fast_r_qm_div3"}
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
  !CheckEq{Peek.l(&x),-6591829,"fast_x_qn_div3_sub_l"}
  x=q*w+lx
  !CheckEq{Peek.l(&x),-2035843072,"fast_x_q_mul_w_add_lx"}
  x=qn*0.5+l
  !CheckEq{Peek.l(&x),6496256,"fast_x_qn_mulh_add_l"}
  x=q*w
  !CheckEq{Peek.l(&x),491520,"fast_x_q_mul_w"}
  x=qm/2+lb
  !CheckEq{Peek.l(&x),-1073807360,"fast_x_qm_div2_add_lb"}
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
  !CheckEq{r,-65535,"wdiv_r_lb_div_qm"}
  r=ln/qbig
  !CheckEq{r,-3,"wdiv_r_ln_div_qbig"}
  r=(l+q)/qnw
  !CheckEq{r,-34,"wdiv_r_lpq_div_qnw"}
  r=(lb+q)/qnw
  !CheckEq{r,-715827883,"wdiv_r_lbpq_div_qnw"}
  r=l/q
  !CheckEq{r,40,"wdiv_r_l_div_q"}
  r=l/qz
  !CheckEq{r,2147483647,"wdiv_r_l_div_qz"}
  r=ln/qz
  !CheckEq{r,-2147483647,"wdiv_r_ln_div_qz"}
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
  x=l/qz
  !CheckEq{Peek.l(&x),2147483647,"wdiv_x_l_div_qz"}
  x=ln/qz
  !CheckEq{Peek.l(&x),-2147483647,"wdiv_x_ln_div_qz"}
  x=l/q
  !CheckEq{Peek.l(&x),2621440,"wdiv_x_l_div_q"}
  x=lx/qnw
  !CheckEq{Peek.l(&x),2110433963,"wdiv_x_lx_div_qnw"}
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
  !CheckEq{Peek.l(&x),163840,"wmod_x_q_mod_qnw"}
  x=qn Mod qnw
  !CheckEq{Peek.l(&x),-311296,"wmod_x_qn_mod_qnw"}
  x=qm Mod qw
  !CheckEq{Peek.l(&x),0,"wmod_x_qm_mod_qw"}
  x=qm Mod qnw
  !CheckEq{Peek.l(&x),-327680,"wmod_x_qm_mod_qnw"}
  x=qm Mod qm
  !CheckEq{Peek.l(&x),0,"wmod_x_qm_mod_qm"}
  x=qbig Mod qm
  !CheckEq{Peek.l(&x),2147418112,"wmod_x_qbig_mod_qm"}
  x=qn Mod qm
  !CheckEq{Peek.l(&x),2147368960,"wmod_x_qn_mod_qm"}
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
  !CheckEq{r,-5,"wmod_r_qn_mod_qnw"}
  x=q Mod 3
  !CheckEq{Peek.l(&x),163840,"wmod_x_q_mod_c3"}
  x=qn Mod 3
  !CheckEq{Peek.l(&x),81920,"wmod_x_qn_mod_c3"}
  x=qn Mod -3
  !CheckEq{Peek.l(&x),-311296,"wmod_x_qn_mod_cn3"}
  x=qm Mod 10
  !CheckEq{Peek.l(&x),131072,"wmod_x_qm_mod_c10"}
  x=qm Mod -32768
  !CheckEq{Peek.l(&x),0,"wmod_x_qm_mod_cm"}
  x=qn Mod -32768
  !CheckEq{Peek.l(&x),2147368960,"wmod_x_qn_mod_cm"}
  x=qx Mod 32767
  !CheckEq{Peek.l(&x),65535,"wmod_x_qx_mod_cbig"}
  x=qn Mod 1
  !CheckEq{Peek.l(&x),16384,"wmod_x_qn_mod_c1"}
  x=qn Mod -1
  !CheckEq{Peek.l(&x),-114688,"wmod_x_qn_mod_cn1"}
  x=q Mod 2.0
  !CheckEq{Peek.l(&x),32768,"wmod_x_q_mod_c2q"}
  x=qn Mod w
  !CheckEq{Peek.l(&x),81920,"wmod_x_qn_mod_w"}
  x=qn Mod wn
  !CheckEq{Peek.l(&x),-573440,"wmod_x_qn_mod_wn"}
  r=qn Mod 3
  !CheckEq{r,1,"wmod_r_qn_mod_c3"}
  x=l Mod qw
  !CheckEq{Peek.l(&x),0,"wmod_x_l_mod_qw"}
  x=lx Mod qnw
  !CheckEq{Peek.l(&x),65536,"wmod_x_lx_mod_qnw"}
  x=ln Mod qw
  !CheckEq{Peek.l(&x),0,"wmod_x_ln_mod_qw"}
  x=ln Mod qnw
  !CheckEq{Peek.l(&x),-262144,"wmod_x_ln_mod_qnw"}
  x=lb Mod qbig
  !CheckEq{Peek.l(&x),65536,"wmod_x_lb_mod_qbig"}
  x=lb Mod qm
  !CheckEq{Peek.l(&x),2147418112,"wmod_x_lb_mod_qm"}
  x=(l+q) Mod qnw
  !CheckEq{Peek.l(&x),32768,"wmod_x_lpq_mod_qnw"}
  x=(ln-q) Mod qw
  !CheckEq{Peek.l(&x),98304,"wmod_x_lnmq_mod_qw"}
  x=(ln-q) Mod qnw
  !CheckEq{Peek.l(&x),-229376,"wmod_x_lnmq_mod_qnw"}
  x=(lb+lb+q) Mod qnw
  !CheckEq{Peek.l(&x),98304,"wmod_x_lblbq_mod_qnw"}
  x=(lb*qbig) Mod qm
  !CheckEq{Peek.l(&x),65536,"wmod_x_lbmulqbig_mod_qm"}
  x=l Mod qz
  !CheckEq{Peek.l(&x),6553600,"wmod_x_l_mod_qz"}
  x=l Mod q
  !CheckEq{Peek.l(&x),0,"wmod_x_l_mod_q"}
  x=lx Mod qone
  !CheckEq{Peek.l(&x),0,"wmod_x_lx_mod_qone"}
  r=ln Mod qnw
  !CheckEq{r,-4,"wmod_r_ln_mod_qnw"}
  r=(ln-q) Mod qw
  !CheckEq{r,1,"wmod_r_lnmq_mod_qw"}
  r=(l+q) Mod 3
  !CheckEq{r,0,"wmod_r_lpq_mod_c3"}
  r=pq3{q,p,qn Mod qw,ln Mod qnw,qn Mod 3,l}
  !CheckEq{r,147456,"wmod_pq3"}
  r=pq4{q,p,qn Mod qw,ln Mod qnw,qn Mod 3,l}
  !CheckEq{r,-262144,"wmod_pq4"}
  r=pq5{q,p,qn Mod qw,ln Mod qnw,qn Mod 3,l}
  !CheckEq{r,81920,"wmod_pq5"}
  r=pq6{q,p,qn Mod qw,ln Mod qnw,qn Mod 3,qm Mod qnw}
  !CheckEq{r,-327680,"wmod_pq6"}
  r=0 : If qn Mod qw=2.25 Then r=1
  !CheckEq{r,1,"wmod_if_eq"}
  r=0 : If ln Mod qnw<-3 Then r=1
  !CheckEq{r,1,"wmod_if_lt"}
!EndTestCase
