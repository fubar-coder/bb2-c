; memlib-messages-errarea (#956) - memlib's runtime errors trap in the
; normal code (err1 "Out of Memory!" of localloc/globalloc/newmem, err2
; "Unable to Free Memory" of locfree/globfree), but their messages lie in
; the error check area.  Without runtime error checking that area is not
; part of the program: d0 points behind the library.
; Here, runtime error checking off: Dim of an array of 400 MB (localloc
; fails, err1 frees all memory of the program and traps).  The program
; sets its own trap handler (tc_TrapCode of its task): a TRAP #0
; (exception 32) continues at a label of the case with the message in
; d0 (the stack pointer is the one saved before the act); any other
; exception goes on to the old handler.
; Correct: TRAP #0 with the message "Out of Memory!".  The original
; passes a pointer behind the library (a garbage text).
DEFTYPE .l

!TestCase{trap_message}
  t=Peek.l(Peek.l(4)+276)
  o=Peek.l(t+50)
  Poke.l ?ar+4,o
  Poke.l t+50,?th
  LEA ar(pc),a0 : MOVE.l a7,8(a0)
  Dim a.l(20000,5000)
  Poke.l t+50,o
  !Check{0,"trapped"}
  Goto fin
rec:
  LEA ar(pc),a0 : MOVE.l 8(a0),a7
  Poke.l t+50,o
  mp=Peek.l(?ar)
  !CheckStrEq{Peek$(mp),"Out of Memory!","message"}
  Goto fin
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
  Dc.l 0,0,0
fin:
!EndTestCase
