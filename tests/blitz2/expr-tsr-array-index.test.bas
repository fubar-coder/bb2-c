; expr-tsr-array-index (#1235, type-system-rework) - every array index is
; evaluated as a Long (spec compiler/expression-types, "Every array index
; is evaluated as a Long"): every index (the first and every later one) is
; evaluated by the rules of the expression types and converted to a Long as
; a Long destination does (rounded down); an index addresses the same
; element with and without runtime error checking (the axis runtime-errors
; of expr-tsr-array-index.test.toml) whenever the checked build does not
; report it.
; The original's values, where they differ, are the declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{a_quick_calculation_beyond_the_range}
  Dim g.b(1,60000),h.b(60000,1)
  y=200
  g(1,20000)=7 : h(20000,1)=9
  rg.l=g(1,y*200/2)
  !CheckEq{rg,7,"g_later_index"}
  rh.l=h(y*200/2,1)
  !CheckEq{rh,9,"h_first_index"}
!EndTestCase

!TestCase{a_shift_of_a_quick_in_a_later_index}
  Dim b.b(1,60)
  q=1.5
  b(1,24)=5 : b(1,16)=6
  rl.l=b(1,q ASL 4)
  !CheckEq{rl,5,"b_asl"}
!EndTestCase

!TestCase{a_quick_with_a_fraction_keeps_its_value}
  Dim b.b(1,60),c.b(1,5,60),v.b(1,60)
  q=1.5
  b(1,15)=3 : c(1,5,15)=4
  rl.l=b(1,q*10)
  !CheckEq{rl,3,"b_later_index"}
  r2l.l=c(1,5,q*10)
  !CheckEq{r2l,4,"c_third_index"}
  v(1,q*10)=99
  r3l.l=v(1,15)
  !CheckEq{r3l,99,"v_assigned"}
!EndTestCase

!TestCase{large_whole_number_indexes}
  Dim g.b(1,60000)
  l.l=40000 : wv.w=20000
  g(1,40000)=8
  rl.l=g(1,40000)
  !CheckEq{rl,8,"g_constant"}
  r2l.l=g(1,l)
  !CheckEq{r2l,8,"g_long"}
  r3l.l=g(1,wv*2)
  !CheckEq{r3l,8,"g_word_product"}
!EndTestCase
