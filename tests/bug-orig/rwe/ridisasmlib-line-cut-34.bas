; ridisasmlib-line-cut-34 (#616) - DisAssemble$ puts a 0 at column 34 of
; its line and always returns the first 34 columns: the operands of a
; long 68020 instruction are cut in the middle.  The line buffer has 80
; bytes; MOVE.l with two memory indirect <ea>s with long base and outer
; displacements makes a line of 86 bytes, whose end overwrites the
; library's variables behind the buffer.
; Correct: at least 34 columns (a short line padded with spaces), a
; longer line complete.
; @rwe ridisasmlib
DEFTYPE .l
Dim c.w(12)
c(0)=$21B0 : c(1)=$0133 : c(2)=$8000 : c(3)=$0000 : c(4)=$8000 : c(5)=$0000
c(6)=$0133 : c(7)=$8000 : c(8)=$0000 : c(9)=$8000 : c(10)=$0000
a$=DisAssemble$(&c(0))
NPrint Len(a$)," [",a$,"] ",InstrSize
c(0)=$4E75
a$=DisAssemble$(&c(0))
NPrint Len(a$)," [",a$,"] ",InstrSize
End
