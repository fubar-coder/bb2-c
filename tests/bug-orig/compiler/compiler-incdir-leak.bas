; compiler-incdir-leak (#342) - incdir (the path of IncDir) is set only by
; IncDir and never cleared: it stays for every following compile of the
; same blitz2 session (TED, tedstub). Here a resident is compiled first,
; with IncDir "CORPUS:nowhere/"; this program has no IncDir, but its
; relative Include "inc.bb2" opens "CORPUS:nowhere/inc.bb2" and fails
; (Can't open Include).
; Correct: every compile starts with an empty incdir, the program prints 7.
; @pass file=r.bb2|@TESTDATA@/compiler-incdir-leak.res
; @pass file=src/inc.bb2|@TESTDATA@/compiler-incdir-leak.inc
; @pass jobline=resident=r|CORPUS:r.bb2|CORPUS:out/r.res
Include "inc.bb2"
NPrint #k
End
