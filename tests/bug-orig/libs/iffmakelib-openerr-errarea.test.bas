; iffmakelib-openerr-errarea (#185) - "Can't open File" (err2 of
; SaveBitmap, SaveScreen, SaveShape, SavePalette) traps in the normal
; code, but its message lies in the error check area.  Without runtime
; error checking that area is not part of the program: d0 points behind
; the library.
; Here, runtime error checking off: SaveBitmap into a directory that does
; not exist.  The program sets its own trap handler (tc_TrapCode of its
; task): a TRAP #0 (exception 32) continues at a label of the program
; with the message in d0; any other exception goes on to the old handler.
; Correct: TRAP #0 with the message "Can't open File".  The original
; passes a pointer behind the library (a garbage text).
; The handler (labels, assembler and the data of its two longs) stands in
; the case, behind a Goto that skips it; the label rec continues the case
; after the trap, checks the message and ends the program.
DEFTYPE .l

!TestCase{trap_message}
  BitMap 0,32,2,1
  t=Peek.l(Peek.l(4)+276)
  o=Peek.l(t+50)
  Poke.l ?ar+4,o
  Poke.l t+50,?th
  SaveBitmap 0,"CORPUS:iffmakelib-openerr-no-dir/t.iff"
  Poke.l t+50,o
  !Check{0,"no_trap"}
  Goto fin

rec:
  Poke.l t+50,o
  !CheckStrEq{Peek$(Peek.l(?ar)),"Can't open File","message"}
  End

th:
  CMP.l #32,(a7) : BNE thold
  ADDQ.l #4,a7
  LEA ar(pc),a0 : MOVE.l d0,(a0)
  LEA rec(pc),a0 : MOVE.l a0,2(a7)
  RTE
thold:
  MOVE.l ar+4(pc),-(a7)
  RTS

ar:
  Dc.l 0,0

fin:
!EndTestCase
