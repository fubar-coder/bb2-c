; memlib-free-err1 - "Out of Memory!" inside a recursion: memlib's err1
; frees every list and memlib's list headers before the runtime error.
; Runtime error checking off: at depth 10 the procedure dimensions a local
; array of 400 MB (localloc fails).  The program sets its own trap handler
; (tc_TrapCode of its task): a TRAP #0 (exception 32) continues at a label
; of the program, which ends it without touching Blitz memory; any other
; exception goes on to the old handler.  The program runs through inject
; (track: what the process leaves allocated).
; Correct: nothing is left allocated (inject's report, asserted on
; `inject`).  The trap handler ends the program: the case has no end
; entry.

DEFTYPE .l

Statement deep{n,s$}
  u$=s$
  If n=0
    !Check{-1,"depth_10"}
    Dim a.l(20000,5000)
  EndIf
  deep{n-1,u$}
End Statement

!TestCase{out_of_memory_in_recursion}
  t=Peek.l(Peek.l(4)+276)
  o=Peek.l(t+50)
  Poke.l ?ar+4,o
  Poke.l t+50,?th
  a$="Hello"
  deep{10,a$}
  Poke.l t+50,o
  !Check{0,"after_deep"}
  Goto skip

  caught:
  Poke.l t+50,o
  End

  th:
  CMP.l #32,(a7) : BNE thold
  ADDQ.l #4,a7
  LEA caught(pc),a0 : MOVE.l a0,2(a7)
  RTE
  thold:
  MOVE.l ar+4(pc),-(a7)
  RTS

  ar:
  Dc.l 0,0

  skip:
!EndTestCase
