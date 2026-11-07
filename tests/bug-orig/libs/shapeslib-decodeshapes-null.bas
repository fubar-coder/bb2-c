; shapeslib-decodeshapes-null (#270) - DecodeShapes with address 0
; branches (TST.l d2:BEQ err10) from the normal code into the error check
; area: without runtime error checking that area is not part of the
; program, and the branch lands behind the library.  Here both forms,
; DecodeShapes 0,0 and DecodeShapes 0,1,0, runtime error checking off.
; The program sets its own trap handler (tc_TrapCode of its task): a
; TRAP #0 (exception 32) continues at a label of the program with the
; message in d0; any other exception goes on to the old handler.
; Correct: both runtime errors with their message.  The original runs
; into other code.
; @fixed #270
; @configs clib
; @pass timeout=60
DEFTYPE .l
t=Peek.l(Peek.l(4)+276)
o=Peek.l(t+50)
Poke.l ?ar+4,o
Poke.l t+50,?th
NPrint "before"
k=1
DecodeShapes 0,0
NPrint "no error"
s2:
k=2
DecodeShapes 0,1,0
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
