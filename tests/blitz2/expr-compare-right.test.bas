; expr-compare-right (#1177) - the right operand of a comparison typed by
; itself since the fix of #1177: a compound right operand was computed in
; its own types with the old operand typing, and a constant with a point on
; the right made a Float comparison.  With base.l=$40000 and a default-typed
; i=20000, i*4+base overflowed as a Quick (If base<i*4+base false), and a
; Quick q=0.1 was compared with the Float 0.1 (If q=0.1 false); the original
; gives true for both (docs/blitz2/deviations.md, #1177's row).
; Correct: the operand typing of #1216 (P+) computes i*4+base exactly
; (342144 > 262144) and compares q with the constant 0.1 as the Quick it is
; held in, so both are true, as in the original.  The original agrees on
; every check here.
!TestCase{compare_right}
  base.l=$40000 : i=20000 : q.q=0.1
  r.l=0 : rw.w=0
  r=0 : If base<i*4+base Then r=-1
  !CheckEq{r,-1,"if_compound_right"}
  r=(base<i*4+base)
  !CheckEq{r,-1,"long_compound_right"}
  rw=(base<i*4+base)
  !CheckEq{rw,-1,"word_compound_right"}
  r=0 : If q=0.1 Then r=-1
  !CheckEq{r,-1,"if_fraction_right"}
  r=(q=0.1)
  !CheckEq{r,-1,"long_fraction_right"}
!EndTestCase
