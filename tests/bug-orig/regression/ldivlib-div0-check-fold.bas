; ldivlib-div0-check-fold (#200, #1044) - constant Long and Quick
; divisions with runtime error checking.  The compiler folds a division
; of two constants by running the generated code, error checks included:
; ldivlib's divchk runs inside the compiler, which refuses a constant
; divisor 0 before (#1044), so the check only ever sees a divisor that is
; not 0.
; Correct: the program compiles and prints the quotients.
; A regression test of the ldivlib error check (#200).
; @pass runtime_errors=on
d.l=7/2
NPrint d
d.l=-100000/3
NPrint d
e.q=7/2
NPrint e
#c=10/3
NPrint #c
Dim a(20/4)
NPrint 20/4
End
