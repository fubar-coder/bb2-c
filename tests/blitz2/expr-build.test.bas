; expr-build - what the order of the side effects of an expression decides:
; a library that only a dropped conversion links (the left operand of a
; comparison gets its own type back, #1177: the conversion varcont made
; for it is not kept, its library is), and the constant fold of two
; constants (its value computed by running the operation in the compiler):
; a sum of literals, a sum whose first operand is a variable (only the
; constants after it pair), a negated literal, Not of a constant, a
; folded Mod as the first use of its library (#1049: its relocations
; stay), a folded division, Float and Quick folds, a constant compared,
; and constants in an array index.
; The cases pin the code of these forms for the restructuring of the
; evaluator (#1180, change expr-tree-4a-emit-after-build): their executable
; is compared with the previous commit's (blitz2_expr_build_prev), the
; values here are their results.
; dropconv: the program's only Quick-to-Float conversion is the one the
; comparison drops (q under the Float wanted by r), so floatquicklib is
; linked by it alone, before ffplib, which its call names.
; Correct: every case gives the values of its checks.
DEFTYPE .l

!TestCase{dropconv}
  q.q = 2
  r.f = (q = 2)
  rl.l = r
  !CheckEq{rl,-1,"compare"}
  r = (q = 3)
  rl = r
  !CheckEq{rl,0,"compare_false"}
!EndTestCase

!TestCase{fold_sum}
  a.l = 1+2
  !CheckEq{a,3,"sum"}
  x.l = 10
  a = x+1+2
  !CheckEq{a,13,"left_var"}
  a = 1+2+x
  !CheckEq{a,13,"right_var"}
  a = 2*3+4*5
  !CheckEq{a,26,"two_pairs"}
  w.w = 100+200
  !CheckEq{w,300,"word"}
!EndTestCase

!TestCase{fold_unary}
  a.l = -5
  !CheckEq{a,-5,"neg"}
  a = - -5
  !CheckEq{a,5,"neg_neg"}
  a = -(2+3)
  !CheckEq{a,-5,"neg_bracket"}
  a = Not 0
  !CheckEq{a,-1,"not_0"}
  a = Not 5+1
  !CheckEq{a,-7,"not_sum"}
  a = 3-Not 0
  !CheckEq{a,4,"minus_not"}
!EndTestCase

!TestCase{fold_mod}
  ; the first Mod of the program: modlib linked during the fold (#1049)
  b.w = 7 Mod 3
  !CheckEq{b,1,"mod"}
  x.l = 17 : y.l = 5
  c.l = x Mod y
  !CheckEq{c,2,"mod_run"}
  c = 100/7
  !CheckEq{c,14,"div"}
  c = 70000*3
  !CheckEq{c,210000,"long_mul"}
!EndTestCase

!TestCase{fold_float}
  f.f = 1.5*2
  !CheckFltNear{f,3,0.001,"float_mul"}
  f = 10/4
  !CheckFltNear{f,2.5,0.001,"float_div"}
  q.q = 1.5+0.25
  !CheckQEq{q,1.75,"quick_add"}
  q = 3*0.5
  !CheckQEq{q,1.5,"quick_mul"}
!EndTestCase

!TestCase{fold_compare}
  a.l = (1+1=2)
  !CheckEq{a,-1,"eq"}
  a = (3<2)+5
  !CheckEq{a,5,"lt"}
  bb.b = 200
  a = (bb=200)
  !CheckEq{a,-1,"byte_200"}
!EndTestCase

!TestCase{fold_index}
  Dim arr.l(20,4)
  arr(3+4,1+1) = 9
  i.l = 7 : j.l = 2
  !CheckEq{arr(i,j),9,"index"}
  !CheckEq{arr(10-3,4/2),9,"index_fold"}
!EndTestCase
