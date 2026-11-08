; compiler-loadres-clash-offset (#36) - loadres checks the constants of
; the residents for clashes in the list firstconst, which nothing fills:
; the constants are labels in firstlabel. Two residents both define #c
; (r1: 1, r2: 2), no clash is reported and #c is the one of r1.
; Correct: the warning "Clash in Residents: #c" when the options load
; (the warnings setting on, the default); the compile goes on and #c is
; the first resident's, 1.
; @fixed #36
; @pass file=r1.bb2|@TESTDATA@/compiler-loadres-clash-offset.r1
; @pass file=r2.bb2|@TESTDATA@/compiler-loadres-clash-offset.r2
; @pass jobline=resident=r1|CORPUS:r1.bb2|CORPUS:out/r1.res
; @pass jobline=resident=r2|CORPUS:r2.bb2|CORPUS:out/r2.res
NPrint #c
End
