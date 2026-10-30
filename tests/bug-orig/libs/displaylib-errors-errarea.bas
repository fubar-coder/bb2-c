; displaylib-errors-errarea (#412) - the error routines of displaylib
; (MOVE.l #message,d0:TRAP #0) lie in the error check area, but the
; normal code branches to them: without runtime error checking that area
; is not part of the program, and the branch lands behind the library.
; Here InitCopList with 9 bitplanes (more than the 8 of every
; resolution: maxbperr), runtime error checking off.  The program sets
; its own trap handler (tc_TrapCode of its task): a TRAP #0 (exception
; 32) continues at a label of the program with the message in d0; any
; other exception goes on to the old handler.
; Correct: the runtime error is raised, TRAP #0 with the message.  The
; original runs into other code (a software failure).
; @configs clib
; @pass timeout=60
DEFTYPE .l
t=Peek.l(Peek.l(4)+276)
o=Peek.l(t+50)
Poke.l ?ar+4,o
Poke.l t+50,?th
NPrint "before"
InitCopList 0,$9
Poke.l t+50,o
NPrint "after"
End

rec:
  Poke.l t+50,o
  d=Peek.l(?ar)
  NPrint "runtime error ",Peek$(d)
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
