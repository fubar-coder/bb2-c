; utillib-deletemsgport-unchecked (#299) - DeleteMsgPort takes the port
; as it is: it frees the port with the messages still queued on it and
; does not reply them, a sender waiting at its reply port never gets the
; message back.  (The other half of the issue, a port utillib did not
; create, corrupts memory in a way that is not reliably observable.)
; Here a message with reply port r is put on port p (exec PutMsg), then p
; is deleted.
; Correct: the pending message is replied - it arrives at r.
DEFTYPE .l

!TestCase{queued_message}
  p=CreateMsgPort("")
  r=CreateMsgPort("")
  m$=String$(Chr$(0),20) : m=&m$
  Poke.b m+8,5 : Poke.l m+14,r : Poke.w m+18,20
  bk$=String$(Chr$(0),8) : bk=&bk$ : Poke.l bk,p : Poke.l bk+4,m
  ; PutMsg(p,m)
  GetReg d0,bk
  MOVEM.l d2-d7/a2-a6,-(a7)
  MOVE.l d0,a1:MOVE.l (a1),a0:MOVE.l 4(a1),a1:MOVE.l 4,a6:JSR -366(a6)
  MOVEM.l (a7)+,d2-d7/a2-a6
  q=Peek.l(p+20)
  DeleteMsgPort p
  ; GetMsg(r)
  GetReg d0,r
  MOVEM.l d2-d7/a2-a6,-(a7)
  MOVE.l d0,a0:MOVE.l 4,a6:JSR -372(a6)
  MOVEM.l (a7)+,d2-d7/a2-a6
  PutReg d0,g
  DeleteMsgPort r
  !Require{q=m,"queued"}
  !CheckEq{g,m,"replied"}
!EndTestCase
