; rundebuglib-supertrap-d0-lost (#246) - a runtime error (TRAP #0, d0 =
; message) in supervisor mode: mytrap0 continues at more2, which calls
; codefinit (fixtraps, CloseLibrary) and then gives TRAP #0 to the old
; vector - with d0 changed by codefinit.
; The case sets its own TRAP #0 handler, starts a child
; (rundebuglib-supertrap-d0-lost.test/child.bas) and puts the vector back.
; The child makes a runtime error ("Uninitialized Shape") once, inside a
; SetInt 5 routine (supervisor mode); the case's handler keeps d0 and
; returns.  The original gives d0 = 0, and more2 runs on behind its trap:
; the child goes on to "child back".  (Since #1069 a SetInt routine is the
; only place where a runtime error in supervisor mode reaches rundebuglib.
; Since #1073 rundebuglib gives such an error to no old vector: it leaves
; the routine and shows the message at the child's next statement, the
; case's handler is not called.)
; Correct: the message ("Uninitialized Shape") is kept: the child's
; requester shows it, and the child ends there.
!TestCase{setint_error}
  v.l=cr_vbr
  o.l=Peek.l(v+128)
  Poke.l ?sd_ar,0
  Poke.l v+128,?sd_th
  *r.TLRun=!RunProgram{"@TESTDIR@/child","child"}
  Poke.l v+128,o
  d.l=Peek.l(?sd_ar)
  !CheckEq{d,0,"handler"}
  !CheckStrEq{*r\printed,"child"+Chr$(10),"printed"}
  Goto sd_done
sd_th:
  LEA sd_ar(pc),a0 : MOVE.l d0,(a0)
  RTE
sd_ar:
  Dc.l 0
sd_done:
!EndTestCase
