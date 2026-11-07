; compiler-incdir-rest-of-line (#17) - doincdir copies the rest of the
; line into the path of IncDir and only skips the quotes: after IncDir
; "CORPUS:src/" : a=2 the path is "CORPUS:src/:a=2", a=2 is not
; compiled, and Include "inc.bb2" opens "CORPUS:src/:a=2inc.bb2" (Can't
; open Include).
; Correct: the quoted string is the path, ':' starts the next statement;
; the program prints 2 and 7.
; @fixed #17
; @pass file=src/inc.bb2|@TESTDATA@/compiler-incdir-rest-of-line.inc
a=1
IncDir "CORPUS:src/" : a=2
Include "inc.bb2"
NPrint a
NPrint #k
End
