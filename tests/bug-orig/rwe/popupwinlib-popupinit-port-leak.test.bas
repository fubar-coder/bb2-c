; popupwinlib-popupinit-port-leak (#470) - the library's init already
; creates the message port of the menu windows; PopUpInit creates another
; one with CreateMsgPort and overwrites the pointer without deleting the
; old port: every call keeps a port and its signal bit until the program
; ends (after about 16 calls CreateMsgPort fails and PopUpWindow loops
; for ever).
; Here: the signals allocated by the program's task (the bits set in
; tc_SigAlloc, +18 of the Task) are counted before and after three
; PopUpInit.
; Correct: PopUpInit keeps the port it has; no signal is lost.
DEFTYPE .l

!TestCase{signals_lost}
  t=FindTask_(0)
  s0=Peek.l(t+18)
  PopUpInit 0
  PopUpInit 0
  PopUpInit 0
  s1=Peek.l(t+18)
  n=0
  For i=0 To 31
    If (s0 AND (1 LSL i))=0 AND (s1 AND (1 LSL i))<>0 Then n=n+1
  Next
  !CheckEq{n,0,"signals_lost"}
!EndTestCase
