; expr-top-div-trunc (#1222) - a / at the top and a constant with a point
; under a wanted Byte or Word as under a wanted Long.  Rule P+ (#1216) cut a
; / at the top towards zero only under a wanted Long and rounded it down
; under a Byte or Word (x.w=l/q -4 for -10 and 3, r.l=l/q -3); a constant
; with a point was a Float under a Long but a Quick under a Byte or Word
; beside a Quick (x.w=-1.5+z -2, r.l=-1.5+z -1).  Correct (the user's
; decision of 2026-11-26, "P+ part 3 under Byte/Word (A1) and F2 under
; whole destinations (option 1)"): a / at the top under a wanted Byte, Word
; or Long towards zero, a constant with a point a Float under all three.
; A deeper / keeps its rule (the named limit: x.w=n/q+z -4, r.l -3).
!TestCase{topdiv}
  l.l=-10 : w.w=-10 : n.q=-10 : q.q=3 : z.q=0 : lz.l=0 : f.q=3.5 : k.q=-0.5
  rb.b=0 : rw.w=0 : rl.l=0
  rb=l/q : rw=l/q : rl=l/q
  !CheckEq{rb,-3,"b_l_div_q"}
  !CheckEq{rw,-3,"w_l_div_q"}
  !CheckEq{rl,-3,"l_l_div_q"}
  rb=w/q : rw=w/q : rl=w/q
  !CheckEq{rb,-3,"b_w_div_q"}
  !CheckEq{rw,-3,"w_w_div_q"}
  !CheckEq{rl,-3,"l_w_div_q"}
  rb=n/q : rw=n/q : rl=n/q
  !CheckEq{rb,-3,"b_n_div_q"}
  !CheckEq{rw,-3,"w_n_div_q"}
  !CheckEq{rl,-3,"l_n_div_q"}
  ; a divisor with a fraction (longdiv on the 16.16 bits; q48div)
  rb=n/f : rw=n/f : rl=n/f
  !CheckEq{rb,-2,"b_n_div_f"}
  !CheckEq{rw,-2,"w_n_div_f"}
  !CheckEq{rl,-2,"l_n_div_f"}
  rw=w/f : rl=w/f
  !CheckEq{rw,-2,"w_w_div_f"}
  !CheckEq{rl,-2,"l_w_div_f"}
  rw=l/f : rb=l/f
  !CheckEq{rw,-2,"w_l_div_f"}
  !CheckEq{rb,-2,"b_l_div_f"}
  rw=k/q
  !CheckEq{rw,0,"w_k_div_q"}
  ; a whole constant divisor
  rw=n/3 : rb=n/3
  !CheckEq{rw,-3,"w_n_div_3"}
  !CheckEq{rb,-3,"b_n_div_3"}
  rw=k/2
  !CheckEq{rw,0,"w_k_div_2"}
  ; a Long divisor
  rw=n/(lz+3)
  !CheckEq{rw,-3,"w_n_div_lz3"}
  ; a division by 0 at the top: the largest value of the type
  rw=n/z
  !CheckEq{rw,-32767,"w_n_div_0"}
  rw=l/z : rb=l/z
  !CheckEq{rw,-32767,"w_l_div_0"}
  !CheckEq{rb,-127,"b_l_div_0"}
  rw=n/lz
  !CheckEq{rw,-32767,"w_n_div_l0"}
  ; the named limit: a deeper / under a Word rounded down
  rw=n/q+z : rl=n/q+z
  !CheckEq{rw,-4,"w_n_div_q_add_z"}
  !CheckEq{rl,-3,"l_n_div_q_add_z"}
!EndTestCase
!TestCase{pointconst}
  w.w=-3 : n.q=-10 : q.q=10 : z.q=0
  rb.b=0 : rw.w=0 : rl.l=0
  rb=-1.5+z : rw=-1.5+z : rl=-1.5+z
  !CheckEq{rb,-1,"b_c_add_z"}
  !CheckEq{rw,-1,"w_c_add_z"}
  !CheckEq{rl,-1,"l_c_add_z"}
  rb=z-1.5 : rw=z-1.5 : rl=z-1.5
  !CheckEq{rb,-1,"b_z_sub_c"}
  !CheckEq{rw,-1,"w_z_sub_c"}
  !CheckEq{rl,-1,"l_z_sub_c"}
  rb=w*0.5+z : rw=w*0.5+z : rl=w*0.5+z
  !CheckEq{rb,-1,"b_w_mul_c_add_z"}
  !CheckEq{rw,-1,"w_w_mul_c_add_z"}
  !CheckEq{rl,-1,"l_w_mul_c_add_z"}
  rb=n*0.25 : rw=n*0.25 : rl=n*0.25
  !CheckEq{rb,-2,"b_n_mul_c"}
  !CheckEq{rw,-2,"w_n_mul_c"}
  !CheckEq{rl,-2,"l_n_mul_c"}
  rb=q*0.1 : rw=q*0.1 : rl=q*0.1
  !CheckEq{rb,1,"b_q_mul_c"}
  !CheckEq{rw,1,"w_q_mul_c"}
  !CheckEq{rl,1,"l_q_mul_c"}
!EndTestCase
