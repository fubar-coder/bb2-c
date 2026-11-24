; riseriallib-readstring-open (#796) - SerialReadString with the port open
; pushes d0 and returns to it (the long it pushed, not its return
; address): the program goes on at whatever d0 held at the call.  The
; program sends "OK" and a 0 to the remote end (serial=on,
; tests/corpus/runner/serial.h), which echoes them (request E), and reads
; them with SerialReadString.
; To give the wrong return a defined end the program loads d0 with the
; address of rec before the call: the original returns to rec, which sets
; the long mk to 1 and executes an RTS.  What that RTS runs next follows
; the stack the library left and the layout of the test program (the
; driver and testlib): measured it is a hang (the session ends in the
; time limit); the same return seen as a normal end is declared too.  The
; check "return_address" (mk is still 0) shows the bug in either case.
; Correct: SerialReadString returns the string up to the 0: "OK".
DEFTYPE .l

!TestCase{readstring_open}
  SerialOpen 19200
  q$=Chr$(1)+"riseriallib-readstring-open"+Chr$(2)+"OK"
  For i=1 To Len(q$) : SerialWrite Asc(Mid$(q$,i,1)) : Next
  SerialWrite 0 : SerialWrite 5 : SerialWrite Asc("E")
  LEA rec(pc),a0 : MOVE.l a0,d0
  a$=SerialReadString
  SerialWrite 4 : VWait 10
  !CheckEq{Peek.l(?mk),0,"return_address"}
  !CheckStrEq{a$,"OK","string"}
  Goto fin
rec:
  LEA mk(pc),a0 : MOVE.l #1,(a0) : MOVEQ #0,d0 : RTS
mk:
  Dc.l 0
fin:
!EndTestCase
