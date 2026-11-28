; expr-constmod-exact (#1223) - a Mod of two constants is the exact Mod, as
; with variables.  Rule P+ (#1216) made Mod exact when an operand is a
; variable (q Mod 2 with q=5.25 is 1.25), but a Mod of two constants took
; the whole parts as the original did (5.25 Mod 2 was 1), and #1219 made
; only a divisor between 0 and 1 exact.  Correct (the user's decision of
; 2026-11-26, "Yes, all exact (Recommended)"): a Mod with a fraction in an
; operand gives the value of the same Mod with variables, under every
; destination, the floor Mod since type-system-rework task 4.7 (#1197: the
; remainder has the divisor's sign; before, a negative remainder plus the
; divisor); a divisor 0 stays "Division by Zero" (#1044), a Mod of whole
; numbers is unchanged (7 Mod 3).  Under a whole destination the Mod is a
; Quick rounded down as the variable form (x.l=-5.25 Mod -2 is -2, before
; task 4.7 -4), an exception to the Float of a constant with a point
; (#1222; the user's decision of 2026-11-26, "B: like the variable form").
!TestCase{quick}
  rq.q=0
  rq=5.25 Mod 2
  !CheckQEq{rq,1.25,"q_5.25_mod_2"}
  rq=5 Mod 1.5
  !CheckQEq{rq,0.5,"q_5_mod_1.5"}
  rq=7.5 Mod 2.5
  !CheckQEq{rq,0,"q_7.5_mod_2.5"}
  rq=6.0 Mod 4
  !CheckQEq{rq,2,"q_6.0_mod_4"}
  rq=7 Mod 3
  !CheckQEq{rq,1,"q_7_mod_3"}
  ; the sign rule of the variable's Mod: the floor Mod (#1197)
  rq=-5.25 Mod 2
  !CheckQEq{rq,0.75,"q_n5.25_mod_2"}
  rq=5.25 Mod -2
  ; before task 4.7: 1.25
  !CheckQEq{rq,-0.75,"q_5.25_mod_n2"}
  rq=-5.25 Mod -2
  ; before task 4.7: -3.25
  !CheckQEq{rq,-1.25,"q_n5.25_mod_n2"}
  rq=5 Mod -0.5
  !CheckQEq{rq,0,"q_5_mod_n0.5"}
!EndTestCase
!TestCase{asvariable}
  a.q=5.25 : b.q=-5.25 : c.q=6 : d.q=2 : e.q=0.5 : g.q=-2 : h.q=5 : m.q=-1000 : g1.q=-1.5
  rq.q=0 : rl.l=0 : rw.w=0 : rb.b=0 : rf.f=0 : vq.q=0 : vl.l=0 : vw.w=0 : vb.b=0 : vf.f=0
  ; every destination as the same Mod with variables
  rq=5.25 Mod 2 : vq=a Mod d : !CheckQEq{rq,vq,"q_a"}
  rl=5.25 Mod 2 : vl=a Mod d : !CheckEq{rl,vl,"l_a"} : !CheckEq{rl,1,"l_a_value"}
  rw=5.25 Mod 2 : vw=a Mod d : !CheckEq{rw,vw,"w_a"}
  rb=5.25 Mod 2 : vb=a Mod d : !CheckEq{rb,vb,"b_a"}
  rf=5.25 Mod 2 : vf=a Mod d : !CheckFltNear{rf,vf,0.0001,"f_a"}
  rl=-5.25 Mod 2 : vl=b Mod d : !CheckEq{rl,vl,"l_b"}
  rw=-5.25 Mod 2 : vw=b Mod d : !CheckEq{rw,vw,"w_b"} : !CheckEq{rw,0,"w_b_value"}
  rf=-5.25 Mod 2 : vf=b Mod d : !CheckFltNear{rf,vf,0.0001,"f_b"}
  rq=-5.25 Mod -2 : vq=b Mod g : !CheckQEq{rq,vq,"q_bg"}
  ; a negative result under a whole destination is rounded down as the variable
  ; form (-1.25 gives -2; -3.25 and -4 before task 4.7), not cut towards zero
  ; as a constant with a point (#1222)
  ; before task 4.7: -4
  rl=-5.25 Mod -2 : vl=b Mod g : !CheckEq{rl,vl,"l_bg"} : !CheckEq{rl,-2,"l_bg_value"}
  rw=-5.25 Mod -2 : vw=b Mod g : !CheckEq{rw,vw,"w_bg"}
  rb=-1000 Mod -1.5 : vb=m Mod g1 : !CheckEq{rb,vb,"b_cf"}
  rl=6.0 Mod 4 : vl=c Mod 4 : !CheckEq{rl,vl,"l_c"}
  rf=6.0 Mod 4 : vf=c Mod 4 : !CheckFltNear{rf,vf,0.0001,"f_c"}
  ; a constant beyond the Quick range is a Float, as the variable is
  fv.f=100000.5
  rf=100000.5 Mod 3 : vf=fv Mod 3 : !CheckFltNear{rf,vf,0.01,"f_big"} : !CheckFltNear{rf,1.5,0.01,"f_big_value"}
  rl=100000.5 Mod 3 : vl=fv Mod 3 : !CheckEq{rl,vl,"l_big"} : !CheckEq{rl,1,"l_big_value"}
  ; a Mod in a larger expression
  rq=1+5.25 Mod 2 : vq=1+a Mod d : !CheckQEq{rq,vq,"q_add"}
  rq=(5.25 Mod 2)*2 : vq=(a Mod d)*2 : !CheckQEq{rq,vq,"q_mul"}
!EndTestCase
!TestCase{printed}
  a.q=5.25 : b.q=-5.25 : d.q=2 : c.q=6
  !CheckStrEq{Str$(5.25 Mod 2),Str$(a Mod d),"s_a"}
  !CheckStrEq{Str$(-5.25 Mod 2),Str$(b Mod d),"s_b"}
  !CheckStrEq{Str$(6.0 Mod 4),Str$(c Mod 4),"s_c"}
  !CheckStrEq{Str$(5.25 Mod 2),"1.25","s_a_text"}
!EndTestCase
