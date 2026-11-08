; compiler-loadres-clash-same (#36) - two residents define the constant
; #c with the same value (both made of the source #c=1 of
; compiler-loadres-clash-offset): the same definition included twice.
; Correct: no message (only differing values warn); #c is 1.
; @fixed #36
; @pass file=r1.bb2|@TESTDATA@/compiler-loadres-clash-offset.r1
; @pass file=r2.bb2|@TESTDATA@/compiler-loadres-clash-offset.r1
; @pass jobline=resident=r1|CORPUS:r1.bb2|CORPUS:out/r1.res
; @pass jobline=resident=r2|CORPUS:r2.bb2|CORPUS:out/r2.res
NPrint #c
End
