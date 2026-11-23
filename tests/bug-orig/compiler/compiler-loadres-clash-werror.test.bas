; compiler-loadres-clash-werror (#36) - the residents of
; compiler-loadres-clash-offset (#c: 1 and 2) with the warnings setting
; "errors" (word +24 of the .xtra $0180): the warning resclash is an
; error.
; Correct: the error "Clash in Residents: #c" when the options load, the
; residents are dropped as for a clashing type, and the compile fails on
; #c (nothing runs). The original reports no clash, takes #c of the first
; resident, and the program runs.
!TestCase{first_value}
  !CheckEq{#c,1,"c"}
!EndTestCase
