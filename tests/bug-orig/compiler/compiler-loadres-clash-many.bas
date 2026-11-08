; compiler-loadres-clash-many (#36) - two residents define the constants
; #a0..#a9 with different values (1 and 2) and #s with the same value 5.
; Correct: the warnings "Clash in Residents: #a0" .. "#a7" (the first 8
; names, in the order of the first resident) and one "Clash in Residents:
; 2 more"; none for #s; the first resident's values are used.
; @fixed #36
; @pass file=r1.bb2|@TESTDATA@/compiler-loadres-clash-many.r1
; @pass file=r2.bb2|@TESTDATA@/compiler-loadres-clash-many.r2
; @pass jobline=resident=r1|CORPUS:r1.bb2|CORPUS:out/r1.res
; @pass jobline=resident=r2|CORPUS:r2.bb2|CORPUS:out/r2.res
NPrint #a0, " ", #a9, " ", #s
End
