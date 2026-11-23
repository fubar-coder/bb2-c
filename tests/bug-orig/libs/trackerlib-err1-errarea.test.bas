; trackerlib-err1-errarea (#957) - "Error Accessing Module File" (err1 of
; LoadModule) traps in the normal code, but its message lies in the error
; check area.  Without runtime error checking that area is not part of
; the program: d0 points behind the library.
; Here, runtime error checking off: LoadModule of a file that does not
; exist.  The case sets its own trap handler (tc_TrapCode of its
; task): a TRAP #0 (exception 32) continues at a label of the case
; with the message in d0, the stack pointer saved before the act restored;
; any other exception goes on to the old handler.
; Correct: TRAP #0 with the message "Error Accessing Module File".  The
; original passes a pointer behind the library (a garbage text).
DEFTYPE .l

!TestCase{message}
  t=Peek.l(Peek.l(4)+276)
  o=Peek.l(t+50)
  Poke.l ?ar+4,o
  Poke.l t+50,?th
  Goto go1

  rec:
    msg$=Peek$(Peek.l(?ar))
    Goto done1

  th:
    CMP.l #32,(a7) : BNE thold
    ADDQ.l #4,a7
    LEA ar(pc),a0 : MOVE.l d0,(a0)
    MOVE.l 8(a0),a5 : MOVE.l 12(a0),a0 : MOVE a0,USP
    LEA rec(pc),a0 : MOVE.l a0,2(a7)
    RTE
  thold:
    MOVE.l ar+4(pc),-(a7)
    RTS

  ar:
    Dc.l 0,0,0,0

  go1:
  LEA ar(pc),a0 : MOVE.l a5,8(a0) : MOVE.l a7,12(a0)
  LoadModule 0,"RAM:trackerlib-err1-errarea.mod"
  msg$="no runtime error"
  done1:
  Poke.l t+50,o
  !CheckStrEq{msg$,"Error Accessing Module File","message"}
!EndTestCase
