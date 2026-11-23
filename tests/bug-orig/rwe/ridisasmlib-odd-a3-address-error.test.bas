; ridisasmlib-odd-a3-address-error (#617) - DisAssemble$ copies its 34
; columns to a3 with 17 word moves (MOVE.w (a0)+,(a3)+).  a3 is the end
; of what the string expression built before: after "x" it is odd, and a
; word write to an odd address is an address error on a 68000 (the
; program crashes; a 68020 writes it).  The machine is the A500 (68000).
; Correct: the line is written byte by byte on every CPU.
DEFTYPE .l

!TestCase{odd_target}
  Dim c.w(3)
  c(0)=$4E75
  a$="x"+DisAssemble$(&c(0))
  !CheckEq{Len(a$),35,"length"}
  !CheckStrEq{a$,"xrts"+String$(" ",31),"line"}
!EndTestCase
