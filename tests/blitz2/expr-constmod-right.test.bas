; expr-constmod-right (#1227) - in a constant expression, a whole-number
; operation right of a fraction (a Mod) no longer cuts the fraction.
; The original types a constant expression by its flow: an operation in the
; type after its right operand.  A Mod's result is whole (a Word in a
; Quick, a Long in a Float), so 0.5+6 Mod 4 added in that whole type: the
; 0.5 was cut and the result 2 under every destination, where 0.5+w Mod 4
; with w=6 is 2.5 (rule P+ of #1216 types variables by their operands).
; Correct (the user's decision of 2026-11-27): the constant form gives the
; exact value of the variable form, the whole result raised to the left
; operand's Quick or Float; under a Byte, Word or Long destination the
; rounding rules of a constant with a point (#1222: a Float, towards zero).
!TestCase{whole}
  w.w=6 : t.q=6
  rb.b=0 : rw.w=0 : rl.l=0 : vb.b=0 : vw.w=0 : vl.l=0
  rl=0.5+6 Mod 4 : vl=0.5+w Mod 4 : !CheckEq{rl,vl,"l_add"} : !CheckEq{rl,2,"l_add_value"}
  rl=0.5-6 Mod 4 : vl=0.5-w Mod 4 : !CheckEq{rl,vl,"l_sub"} : !CheckEq{rl,-1,"l_sub_value"}
  rw=0.5-6 Mod 4 : vw=0.5-w Mod 4 : !CheckEq{rw,vw,"w_sub"} : !CheckEq{rw,-1,"w_sub_value"}
  rb=0.5-6 Mod 4 : vb=0.5-w Mod 4 : !CheckEq{rb,vb,"b_sub"} : !CheckEq{rb,-1,"b_sub_value"}
  rl=0.5*6 Mod 4 : vl=0.5*w Mod 4 : !CheckEq{rl,vl,"l_mul"} : !CheckEq{rl,1,"l_mul_value"}
  rw=0.5*6 Mod 4 : vw=0.5*w Mod 4 : !CheckEq{rw,vw,"w_mul"} : !CheckEq{rw,1,"w_mul_value"}
  rb=0.5*6 Mod 4 : vb=0.5*w Mod 4 : !CheckEq{rb,vb,"b_mul"} : !CheckEq{rb,1,"b_mul_value"}
  rl=-1.5+6.0 Mod 4 : vl=-1.5+t Mod 4 : !CheckEq{rl,vl,"l_addpoint"} : !CheckEq{rl,0,"l_addpoint_value"}
  rl=-1.5*6.0 Mod 4 : vl=-1.5*t Mod 4 : !CheckEq{rl,vl,"l_mulpoint"} : !CheckEq{rl,-3,"l_mulpoint_value"}
  rw=-1.5*6.0 Mod 4 : vw=-1.5*t Mod 4 : !CheckEq{rw,vw,"w_mulpoint"} : !CheckEq{rw,-3,"w_mulpoint_value"}
  rb=-1.5*6.0 Mod 4 : vb=-1.5*t Mod 4 : !CheckEq{rb,vb,"b_mulpoint"} : !CheckEq{rb,-3,"b_mulpoint_value"}
!EndTestCase
!TestCase{fractional}
  w.w=6 : t.q=6
  rq.q=0 : rf.f=0 : vq.q=0 : vf.f=0
  rq=0.5+6 Mod 4 : vq=0.5+w Mod 4 : !CheckQEq{rq,vq,"q_add"} : !CheckQEq{rq,2.5,"q_add_value"}
  rf=0.5+6 Mod 4 : vf=0.5+w Mod 4 : !CheckFltNear{rf,vf,0.0001,"f_add"} : !CheckFltNear{rf,2.5,0.0001,"f_add_value"}
  rq=0.5-6 Mod 4 : vq=0.5-w Mod 4 : !CheckQEq{rq,vq,"q_sub"} : !CheckQEq{rq,-1.5,"q_sub_value"}
  rf=0.5-6 Mod 4 : vf=0.5-w Mod 4 : !CheckFltNear{rf,-1.5,0.0001,"f_sub_value"}
  rq=0.5*6 Mod 4 : vq=0.5*w Mod 4 : !CheckQEq{rq,vq,"q_mul"} : !CheckQEq{rq,1,"q_mul_value"}
  rf=0.5*6 Mod 4 : vf=0.5*w Mod 4 : !CheckFltNear{rf,1,0.0001,"f_mul_value"}
  rq=-1.5+6.0 Mod 4 : vq=-1.5+t Mod 4 : !CheckQEq{rq,vq,"q_addpoint"} : !CheckQEq{rq,0.5,"q_addpoint_value"}
  rq=-1.5*6.0 Mod 4 : vq=-1.5*t Mod 4 : !CheckQEq{rq,vq,"q_mulpoint"} : !CheckQEq{rq,-3,"q_mulpoint_value"}
  rf=-1.5*6.0 Mod 4 : vf=-1.5*t Mod 4 : !CheckFltNear{rf,-3,0.0001,"f_mulpoint_value"}
  ; deeper on the right, and the other operators of a fraction and a Mod
  rq=0.5+2*(6 Mod 4) : vq=0.5+2*(w Mod 4) : !CheckQEq{rq,vq,"q_nested"} : !CheckQEq{rq,4.5,"q_nested_value"}
  rf=0.5+2*(6 Mod 4) : !CheckFltNear{rf,4.5,0.0001,"f_nested_value"}
  ; a Long right of a Quick, in the Quick range
  rq=0.5+(6 Mod 4)*2 : vq=0.5+(w Mod 4)*2 : !CheckQEq{rq,vq,"q_long"} : !CheckQEq{rq,4.5,"q_long_value"}
  rq=0.5+(40000-39999) : !CheckQEq{rq,1.5,"q_bigsub_value"}
  rq=0.5/(6 Mod 4) : vq=0.5/(w Mod 4) : !CheckQEq{rq,vq,"q_div"} : !CheckQEq{rq,0.25,"q_div_value"}
  rf=0.5/(6 Mod 4) : !CheckFltNear{rf,0.25,0.0001,"f_div_value"}
  rq=0.5 Mod (6 Mod 4) : vq=0.5 Mod (w Mod 4) : !CheckQEq{rq,vq,"q_mod"} : !CheckQEq{rq,0.5,"q_mod_value"}
  ; the operands swapped were right already
  rq=(6 Mod 4)+0.5 : !CheckQEq{rq,2.5,"q_swapped_value"}
  !CheckStrEq{Str$(0.5+6 Mod 4),Str$(0.5+w Mod 4),"s_add"}
!EndTestCase
