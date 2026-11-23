; compiler-loadres-clash-offset (#36) - loadres checks the constants of
; the residents for clashes in the list firstconst, which nothing fills:
; the constants are labels in firstlabel. Two residents both define #c
; (r1: 1, r2: 2), no clash is reported and #c is the one of r1.
; Correct: the warning "Clash in Residents: #c" when the options load
; (the warnings setting on, the default); the compile goes on and #c is
; the first resident's, 1.
!TestCase{first_value}
  !CheckEq{#c,1,"c"}
!EndTestCase
