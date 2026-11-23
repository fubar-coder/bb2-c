; ldivlib-div0-check-fold (#200, #1044) - constant Long and Quick
; divisions with runtime error checking.  The compiler folds a division
; of two constants by running the generated code, error checks included:
; ldivlib's divchk runs inside the compiler, which refuses a constant
; divisor 0 before (#1044), so the check only ever sees a divisor that is
; not 0.
; Correct: the program compiles (with runtime error checking) and gives
; the quotients.
; A regression test of the ldivlib error check (#200).

#c=10/3

!TestCase{fold_quotients}
  d.l=7/2
  !CheckEq{d,3,"long_7_2"}
  d.l=-100000/3
  !CheckEq{d,-33333,"long_minus_100000_3"}
  e.q=7/2
  !CheckQEq{e,3.5,"quick_7_2"}
  !CheckEq{#c,3,"constant"}
  Dim a(20/4)
  a(5)=9
  !CheckEq{a(5),9,"array_last_element"}
  !CheckEq{20/4,5,"constant_quotient"}
!EndTestCase
