; screenslib-err1-errorarea (#251) - the runtime errors of screenslib
; that the normal code raises lie in the error check area: "Can't open
; Screen" is `BEQ err1` into it, err8/err12/err13 (WorkBench / Screen /
; CopIns not found) trap in the normal code with their texts in it.
; Without runtime error checking that area is not part of the program:
; the branch lands behind the library, d0 points behind it.
; Here, runtime error checking off: FindScreen with a title no screen has
; (err12), then Screen with a size no chip memory holds (err1).  The
; program sets its own trap handler (tc_TrapCode of its task): a TRAP #0
; (exception 32) continues at the label in ar+8 with the message in d0;
; any other exception goes on to the old handler.
; Correct: both runtime errors are raised, TRAP #0 with their messages.
; The original shows a garbage text for the first and runs into other
; code for the second.
; @fixed #251
; @configs clib
; @pass timeout=60
DEFTYPE .l
t=Peek.l(Peek.l(4)+276)
o=Peek.l(t+50)
Poke.l ?ar+4,o
Poke.l ?ar+8,?rec1
Poke.l t+50,?th
NPrint "before"
FindScreen 0,"bb2-c: no screen has this title"
Poke.l t+50,o
NPrint "no error 1"
End

rec1:
  NPrint "runtime error ",Peek$(Peek.l(?ar))
  Poke.l ?ar+8,?rec2
  Screen 0,0,0,4096,4096,8,0,"too large",1,2
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
