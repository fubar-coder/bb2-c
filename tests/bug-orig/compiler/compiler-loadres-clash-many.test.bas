; compiler-loadres-clash-many (#36) - two residents define the constants
; #a0..#a9 with different values (1 and 2) and #s with the same value 5.
; Correct: the warnings "Clash in Residents: #a0" .. "#a7" (the first 8
; names, in the order of the first resident) and one "Clash in Residents:
; 2 more"; none for #s; the first resident's values are used.
!TestCase{first_values}
  !CheckEq{#a0,1,"a0"}
  !CheckEq{#a9,1,"a9"}
  !CheckEq{#s,5,"same"}
!EndTestCase
