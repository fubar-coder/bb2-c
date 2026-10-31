; spriteslib-messages-errarea (#954) - the error routines err10/err11 of
; spriteslib (MOVE.l #message,d0:TRAP #0) are in the normal code, but
; their messages lie in the error check area: without runtime error
; checking that area is not part of the program, and d0 points behind the
; library.  Here SaveSprites and LoadSprites with a file that cannot be
; opened ("Can't open Sprites file"), runtime error checking off.  The
; program sets its own trap handler (tc_TrapCode of its task): a TRAP #0
; (exception 32) continues at a label of the program with the message in
; d0; any other exception goes on to the old handler.
; Correct: both runtime errors with their message.  The original passes
; an address behind the library as the message.
; @configs clib
; @pass timeout=60
DEFTYPE .l
t=Peek.l(Peek.l(4)+276)
o=Peek.l(t+50)
Poke.l ?ar+4,o
Poke.l t+50,?th
NPrint "before"
k=1
SaveSprites 0,1,"RAM:spriteslib-messages-errarea.none/x"
NPrint "no error"
s2:
k=2
LoadSprites 0,1,"RAM:spriteslib-messages-errarea.none"
NPrint "no error"
s3:
Poke.l t+50,o
NPrint "after"
End

rec:
  d=Peek.l(?ar)
  NPrint "runtime error ",Peek$(d)
  If k=1 Then Goto s2
  Goto s3

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
