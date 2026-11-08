; compiler-loadres-clash-werror (#36) - the residents of
; compiler-loadres-clash-offset (#c: 1 and 2) with the warnings setting
; "errors" (word +24 of the .xtra $0180): the warning resclash is an
; error.
; Correct: the error "Clash in Residents: #c" when the options load, the
; residents are dropped as for a clashing type, and the compile fails on
; #c.
; @fixed #36
; @pass file=r1.bb2|@TESTDATA@/compiler-loadres-clash-offset.r1
; @pass file=r2.bb2|@TESTDATA@/compiler-loadres-clash-offset.r2
; @pass jobline=resident=r1|CORPUS:r1.bb2|CORPUS:out/r1.res
; @pass jobline=resident=r2|CORPUS:r2.bb2|CORPUS:out/r2.res
; @pass jobline=warnings=errors
NPrint #c
End
