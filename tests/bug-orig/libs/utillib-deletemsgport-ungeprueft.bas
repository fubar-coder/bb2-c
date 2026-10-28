; utillib-deletemsgport-ungeprueft (#299) - DeleteMsgPort takes the port
; as it is: it frees the port with the messages still queued on it and
; does not reply them, a sender waiting at its reply port never gets the
; message back.  (The other half of the issue, a port utillib did not
; create, corrupts memory in a way that is not reliably observable.)
; Here a message with reply port r is put on port p (exec PutMsg), then p
; is deleted.
; Correct: the pending message is replied - it arrives at r.
; @configs clib
p.l=CreateMsgPort("")
r.l=CreateMsgPort("")
m$=String$(Chr$(0),20) : m.l=&m$
Poke.b m+8,5 : Poke.l m+14,r : Poke.w m+18,20
bk$=String$(Chr$(0),8) : bk.l=&bk$ : Poke.l bk,p : Poke.l bk+4,m
; PutMsg(p,m)
GetReg d0,bk
MOVEM.l d2-d7/a2-a6,-(a7)
MOVE.l d0,a1:MOVE.l (a1),a0:MOVE.l 4(a1),a1:MOVE.l 4,a6:JSR -366(a6)
MOVEM.l (a7)+,d2-d7/a2-a6
NPrint "queued ",Peek.l(p+20)=m
DeleteMsgPort p
; GetMsg(r)
GetReg d0,r
MOVEM.l d2-d7/a2-a6,-(a7)
MOVE.l d0,a0:MOVE.l 4,a6:JSR -372(a6)
MOVEM.l (a7)+,d2-d7/a2-a6
PutReg d0,g.l
NPrint "replied ",g=m
DeleteMsgPort r
End
