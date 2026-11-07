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
; @fixed #398
; @configs clib
; @pass timeout=60
; @pass file=op3.anim|@TESTDATA@/animlib-errors-check-area.anim
DEFTYPE .l
BitMap 0,16,1,1
t=Peek.l(Peek.l(4)+276)
o=Peek.l(t+50)
Poke.l ?ar+4,o
Poke.l ?ar+8,?rec1
Poke.l t+50,?th
NPrint "before"
LoadAnim 0,"CORPUS:op3.anim"
Poke.l t+50,o
NPrint "no error 1"
End

rec1:
  NPrint "runtime error ",Peek$(Peek.l(?ar))
  Poke.l ?ar+8,?rec2
  LoadAnim 1,"RAM:animlib-errors-check-area.missing"
  Poke.l t+50,o
  NPrint "no error 2"
  End

rec2:
  Poke.l t+50,o
  NPrint "runtime error ",Peek$(Peek.l(?ar))
  End

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
