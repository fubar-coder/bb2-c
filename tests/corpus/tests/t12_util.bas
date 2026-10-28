; t12_util - utillib: CreateMsgPort, DeleteMsgPort, Wait, TaskName$
t$=TaskName$(0)
NPrint "task [",t$,"] ",Len(t$)
u$=TaskName$(0)+"|"
NPrint "concat len ",Len(u$)
p1.l=CreateMsgPort("bb2testport")
p2.l=CreateMsgPort("")
p3.l=CreateMsgPort("x")
NPrint "ports ",p1<>0," ",p2<>0," ",p3<>0
NPrint "type ",Peek.b(p1+8)," pri ",Peek.b(p1+9)," flags ",Peek.b(p1+14)
NPrint "name [",Peek$(Peek.l(p1+10)),"] size ",Peek.l(Peek.l(p1+10)-4)
NPrint "noname ",Peek.l(p2+10)," [",Peek$(Peek.l(p3+10)),"]"
NPrint "sigtask ",TaskName$(Peek.l(p1+16))
NPrint "sigbits ",Peek.b(p1+15)-Peek.b(p2+15)," ",Peek.b(p2+15)-Peek.b(p3+15)
NPrint "msglist ",Peek.l(p1+20)=p1+24," ",Peek.l(p1+24)," ",Peek.l(p1+28)=p1+20
NPrint "links ",Peek.l(p1-8)=p2-8," ",Peek.l(p2-8)=p3-8," ",Peek.l(p3-8)," ",Peek.l(p3-4)=p2-8
DeleteMsgPort p2
NPrint "after delete ",Peek.l(p1-8)=p3-8," ",Peek.l(p3-4)=p1-8
p4.l=CreateMsgPort("another port")
NPrint "relinked ",Peek.l(p3-8)=p4-8," ",Peek.l(p4-4)=p3-8
DeleteMsgPort p1
; signal ourselves (SIGBREAKF_CTRL_F) so that Wait returns at once
MOVE.l 4,a6:SUB.l a1,a1:JSR -294(a6):MOVE.l d0,a1:MOVE.l #$8000,d0:JSR -324(a6)
w.l=Wait
; only CTRL_F: SIGF_DOS ($100) may also be set, depending on timing
NPrint "wait ",Hex$(w AND $F000)
MOVE.l 4,a6:SUB.l a1,a1:JSR -294(a6):MOVE.l d0,a1:MOVE.l #$4000,d0:JSR -324(a6)
Wait
NPrint "wait statement ok"
End
