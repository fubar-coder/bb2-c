; utillib-createmsgport-no-signal (#298) - CreateMsgPort tests the result
; of AllocSignal(-1) for 0 (TST.l d0:BEQ error_NoSig), but a failure is
; -1: with every signal of the task allocated it makes a port with
; mp_SigBit $FF, whose messages never wake the task.
; Here the program allocates every free signal (AllocSignal(-1) until it
; returns -1), sets its own trap handler (tc_TrapCode of its task: a
; TRAP #0, exception 32, records d0 and returns; any other exception goes
; on to the old handler), then CreateMsgPort("").  The signals are freed
; again before the checks; the handler is inside the case, behind a jump.
; Correct: no port: the runtime error "Failed to allocate signal for
; Port", and CreateMsgPort returns 0 when the trap returns.
DEFTYPE .l

!TestCase{no_signal}
  Dim sg.l(32)
  n=0
  Repeat
    MOVEM.l d2-d7/a2-a6,-(a7)
    MOVEQ #-1,d0:MOVE.l 4,a6:JSR -330(a6)
    MOVEM.l (a7)+,d2-d7/a2-a6
    PutReg d0,s
    s=s AND 255
    If s<>255 Then sg(n)=s : n+1
  Until s=255 OR n>31
  t=Peek.l(Peek.l(4)+276)
  o=Peek.l(t+50)
  Poke.l ?ar+4,o
  Poke.l t+50,?th
  p=CreateMsgPort("")
  Poke.l t+50,o
  e$=""
  If Peek.l(?ar) Then e$=Peek$(Peek.l(?ar))
  For i=0 To n-1
    s=sg(i)
    GetReg d0,s
    MOVEM.l d2-d7/a2-a6,-(a7)
    MOVE.l 4,a6:JSR -336(a6)
    MOVEM.l (a7)+,d2-d7/a2-a6
  Next
  !CheckStrEq{e$,"Failed to allocate signal for Port","error"}
  !CheckEq{p,0,"port"}
  Goto skiphandler

th:
  CMP.l #32,(a7) : BNE thold
  ADDQ.l #4,a7
  LEA ar(pc),a0 : MOVE.l d0,(a0)
  RTE
thold:
  MOVE.l ar+4(pc),-(a7)
  RTS

ar:
  Dc.l 0,0
skiphandler:
!EndTestCase
