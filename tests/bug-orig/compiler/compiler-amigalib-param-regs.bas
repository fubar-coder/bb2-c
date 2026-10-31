; compiler-amigalib-param-regs (#344) - an Amiga library function used
; inside an expression: blitz2 saves only the data registers covered by
; savereg (d0/d1) around the call, so a parameter loaded into d2 or higher
; overwrites an intermediate value of the expression held there. Write_
; takes fh in d1, buf in d2, len in d3; in a+(b+(c+Write_(...))) c is in a
; register >= d2 when the call is made. Correct: the program prints 1234
; twice. The wrong sum depends on where s$ is, so the program prints only
; that it is wrong, not its value (the same text for every run and build).
fh.l=Open_("NIL:",1006)
s$="abcd"
a.l=1000:b.l=200:c.l=30
x.l=Write_(fh,&s$,4)
NPrint a+b+c+x
y.l=a+(b+(c+Write_(fh,&s$,4)))
If y=1234 Then NPrint y Else NPrint "wrong: not 1234"
Close_ fh
End
