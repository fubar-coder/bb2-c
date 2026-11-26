; expr-constmod-larger (#1224) - a Mod of two constants inside a larger
; constant expression gives what the same expression with variables gives.
; #1223 made a Mod of two constants with a fraction (or a point) in an
; operand the exact Mod, a Quick rounded down under a whole destination as
; the variable form (x.l=-5.25 Mod -2 is -4), but a larger constant
; expression with such a Mod still followed the Float of a constant with a
; point (#1222): x.l=-1-5.25 Mod 2 was -2 (-2.25 towards zero), where
; x.l=-1-q Mod 2 with q=5.25 is -3 (rounded down).  Correct (the user's
; decision of 2026-11-26): the rule of #1223 also holds in a larger constant
; expression, under every destination: an expression of +, - and * over
; whole constants and such Mods is a Quick under a whole destination,
; rounded down as its variable form.
!TestCase{whole}
  a.q=5.25 : b.q=-5.25 : d.q=2 : g.q=-2 : p.q=1.5 : s.q=7.5
  rl.l=0 : rw.w=0 : rb.b=0 : vl.l=0 : vw.w=0 : vb.b=0
  rl=-1-5.25 Mod 2 : vl=-1-a Mod d : !CheckEq{rl,vl,"l_sub"} : !CheckEq{rl,-3,"l_sub_value"}
  rw=-1-5.25 Mod 2 : vw=-1-a Mod d : !CheckEq{rw,vw,"w_sub"} : !CheckEq{rw,-3,"w_sub_value"}
  rb=-1-5.25 Mod 2 : vb=-1-a Mod d : !CheckEq{rb,vb,"b_sub"} : !CheckEq{rb,-3,"b_sub_value"}
  rl=-5.25 Mod -2+1 : vl=b Mod g+1 : !CheckEq{rl,vl,"l_add"} : !CheckEq{rl,-3,"l_add_value"}
  rw=-5.25 Mod -2+1 : vw=b Mod g+1 : !CheckEq{rw,vw,"w_add"}
  rl=-5.25 Mod 2-1 : vl=b Mod d-1 : !CheckEq{rl,vl,"l_sub1"} : !CheckEq{rl,-1,"l_sub1_value"}
  rl=0-5 Mod 1.5 : vl=0-5 Mod p : !CheckEq{rl,vl,"l_divfrac"} : !CheckEq{rl,-1,"l_divfrac_value"}
  rb=0-5 Mod 1.5 : vb=0-5 Mod p : !CheckEq{rb,vb,"b_divfrac"}
  rl=(-5.25 Mod 2)*-1 : vl=(b Mod d)*-1 : !CheckEq{rl,vl,"l_mul"} : !CheckEq{rl,-1,"l_mul_value"}
  rw=(5.25 Mod 2)-(7.5 Mod 2) : vw=(a Mod d)-(s Mod d) : !CheckEq{rw,vw,"w_mods"} : !CheckEq{rw,-1,"w_mods_value"}
  ; a whole-number operation of constants beside such a Mod is a constant of
  ; its value, as in the variable form (7 Mod 3 is 1)
  rl=(7 Mod 3)-(5.25 Mod 2) : vl=(7 Mod 3)-(a Mod d) : !CheckEq{rl,vl,"l_whole"} : !CheckEq{rl,-1,"l_whole_value"}
  rw=1-(7 Mod 3)*(5.25 Mod 2) : vw=1-(7 Mod 3)*(a Mod d) : !CheckEq{rw,vw,"w_whole"} : !CheckEq{rw,-1,"w_whole_value"}
  rb=(1 LSL 2)-(5.25 Mod 2)*4 : vb=(1 LSL 2)-(a Mod d)*4 : !CheckEq{rb,vb,"b_whole"} : !CheckEq{rb,-1,"b_whole_value"}
  ; beyond the Quick range: rounded down as the variable form's Long
  rl=-5.25 Mod 2-100000 : vl=b Mod d-100000 : !CheckEq{rl,vl,"l_big"} : !CheckEq{rl,-100000,"l_big_value"}
  ; a further constant with a point makes the variable form a Float (#1222):
  ; towards zero, in both forms
  rl=-0.5-5.25 Mod 2 : vl=-0.5-a Mod d : !CheckEq{rl,vl,"l_point"} : !CheckEq{rl,-1,"l_point_value"}
  ; a positive result: rounded down as before
  rl=3-5.25 Mod 2 : vl=3-a Mod d : !CheckEq{rl,vl,"l_pos"} : !CheckEq{rl,1,"l_pos_value"}
  rl=2*5.25 Mod 2 : vl=2*a Mod d : !CheckEq{rl,vl,"l_mulpos"} : !CheckEq{rl,2,"l_mulpos_value"}
!EndTestCase
!TestCase{fractional}
  a.q=5.25 : b.q=-5.25 : d.q=2 : g.q=-2
  rq.q=0 : rf.f=0 : vq.q=0 : vf.f=0
  rq=-1-5.25 Mod 2 : vq=-1-a Mod d : !CheckQEq{rq,vq,"q_sub"} : !CheckQEq{rq,-2.25,"q_sub_value"}
  rf=-1-5.25 Mod 2 : vf=-1-a Mod d : !CheckFltNear{rf,vf,0.0001,"f_sub"}
  rq=-5.25 Mod -2+1 : vq=b Mod g+1 : !CheckQEq{rq,vq,"q_add"}
  rf=-5.25 Mod -2+1 : vf=b Mod g+1 : !CheckFltNear{rf,vf,0.0001,"f_add"}
  !CheckStrEq{Str$(-1-5.25 Mod 2),Str$(-1-a Mod d),"s_sub"}
!EndTestCase
