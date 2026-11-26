; expr-q48-cut-down (#1221) - the cut at 1/65536 of a 48.16 product or
; quotient under a wanted Byte, Word or Long.  The 48.16 code cut it
; towards zero and the wanted whole number then rounded the value down, so
; a negative value less than 1/65536 below a whole number came out one
; above the exact value rounded down.  Correct (the user's decision of
; 2026-11-26, "Quick rounding: down stays; the 48.16 cut in the final
; direction (E)"): under a wanted whole number the cut rounds down too, the
; value is the exact one rounded down once (q48mul's variant q48mulf,
; q48divw, q48divb, the inline fast paths); under a wanted Quick, without a
; wanted type and for a / at the top (towards zero under a wanted Byte, Word
; or Long, #1222) it stays as it was.  A constant with a point is a Float
; under a wanted whole number (#1222): no 48.16 cut there.
!TestCase{cutdown}
  q.q=-0.5 : p.q=1 : p=p/65536 : l.l=5 : lz.l=0 : lx.l=100000 : r.l=0 : w.w=0 : b.b=0 : x.q=0
  qt.q=-1 : qt=qt/65536 : qn.q=-1.5 : qw.q=2
  r=q*p
  !CheckEq{r,-1,"r_q_mul_p"}
  r=q*p+l
  !CheckEq{r,4,"r_q_mul_p_add_l"}
  r=l+q*p
  !CheckEq{r,4,"r_l_add_q_mul_p"}
  w=q*p+lz
  !CheckEq{w,-1,"w_q_mul_p_add_lz"}
  w=qt/2+lz
  !CheckEq{w,-1,"w_qt_div_2_add_lz"}
  r=(qt+lz)*q
  !CheckEq{r,0,"r_qtlz_mul_q_positive"}
  r=(qt+lz)*p
  !CheckEq{r,-1,"r_qtlz_mul_p"}
  ; a / at the top towards zero, a constant with a point a Float (#1222)
  w=qn/lx
  !CheckEq{w,0,"w_qn_div_lx"}
  b=qn/lx
  !CheckEq{b,0,"b_qn_div_lx"}
  w=qt*0.5+lz
  !CheckEq{w,0,"w_qt_mul_half_add_lz"}
  w=(qt+lz)/qw
  !CheckEq{w,0,"w_qtlz_div_qw"}
  w=(qt+lz)/2.5
  !CheckEq{w,0,"w_qtlz_div_c"}
  ; unchanged: a wanted Quick, no wanted type, a / at the top under a Long
  x=q*p+lz
  !CheckEq{Peek.l(&x),0,"x_q_mul_p_add_lz"}
  x=qn/lx
  !CheckEq{Peek.l(&x),0,"x_qn_div_lx"}
  r=qn/lx
  !CheckEq{r,0,"r_qn_div_lx_top"}
  r=0 : If q*p+lz<0 Then r=1
  !CheckEq{r,0,"if_q_mul_p_add_lz"}
!EndTestCase
