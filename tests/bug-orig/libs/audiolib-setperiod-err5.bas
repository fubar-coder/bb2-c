; audiolib-setperiod-err5 (#70) - SetPeriod without a sound branches
; (BEQ err5) from the normal code into the error check area, and the
; messages of err1-err4 (normal code) lie there too: without runtime
; error checking that area is not part of the program, the branch lands
; behind the library and d0 points behind it.  Here SetPeriod on an empty
; sound ("Uninitialized Sound") and LoadSound of a missing file
; ("Illegal Sound file"), runtime error checking off.  The program sets
; its own trap handler (tc_TrapCode of its task): a TRAP #0 (exception
; 32) continues at a label of the program with the message in d0; any
; other exception goes on to the old handler.
; Correct: both runtime errors with their messages.  The original runs
; into other code for SetPeriod and passes an address behind the library
; as the message of LoadSound.
; @fixed #70
; @configs clib
; @pass timeout=60
DEFTYPE .l
t=Peek.l(Peek.l(4)+276)
o=Peek.l(t+50)
Poke.l ?ar+4,o
Poke.l t+50,?th
NPrint "before"
k=1
SetPeriod 0,428
NPrint "no error"
s2:
k=2
LoadSound 0,"RAM:audiolib-setperiod-err5.none"
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
