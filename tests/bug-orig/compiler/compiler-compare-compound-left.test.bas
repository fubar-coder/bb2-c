; compiler-compare-compound-left (#1183) - the type an expression is
; wanted in reaches the operands of a comparison.  The compiler emits
; the code of the left operand before it reads the comparison operator,
; so when the left operand is an expression (q*2, q+q) or a
; constant it is computed in that type: with q=1.25 and a Long
; destination q*2>2.4 computes 2>2.4 (0; the original cut q to 1 first,
; 1*2>2.4).
; Correct: the operands are compared with their own values: q*2 is 2.5
; and q+q is 2.5, so all three comparisons are true (-1); the Long constant
; 100000001 under a Float destination and the
; Long sum l1+1 under a Quick destination are compared as Longs (a Float
; holds 24 bits, a Quick 16 bits of whole part).
; The count cases of the spec (compiler/expression-types, "The left
; operand of a comparison") whose sums, products and quotients keep their
; results (-1) are the checks the original already passes.
; Fixed (#1183) by the operand typing of #1216 (rule P+): the left operand
; is typed by its own operands; the left operand that is a single element
; is #1177 (compiler-quick-compare).
!TestCase{compound_left}
  q.q=1.25 : r.l=0
  r=(q*2>2.4)
  !CheckEq{r,-1,"times_greater"}
  r=(q+q>2.4)
  !CheckEq{r,-1,"plus_greater"}
  r=(q*2=2.5)
  !CheckEq{r,-1,"times_equal"}
  l3.l=100000000 : rf.f=0
  rf=(100000001>l3)
  !CheckFltNear{rf,-1,0.001,"long_constant_float"}
  l1.l=40000 : rq.q=0
  rq=(l1+1>40000)
  !CheckQEq{rq,-1,"long_sum_quick"}
  ; the count cases that keep their results
  w.w=20000 : v.w=20000 : c.w=0
  r=(w+v>c)
  !CheckEq{r,-1,"word_sum"}
  r=(w-(-v)>c)
  !CheckEq{r,-1,"word_difference"}
  qh.q=100
  r=(qh*1000>50000)
  !CheckEq{r,-1,"quick_product"}
  ld.l=10 : rf=0 : rq=0
  rf=(ld/3>3.2)
  !CheckFltNear{rf,-1,0.001,"quotient_float"}
  rq=(ld/3>3.2)
  !CheckQEq{rq,-1,"quotient_quick"}
  b1.b=100 : b2.b=100 : rb.b=0
  rb=(b1+b2>150)
  !CheckEq{rb,-1,"byte_sum"}
!EndTestCase
