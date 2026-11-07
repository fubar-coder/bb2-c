; memlib-messages-errarea (#956) - memlib's runtime errors trap in the
; normal code (err1 "Out of Memory!" of localloc/globalloc/newmem, err2
; "Unable to Free Memory" of locfree/globfree), but their messages lie in
; the error check area.  Without runtime error checking that area is not
; part of the program: d0 points behind the library.
; Here, runtime error checking off: Dim of an array of 400 MB (localloc
; fails, err1 frees all memory of the program and traps).  The program
; sets its own trap handler (tc_TrapCode of its task): a TRAP #0
; (exception 32) continues at a label of the program with the message in
; d0; any other exception goes on to the old handler.
; Correct: TRAP #0 with the message "Out of Memory!".  The original
; passes a pointer behind the library (a garbage text).
; @fixed #956
; @configs clib
; @pass timeout=60
DEFTYPE .l
t=Peek.l(Peek.l(4)+276)
o=Peek.l(t+50)
Poke.l ?ar+4,o
Poke.l t+50,?th
NPrint "before"
Dim a.l(20000,5000)
Poke.l t+50,o
NPrint "after"
End

rec:
  Poke.l t+50,o
  NPrint "runtime error ",Peek$(Peek.l(?ar))
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
