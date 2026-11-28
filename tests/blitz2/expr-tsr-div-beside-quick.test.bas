; expr-tsr-div-beside-quick (#1235, type-system-rework) - a whole division
; beside a Quick keeps its fraction (spec compiler/expression-types, "A
; whole division beside a Quick keeps its fraction", "A whole division
; reached by two exceptions under a Float destination"): a `/` of two
; whole operands whose other operand at an arithmetic operator (+ - * /,
; Mod, ^) or a comparison is of Quick class (a Quick variable, a
; subexpression with a Quick result, a constant with a point that is a
; Quick; not a Float) keeps its fraction as a 48.16 quotient under every
; destination, also through a unary minus; at a bit operator or a shift it
; stays whole.  Where this and the whole-division exception both reach the
; `/` under a Float destination, its quotient is the Float quotient.
; The original's values, where they differ, are the declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{print_beside_a_quick}
  q=0.5 : w.w=3
  NPrint q+w/2
  NPrint w/2+q
!EndTestCase

!TestCase{under_a_word_destination}
  q2=149.75 : w2.w=135
  xw.w=(q2+(w2/2))-2
  !CheckEq{xw,215,"x_word"}
!EndTestCase

!TestCase{print_other_neighbours}
  q=0.25 : n.w=2 : w.w=3 : d.w=2 : f.f=0.5
  NPrint q+w/d
  NPrint 0.5+w/2
  NPrint (q*n)+w/2
  NPrint f+w/2
!EndTestCase

!TestCase{bit_operator}
  q=5 : w.w=-3
  xl.l=q AND w/2
  !CheckEq{xl,5,"x_and"}
!EndTestCase

!TestCase{print_comparison}
  q2=1 : w2.w=3
  NPrint q2<w2/2
!EndTestCase

!TestCase{print_through_a_unary_minus}
  q=0.5 : w.w=3
  NPrint q+-(w/2)
!EndTestCase

!TestCase{two_exceptions_under_a_float}
  q=0.5 : w.w=1
  yf.f=q+w/3
  !CheckFltNear{yf,0.8333334,0.000001,"y_float"}
!EndTestCase
