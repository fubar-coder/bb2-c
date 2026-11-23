; memlib-free-runerr - a runtime error inside a recursion leaves the
; lists of the procedures it left open (each with its local strings); the
; end of the program frees them and memlib's list headers.  Runtime error
; checking off: at depth 10 the procedure raises a runtime error (TRAP #0,
; as the libraries do).  The program sets its own trap handler
; (tc_TrapCode of its task): a TRAP #0 (exception 32) continues at a label
; of the program, which ends it; any other exception goes on to the old
; handler.  The program runs through inject (track: what the process
; leaves allocated).
; Correct: nothing is left allocated (inject's report, asserted on
; `inject`).  The trap handler ends the program: the case has no end
; entry.

DEFTYPE .l

Statement deep{n,s$}
  u$=s$
  If n=0
    !Check{-1,"depth_10"}
    MOVE.l #0,d0 : TRAP #0
  EndIf
  deep{n-1,u$}
End Statement

!TestCase{runtime_error_in_recursion}
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
  !Check{-1,"runtime_error_caught"}
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
