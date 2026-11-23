; shapeslib-messages-errarea (#953) - the error routines err2/err3/err7/
; err8 of shapeslib (MOVE.l #message,d0:TRAP #0) are in the normal code,
; but their messages lie in the error check area: without runtime error
; checking that area is not part of the program, and d0 points behind the
; library.  Here SaveShapes, LoadShapes and LoadShape with a file that
; cannot be opened ("Can't open Shapes File"), runtime error checking
; off.  The program sets its own trap handler (tc_TrapCode of its task):
; a TRAP #0 (exception 32) continues at a label of the program with the
; message in d0; any other exception goes on to the old handler.
; Correct: the three runtime errors with their message.  The original
; passes an address behind the library as the message.
; The handler (labels, assembler and the data of its longs) stands in the
; case, behind a Goto that skips it; the label rec continues the case
; after a trap: it restores the stack pointer (the trap left the library's
; frames on it), checks the message and goes on with the next statement.
DEFTYPE .l

!TestCase{messages}
  t=Peek.l(Peek.l(4)+276)
  o=Peek.l(t+50)
  Poke.l ?ar+4,o
  Poke.l t+50,?th
  LEA sv(pc),a0 : MOVE.l a7,(a0)
  k=1
  SaveShapes 0,0,"CORPUS:shapeslib-messages-errarea.none/x"
  !Check{0,"no_error_1"}
s2:
  k=2
  LoadShapes 0,"CORPUS:shapeslib-messages-errarea.none"
  !Check{0,"no_error_2"}
s3:
  k=3
  LoadShape 0,"CORPUS:shapeslib-messages-errarea.none"
  !Check{0,"no_error_3"}
s4:
  Poke.l t+50,o
  !Check{1,"after"}
  Goto fin

rec:
  LEA sv(pc),a0 : MOVE.l (a0),a7
  If k=1
    !CheckStrEq{Peek$(Peek.l(?ar)),"Can't open Shapes File","message_1"}
    Goto s2
  EndIf
  If k=2
    !CheckStrEq{Peek$(Peek.l(?ar)),"Can't open Shapes File","message_2"}
    Goto s3
  EndIf
  !CheckStrEq{Peek$(Peek.l(?ar)),"Can't open Shapes File","message_3"}
  Goto s4

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
sv:
  Dc.l 0

fin:
!EndTestCase
