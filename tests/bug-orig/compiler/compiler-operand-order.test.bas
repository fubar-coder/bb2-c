; compiler-operand-order (#1216) - the type and the format of an
; operation depend on the order of its operands and the place of a
; subexpression in a chain.
; BUG(orig) (#1216): the expression is evaluated from left to right with
; one "type so far": a Quick fetched while that type is Long (a Long
; operand or destination before it) is added, subtracted and multiplied
; in 16.16 or cut, a Quick before it keeps its fraction.  So l+q+p is 13
; and q+p+l 14 for l=10, q=1.5, p=2.75 (the port; the original gives 13
; for both), and w/2 beside a Quick is the whole-number division before
; it and the fractional one after it.
; Correct (spec compiler/expression-types, "The type and format of an
; operation do not depend on the order of its operands"): the value of
; both orders agrees, `If` and a comparison compute both operands in one
; common format, a constant with a point is a Quick beside a Quick; the
; values are the declared deviations of the original.  The decided rule
; is P+ (design.md of expr-tree-5-operand-typing, decisions 6 and 7).
; The cells are those where the port before the fix of #1216 and the
; original gave the same value: the forms the port already computed
; right (base+q*4, the Word chains, the second order of a bit operator,
; q+p+l) are the fixed tests of #1178.  The
; other forms of the change: compiler-expr-1216-* (the matrix's named
; cells), compiler-operand-order-print (NPrint).
!TestCase{order}
  q.q=1.5 : p.q=2.75 : l.l=10 : f.f=0.5 : r.l=0
  r=l+q+p
  !CheckEq{r,14,"long_quick_quick"}
  r=l+q+f
  !CheckEq{r,12,"long_quick_float"}
  r=f+q+l
  !CheckEq{r,12,"float_quick_long"}
  ; the limit: an intermediate leaves its format, both orders typed alike
  q.q=300 : q2.q=300 : rr.q=0.25 : xq.q=0
  xq=q*q2*rr
  !CheckQEq{xq,6116,"wrapped_product"}
  xq=rr*q*q2
  !CheckQEq{xq,22500,"scaled_first"}
!EndTestCase

!TestCase{bits}
  q.q=1.5 : p.q=1.5 : l.l=10 : r.l=0
  r=(l+q+p) AND 255
  !CheckEq{r,13,"and_long_first"}
  r=(l+q+p) LSL 1
  !CheckEq{r,26,"lsl_long_first"}
  r=Not (l+q+p)
  !CheckEq{r,-14,"not_long_first"}
!EndTestCase

!TestCase{if_forms}
  l1.l=10 : w.w=3 : q.q=1.2 : xq.q=3 : q3.q=2 : r.l=0 : rf.f=0 : rq.q=0
  rf=(l1/3>3.2)
  !CheckFltNear{rf,-1,0.001,"float_dest"}
  rq=(l1/3>3.2)
  !CheckQEq{rq,-1,"quick_dest"}
  r=(l1/3>3.2)
  !CheckEq{r,-1,"long_dest"}
  r=0 : If l1/3>3.2 Then r=-1
  !CheckEq{r,-1,"if_quotient"}
  r=0 : If w/2>q Then r=-1
  !CheckEq{r,-1,"if_word_half_greater"}
  r=0 : If w/2*2<>xq Then r=-1
  !CheckEq{r,0,"if_word_half_twice_differs"}
  r=0 : If w/2*2=q3 Then r=-1
  !CheckEq{r,0,"if_word_half_twice_equals"}
  r=0 : If w/2*2=w Then r=-1
  !CheckEq{r,0,"if_word_half_twice_word"}
!EndTestCase

!TestCase{mod_fraction}
  q.q=7.5 : xm.q=0
  xm=q Mod 2
  !CheckQEq{xm,1.5,"quick_mod"}
!EndTestCase

!TestCase{word_sum}
  ww.w=20000 : vv.w=20000 : cc.w=0 : rw.w=0 : r.l=0
  rw=(ww+vv>cc)
  !CheckEq{rw,-1,"word_dest"}
  If ww+vv>cc Then r=-1
  !CheckEq{r,-1,"if_word_sum"}
!EndTestCase
