; expr-q48-bigconst (#1220) - a constant with a fraction beyond the Quick
; range beside a 48.16 operand.  The pass encoded such a constant as one
; 32-bit Quick value (-99898.5 * 65536 wrapped to 32 bits) and widened it as
; a Quick, so a comparison with l+q-lx (-99897.5) gave the wrong result.
; Correct: the constant is a 48.16 value in a register pair (rule P+, part 8:
; the comparison is exact).  The whole constant, the arithmetic with such a
; constant and the constants just inside the Quick range stay as they were.
!TestCase{bigconst}
  l.l=100 : lx.l=100000 : q.q=2.5 : r.l=0 : lm.l=-2147483647
  If l+q-lx>=-99898.5 Then r=1 Else r=2
  !CheckEq{r,1,"if_ge_neg"}
  r=(l+q-lx>=-99898.5)
  !CheckEq{r,-1,"val_ge_neg"}
  r=(l+q+lx<100103.5)
  !CheckEq{r,-1,"val_lt_pos"}
  r=(l+q-lx<-99898.5)
  !CheckEq{r,0,"val_lt_neg"}
  r=(l+q+lx>100103.5)
  !CheckEq{r,0,"val_gt_pos"}
  r=(-99898.5<=l+q-lx)
  !CheckEq{r,-1,"val_const_left"}
  r=(l+q-lx<>-99898.5)
  !CheckEq{r,-1,"val_ne"}
  r=(l+q-lx=-99897.5)
  !CheckEq{r,-1,"val_eq_exact"}
  r=(l+q-lx>-99897.5)
  !CheckEq{r,0,"val_gt_equal"}
  r=(l+q-lx>-99898)
  !CheckEq{r,-1,"val_whole_const"}
  r=l+q-lx+99898.5
  !CheckEq{r,1,"arith_add"}
  r=l+q-lx-(-99898.5)
  !CheckEq{r,1,"arith_sub"}
  r=(99898.5+(l+q-lx))
  !CheckEq{r,1,"arith_add_left"}
  r=(l+q-lx<=-32767.5)
  !CheckEq{r,-1,"inside_le"}
  r=(l+q-lx>=-32767.5)
  !CheckEq{r,0,"inside_ge"}
  r=(l+q-lx<=-32768.5)
  !CheckEq{r,-1,"edge_le"}
  r=(l+q-lx>=-32768.5)
  !CheckEq{r,0,"edge_ge"}
  r=((l+q-lx)-99898.5<-199795)
  !CheckEq{r,-1,"sub_lt"}
  r=((l+q-lx)-99898.5=-199796)
  !CheckEq{r,-1,"sub_eq"}
  r=99898.5-(l+q-lx)
  !CheckEq{r,199796,"rsub"}
  r=(l+q-lx)*0.5+99898.5
  !CheckEq{r,49949,"mul_half_add"}
  r=((l+q-lx)*99898.5<lm)
  !CheckEq{r,-1,"mul_lt_long"}
  r=((l+q-lx)/99898.5<-0.99)
  !CheckEq{r,-1,"div_lt"}
  r=((l+q-lx)/99898.5>-1)
  !CheckEq{r,-1,"div_gt"}
  r=(l+q-lx) Mod 99898.5
  !CheckEq{r,1,"mod_floored"}
!EndTestCase
