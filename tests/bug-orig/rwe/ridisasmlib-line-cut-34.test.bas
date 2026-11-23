; ridisasmlib-line-cut-34 (#616) - DisAssemble$ puts a 0 at column 34 of
; its line and always returns the first 34 columns: the operands of a
; long 68020 instruction are cut in the middle.  The line buffer has 80
; bytes; MOVE.l with two memory indirect <ea>s with long base and outer
; displacements makes a line of 86 bytes, whose end overwrites the
; library's variables behind the buffer.
; Correct: at least 34 columns (a short line padded with spaces), a
; longer line complete.
DEFTYPE .l

!TestCase{long_line}
  Dim c.w(12)
  c(0)=$21B0 : c(1)=$0133 : c(2)=$8000 : c(3)=$0000 : c(4)=$8000 : c(5)=$0000
  c(6)=$0133 : c(7)=$8000 : c(8)=$0000 : c(9)=$8000 : c(10)=$0000
  a$=DisAssemble$(&c(0))
  n=InstrSize
  e$="move.l   ([-2147483648,a0,d0.w],-2147483648),"
  e$=e$+"([-2147483648,a0,d0.w],-2147483648)"
  !CheckEq{Len(a$),80,"long_length"}
  !CheckStrEq{a$,e$,"long_line"}
  !CheckEq{n,22,"long_size"}
  c(0)=$4E75
  b$=DisAssemble$(&c(0))
  !CheckEq{Len(b$),34,"short_length"}
  !CheckStrEq{b$,"rts"+String$(" ",31),"short_line"}
  !CheckEq{InstrSize,2,"short_size"}
!EndTestCase
