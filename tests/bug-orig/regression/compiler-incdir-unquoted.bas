; compiler-incdir-unquoted (#17) - an unquoted path of IncDir and an
; unquoted name of Include are the rest of the line, also after ':' (the
; fix of #17 changes only quoted names): IncDir CORPUS:src/ sets the path
; "CORPUS:src/", Include zz.bb2 opens "CORPUS:src/zz.bb2".
; Correct: the program prints 7.
; A regression test of the fix of #17: the original already gives this (correct) result.
; @pass file=src/zz.bb2|@TESTDATA@/compiler-incdir-quote-next.inc
IncDir CORPUS:src/
Include zz.bb2
NPrint #k
End
