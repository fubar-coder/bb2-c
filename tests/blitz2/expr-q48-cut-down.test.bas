; expr-q48-cut-down (#1221) - the cut at 1/65536 of a 48.16 product or
; quotient.  The 48.16 code cut it towards zero and the wanted whole number
; then rounded the value down, so a negative value less than 1/65536 below
; a whole number came out one above the exact value rounded down.  Correct
; (the user's decision of 2026-11-26, "Quick rounding: down stays; the
; 48.16 cut in the final direction (E)"): the cut rounds down, the value is
; the exact one rounded down once (q48mul's variant q48mulf, q48divw,
; q48divb, the inline fast paths).  Since type-system-rework (#1235, task
; 4.2; spec compiler/expression-types, "An operation with a Quick is
; computed in 48.16") every cut rounds down, under every destination and
; for a / at the top too (#1222's cut towards zero is gone); a value that
; changed with it names the one before in its comment.  A constant with a
; point under a wanted whole number is still a Float here (rule P+'s
; reading of constants until task 3.3): no 48.16 cut there.
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
  ; a / at the top: cut down, then rounded down (before: towards zero, 0)
  w=qn/lx
  !CheckEq{w,-1,"w_qn_div_lx"}
  b=qn/lx
  !CheckEq{b,-1,"b_qn_div_lx"}
  ; a constant with a point a Float (rule P+, until task 3.3)
  w=qt*0.5+lz
  !CheckEq{w,0,"w_qt_mul_half_add_lz"}
  ; before: 0
  w=(qt+lz)/qw
  !CheckEq{w,-1,"w_qtlz_div_qw"}
  w=(qt+lz)/2.5
  !CheckEq{w,0,"w_qtlz_div_c"}
  ; a wanted Quick, no wanted type, a / at the top under a Long: cut down
  ; too (before: towards zero, 0, 0, 0 and the condition false)
  x=q*p+lz
  !CheckEq{Peek.l(&x),-1,"x_q_mul_p_add_lz"}
  x=qn/lx
  !CheckEq{Peek.l(&x),-1,"x_qn_div_lx"}
  r=qn/lx
  !CheckEq{r,-1,"r_qn_div_lx_top"}
  r=0 : If q*p+lz<0 Then r=1
  !CheckEq{r,1,"if_q_mul_p_add_lz"}
!EndTestCase
