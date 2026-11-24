; animlib-errors-check-area (#398) - LoadAnim's runtime errors: "Can't
; Open Anim File" is `BEQ err7` into the error check area, "Anim format
; not supported" (err6) traps in the normal code with its text in the
; error check area and runs on into err1 if the trap returns.  Without
; runtime error checking that area is not part of the program: the
; branch lands behind the library, d0 points behind it.
; Here, runtime error checking off: an ANIM (generated) whose ANHD has
; operation 3 (err6), then a file that does not exist (err7).  The
; program sets its own trap handler (tc_TrapCode of its task): a TRAP #0
; (exception 32) continues at the label in ar+8 with the message in d0;
; any other exception goes on to the old handler.
; Correct: both runtime errors are raised, TRAP #0 with their messages.
; The original shows a garbage text for the first and runs into other
; code for the second.
; The handler (labels, assembler and the data of its longs) stands in the
; case, behind a Goto that skips it; the labels rec1 and rec2 continue the
; case after a trap: each restores the stack pointer (the trap left the
; library's frames on it) and checks the message.
; Where the original's branch for the second error lands is the code at
; offset $28 of the library the compiler copies behind animlib.  The
; expression ReadSerial(0) behind the Goto after rec2 is never run: it
; makes the compiler copy serial library 90 behind animlib (the last
; library, the one the compiler appends, follows it).  At its offset $28
; are ORI.b #2,d4 (the data of a branch), MOVEA.l (a7)+,a6 and an RTS:
; the saved a4 of the LoadAnim is popped and the original returns from it
; as if nothing were wrong.
DEFTYPE .l

!TestCase{messages}
  BitMap 0,16,1,1
  t=Peek.l(Peek.l(4)+276)
  o=Peek.l(t+50)
  Poke.l ?ar+4,o
  Poke.l ?ar+8,?rec1
  Poke.l t+50,?th
  LEA sv(pc),a0 : MOVE.l a7,(a0)
  LoadAnim 0,"CORPUS:op3.anim"
  Poke.l t+50,o
  !Check{0,"no_error_1"}
  Goto fin

rec1:
  LEA sv(pc),a0 : MOVE.l (a0),a7
  !CheckStrEq{Peek$(Peek.l(?ar)),"Anim format not supported","message_1"}
  Poke.l ?ar+8,?rec2
  LoadAnim 1,"CORPUS:animlib-errors-check-area.missing"
  Poke.l t+50,o
  !Check{0,"no_error_2"}
  Goto fin

rec2:
  LEA sv(pc),a0 : MOVE.l (a0),a7
  Poke.l t+50,o
  !CheckStrEq{Peek$(Peek.l(?ar)),"Can't Open Anim File","message_2"}
  Goto fin
  x=ReadSerial(0)

th:
  CMP.l #32,(a7) : BNE thold
  ADDQ.l #4,a7
  LEA ar(pc),a0 : MOVE.l d0,(a0)
  MOVE.l ar+8(pc),2(a7)
  RTE
thold:
  MOVE.l ar+4(pc),-(a7)
  RTS

ar:
  Dc.l 0,0,0
sv:
  Dc.l 0

fin:
!EndTestCase
