; expr-tsr-print (#1235, type-system-rework) - without a destination a
; 48.16 value is a Quick, a Long or a Float (spec compiler/expression-
; types, "Without a destination a 48.16 value is a Quick, a Long or a
; Float"): where no type is wanted (Print, NPrint, Str$ and untyped
; parameters) a whole result is passed as a whole number, and a 48.16
; result at run time as a Quick when it fits the Quick range, else as a
; Long when it is whole and fits a Long, else as a Float.  The printed
; text is what the test judges (the first case asserts `output`); Str$
; takes the same value (the second case).
; The original's values, where they differ, are the declared deviations.
; Correct: the values of the spec's scenarios.
!TestCase{print_the_three_forms}
  q=1.5 : w.w=3 : l.l=100000
  NPrint q+w
  NPrint q*l
  NPrint q+l
!EndTestCase

; Str$ is a place without a destination too: a Float form of the same sum
!TestCase{str_of_a_float_form}
  q=1.5 : l.l=100000
  a$=Str$(q+l)
  !CheckStrEq{a$,"100001.5","str_float"}
!EndTestCase
