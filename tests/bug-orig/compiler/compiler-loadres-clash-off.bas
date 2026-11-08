; compiler-loadres-clash-off (#36) - the residents of
; compiler-loadres-clash-offset (#c: 1 and 2) with the warnings setting
; "off" (word +24 of the .xtra $0000): the warning resclash is disabled.
; Correct: no message; #c is the first resident's, 1.
; @fixed #36
; @pass file=r1.bb2|@TESTDATA@/compiler-loadres-clash-offset.r1
; @pass file=r2.bb2|@TESTDATA@/compiler-loadres-clash-offset.r2
; @pass jobline=resident=r1|CORPUS:r1.bb2|CORPUS:out/r1.res
; @pass jobline=resident=r2|CORPUS:r2.bb2|CORPUS:out/r2.res
; @pass jobline=warnings=off
NPrint #c
End
