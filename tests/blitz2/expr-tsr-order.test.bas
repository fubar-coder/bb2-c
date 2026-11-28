; expr-tsr-order (#1235, type-system-rework) - the type and format of an
; operation do not depend on the order of its operands (spec compiler/
; expression-types, "The type and format of an operation do not depend on
; the order of its operands", modified): each operation has its type and
; format from its operator and its operands' types, never from the order
; of its operands, the place of a subexpression in a chain or the
; destination; the values in both orders agree unless a Long or 48.16
; intermediate wraps.  Each case computes both orders.
; The original's values, where they differ, are the declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{base_plus_scaled_quick}
  base.l=2097152 : q=1.5
  adr.l=base+q*4
  !CheckEq{adr,2097158,"adr_base_first"}
  adr2.l=q*4+base
  !CheckEq{adr2,2097158,"adr2_quick_first"}
!EndTestCase

!TestCase{words_and_a_quick_under_a_word}
  q=1.5 : w.w=3 : v.w=4
  xw.w=w+v+q*2
  !CheckEq{xw,10,"x_words_first"}
  yw.w=q*2+w+v
  !CheckEq{yw,10,"y_quick_first"}
!EndTestCase

!TestCase{comparison_of_a_chain}
  l.l=10 : q=1.5
  rl.l=0
  If l+q*2>12 Then rl=1
  !CheckEq{rl,1,"if_long_first"}
  r2l.l=0
  If q*2+l>12 Then r2l=1
  !CheckEq{r2l,1,"if_quick_first"}
!EndTestCase

!TestCase{long_and_quicks}
  l.l=10 : q=1.5 : p=2.75
  rl.l=l+q+p
  !CheckEq{rl,14,"r_long_first"}
  r2l.l=q+p+l
  !CheckEq{r2l,14,"r2_long_last"}
!EndTestCase

!TestCase{bit_operator_shift_and_not}
  l.l=10 : q=1.5 : p=1.5
  rl.l=(l+q+p) AND 255
  !CheckEq{rl,13,"and_long_first"}
  r2l.l=(q+p+l) AND 255
  !CheckEq{r2l,13,"and_long_last"}
  r3l.l=(l+q+p) LSL 1
  !CheckEq{r3l,26,"lsl_long_first"}
  r4l.l=(q+p+l) LSL 1
  !CheckEq{r4l,26,"lsl_long_last"}
  r5l.l=Not (l+q+p)
  !CheckEq{r5l,-14,"not_long_first"}
  r6l.l=Not (q+p+l)
  !CheckEq{r6l,-14,"not_long_last"}
!EndTestCase

!TestCase{a_float_in_the_chain}
  l.l=10 : q=1.5 : f.f=0.5
  rl.l=l+q+f
  !CheckEq{rl,12,"r_long_first"}
  r2l.l=f+q+l
  !CheckEq{r2l,12,"r2_float_first"}
!EndTestCase

!TestCase{print_word_sums_in_both_orders}
  w.w=20000 : v.w=20000 : q=0.5
  NPrint w+v+q
  NPrint q+w+v
!EndTestCase

!TestCase{an_intermediate_leaves_its_format}
  q=300 : q2=300 : r=0.25
  xq.q=q*q2*r
  !CheckQEq{xq,22500,"x_product_first"}
  yq.q=r*q*q2
  !CheckQEq{yq,22500,"y_product_last"}
!EndTestCase
