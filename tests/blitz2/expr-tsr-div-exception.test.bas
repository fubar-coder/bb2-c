; expr-tsr-div-exception (#1235, type-system-rework) - the whole-division
; exception under a Quick or Float destination (spec compiler/expression-
; types, "The whole-division exception under a Quick or Float
; destination"): a `/` of two whole operands that is the whole expression
; or is reached from it only through + - * / and the unary minus, at any
; depth, keeps its fraction, for constants and variables alike (a 48.16
; quotient cut down under a Quick, a quotient computed directly in Float
; under a Float); below any other operator and under any other destination
; it stays whole.  The nested forms (the answer I4: (a/b)/c, a/(d/b),
; -(a/b)) are checked with variables and with constants, each under a
; Quick, a Float and a Long.
; The original's values, where they differ, are the declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{the_top_division}
  w.w=200
  xq.q=1/16
  !CheckQEq{xq,0.0625,"x_constants"}
  yq.q=w/9
  !CheckEq{Peek.l(&yq),$001638E3,"y_variable"}
  zf.f=7/2
  !CheckFltNear{zf,3.5,0.000001,"z_float"}
!EndTestCase

!TestCase{through_plus_minus_times}
  w.w=117 : v.w=70
  xq.q=15-(w/16)
  !CheckQEq{xq,7.6875,"x_minus"}
  yq.q=1-(v/15)
  !CheckEq{Peek.l(&yq),$FFFC5556,"y_minus_negative"}
  zf.f=(7/2)*2
  !CheckFltNear{zf,7,0.000001,"z_float_product"}
  uq.q=2*(4/200)
  !CheckEq{Peek.l(&uq),$00000A3C,"u_product"}
!EndTestCase

; the forms of I4 with variables: a/b = 7/2, d/b = 1/2
!TestCase{nested_variables}
  a.w=7 : b.w=2 : c.w=2 : d.w=1
  xq.q=(a/b)/c
  !CheckQEq{xq,1.75,"x_quick_left"}
  yq.q=a/(d/b)
  !CheckQEq{yq,14,"y_quick_right"}
  zq.q=-(a/b)
  !CheckQEq{zq,-3.5,"z_quick_minus"}
  fx.f=(a/b)/c
  !CheckFltNear{fx,1.75,0.000001,"fx_float_left"}
  fy.f=a/(d/b)
  !CheckFltNear{fy,14,0.000001,"fy_float_right"}
  fz.f=-(a/b)
  !CheckFltNear{fz,-3.5,0.000001,"fz_float_minus"}
  rl.l=(a/b)/c
  !CheckEq{rl,1,"r_long"}
  r2l.l=-(a/b)
  !CheckEq{r2l,-3,"r2_long_minus"}
!EndTestCase

; the same forms with constants
!TestCase{nested_constants}
  xq.q=(7/2)/2
  !CheckQEq{xq,1.75,"x_quick_left"}
  yq.q=7/(1/2)
  !CheckQEq{yq,14,"y_quick_right"}
  zq.q=-(7/2)
  !CheckQEq{zq,-3.5,"z_quick_minus"}
  fx.f=(7/2)/2
  !CheckFltNear{fx,1.75,0.000001,"fx_float_left"}
  fy.f=7/(1/2)
  !CheckFltNear{fy,14,0.000001,"fy_float_right"}
  fz.f=-(7/2)
  !CheckFltNear{fz,-3.5,0.000001,"fz_float_minus"}
  rl.l=(7/2)/2
  !CheckEq{rl,1,"r_long"}
  r2l.l=-(7/2)
  !CheckEq{r2l,-3,"r2_long_minus"}
!EndTestCase

; a deeper nesting, with variables and with constants
!TestCase{nested_deeper}
  a.w=7 : b.w=2 : c.w=2 : d.w=1
  xq.q=((a/b)/c)/d
  !CheckQEq{xq,1.75,"x_quick_variables"}
  yq.q=((7/2)/2)/1
  !CheckQEq{yq,1.75,"y_quick_constants"}
  zf.f=-((a/b)/c)
  !CheckFltNear{zf,-1.75,0.000001,"z_float_variables"}
  uf.f=-((7/2)/2)
  !CheckFltNear{uf,-1.75,0.000001,"u_float_constants"}
!EndTestCase

!TestCase{below_other_operators}
  xq.q=(7/2=3)
  !CheckQEq{xq,-1,"x_compare"}
  yq.q=(7/2) AND 3
  !CheckQEq{yq,3,"y_bit_and"}
  zl.l=(7/2)*2
  !CheckEq{zl,6,"z_long"}
!EndTestCase
