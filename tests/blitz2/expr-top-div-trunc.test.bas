; expr-top-div-trunc (#1222) - a / at the top and a constant with a point
; under a wanted Byte or Word as under a wanted Long.  Rule P+ (#1216) cut a
; / at the top towards zero only under a wanted Long and rounded it down
; under a Byte or Word (x.w=l/q -4 for -10 and 3, r.l=l/q -3); a constant
; with a point was a Float under a Long but a Quick under a Byte or Word
; beside a Quick (x.w=-1.5+z -2, r.l=-1.5+z -1).  The user's decision of
; 2026-11-26, "P+ part 3 under Byte/Word (A1) and F2 under whole
; destinations (option 1)", made a / at the top towards zero under all
; three and a constant with a point a Float under all three.
; type-system-rework (#1235, task 4.2; spec compiler/expression-types, "An
; operation with a Quick is computed in 48.16", "The conversion to a whole
; destination rounds down and wraps") replaces the first part: a / with a
; Quick operand is a 48.16 quotient cut down, under every destination, and
; the conversion to a Byte, Word or Long rounds it down, at the top as
; deeper (x.w=l/q and r.l=l/q -4 for -10 and 3); each value that changed
; names the one before in its comment.  A division by 0 keeps the largest
; value of the wanted type until task 4.7.  The constants with a point
; (pointconst) keep #1222's Float until task 3.3.
!TestCase{topdiv}
  l.l=-10 : w.w=-10 : n.q=-10 : q.q=3 : z.q=0 : lz.l=0 : f.q=3.5 : k.q=-0.5
  rb.b=0 : rw.w=0 : rl.l=0
  ; before: -3 each
  rb=l/q : rw=l/q : rl=l/q
  !CheckEq{rb,-4,"b_l_div_q"}
  !CheckEq{rw,-4,"w_l_div_q"}
  !CheckEq{rl,-4,"l_l_div_q"}
  rb=w/q : rw=w/q : rl=w/q
  !CheckEq{rb,-4,"b_w_div_q"}
  !CheckEq{rw,-4,"w_w_div_q"}
  !CheckEq{rl,-4,"l_w_div_q"}
  rb=n/q : rw=n/q : rl=n/q
  !CheckEq{rb,-4,"b_n_div_q"}
  !CheckEq{rw,-4,"w_n_div_q"}
  !CheckEq{rl,-4,"l_n_div_q"}
  ; a divisor with a fraction (before: -2 each)
  rb=n/f : rw=n/f : rl=n/f
  !CheckEq{rb,-3,"b_n_div_f"}
  !CheckEq{rw,-3,"w_n_div_f"}
  !CheckEq{rl,-3,"l_n_div_f"}
  rw=w/f : rl=w/f
  !CheckEq{rw,-3,"w_w_div_f"}
  !CheckEq{rl,-3,"l_w_div_f"}
  rw=l/f : rb=l/f
  !CheckEq{rw,-3,"w_l_div_f"}
  !CheckEq{rb,-3,"b_l_div_f"}
  ; before: 0
  rw=k/q
  !CheckEq{rw,-1,"w_k_div_q"}
  ; a whole constant divisor (before: -3, -3 and 0)
  rw=n/3 : rb=n/3
  !CheckEq{rw,-4,"w_n_div_3"}
  !CheckEq{rb,-4,"b_n_div_3"}
  rw=k/2
  !CheckEq{rw,-1,"w_k_div_2"}
  ; a Long divisor (before: -3)
  rw=n/(lz+3)
  !CheckEq{rw,-4,"w_n_div_lz3"}
  ; a division by 0 at the top: the largest value of the type
  rw=n/z
  !CheckEq{rw,-32767,"w_n_div_0"}
  rw=l/z : rb=l/z
  !CheckEq{rw,-32767,"w_l_div_0"}
  !CheckEq{rb,-127,"b_l_div_0"}
  rw=n/lz
  !CheckEq{rw,-32767,"w_n_div_l0"}
  ; a deeper / as one at the top (before: -4 and -3)
  rw=n/q+z : rl=n/q+z
  !CheckEq{rw,-4,"w_n_div_q_add_z"}
  !CheckEq{rl,-4,"l_n_div_q_add_z"}
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
